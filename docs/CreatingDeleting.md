# Creating and Deleting Preparations
# Creating Preparations
There are two ways to do this:
- Press the one of the hot keys
- Right click anywhere on the screen and select the preparation you want from the from the popup

Either one of these actions results in the a preparation popping up on your screen. Let's first walk through how this happens at a high level then we'll dig into some details in a function call trace.
We will use the direct preparation as an example for simplicity.

## Preparation Creation Overview
1. ConstructionSite registers a key press or handles the popup selection (however you decide to create the preparation)
2. We add a direct preparation ValueTree to the PREPARATIONS ValueTree
3. We create and set up the AudioProcessor for the direct preparation
4. We add that processor to the AudioProcessorGraph
5. We create a PreparationSection for the direct preparation and add it to the list of things that openGL renders
6. We add the PreparationSection to the ConstructionSite's list of components so that we can continue to work with it
7. OpenGL draws the preparation that is now properly hooked up and ready to use

## Preparation Creation Function Call Trace

1. When we press the 'd' key, [a whole bunch of stuff](#the-applicationcommandtarget) happens [ConstructionSite::perform()](../source/interface/sections/ConstructionSite.cpp#L185) gets called. When we right-click on the ConstructionSite and select 'Direct' from the popup, [ConstructionSite::handlePluginPopup()](../source/interface/sections/ConstructionSite.cpp) gets called
    
    - In both functions, we create a [ValueTree](#the-valuetree) with ID "PREPARATION" and set the properties (type, width, height, and x & y positions)
2. From either the keyPressed() or the handlePluginPopup() function, we call [prep_list.appendChild()](../source/interface/Preparations/PreparationList.h)

   - `prep_list` is the [PreparationList](#the-preparationlist) object in ConstructionSite that keeps track of all the preparations, and its appendChild() function looks like this
   ```
   void appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        this->parent.appendChild(child,undoManager);
    }
   ```
   - `parent` is the PREPARATIONS ValueTree, and `child` is the PREPARATION (note there is no 'S' at the end) ValueTree that we have created for our direct preparation. I won't go down the rabbit hole of functions that gets called from this... Just know that all we had to do was write this one line of code and JUCE will take care of making PREPARATION a child of PREPARATIONS in our ValueTree.
3. When this preparation gets added to the PREPARATIONS ValueTree, the [valueTreeChildAdded()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) function from the [ValueTreeObjectList](#the-valuetreeobjectlist) is listening...
4. This function sees that we're adding a preparation to the ValueTree so it calls [createNewObject()](../source/interface/Preparations/PreparationList.cpp) which has been implemented by our [PreparationList](#the-preparationlist) object.

      - createNewObject() is where we finally create the AudioProcessor for the direct preparation and set it up with the sample rate and buffer size
5. From within createNewObject(), we call SynthBase's [synth.addProcessor()](../source/synthesis/synth_base.cpp), which calls [engine_->addNode()](../source/synthesis/sound_engine/sound_engine.h)

   - This is where we create a node for our direct AudioProcessor and add it to the [AudioProcessorGraph](#the-audioprocessorgraph)
6. Back in step 3, we said that [valueTreeChildAdded()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) was listening to changes in our ValueTree and called createNewObject() when it saw that we had added our direct preparation to the ValueTree. After doing that, it calls [newObjectAdded()](../source/interface/Preparations/PreparationList.cpp), which is another function implemented by our PreparationList object

   - newObjectAdded() says "Hey, who's [listening](#listeners)? Who needs to know if we've added something?" And the ConstructionSite says "Ooo! Me!"
7. From newObjectAdded(), we call the ConstructionSite's [moduleAdded()](../source/interface/sections/ConstructionSite.cpp) function.
8. From moduleAdded(), we call [addSubSection()](../source/interface/sections/synth_section.cpp)

   - This function lives in SynthSection (which ConstructionSite inherits from). It creates a PreparationSection (the GUI component) for our preparation and adds a pointer to it to `sub_sections_` (the vector of SynthSection pointers) that [renderOpenGlComponents()](../source/interface/sections/synth_section.cpp) draws
9. Finally, in [moduleAdded()](../source/interface/sections/ConstructionSite.cpp), after we create the PreparationSection, we add a unique pointer to the PreparationSection to `plugin_components` - the ConstructionSite's vector of unique pointers to PreparationSections. If we didn't do this, we'd lose the subsection once the function finished running and everything will crash.
10. **ALL DONE!** The direct preparation shows up on our screen, and we've added it to all the important places:

- the ValueTree
- the AudioProcessorGraph
- [`objects`](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h): the array of PluginInstanceWrapper pointers in the ValueTreeObjectList
- [`sub_sections_`](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h): the vector of SynthSection pointers in SynthSection
- [`plugin_components`](../source/interface/sections/ConstructionSite.h): the vector of unique pointers to PreparationSections in ConstructionSite

**NICE!**
## Deleting Preparations
There are 3 ways a preparation could be deleted
- press the 'backspace' key on your keyboard with the preparation selected
- press 'CMD + z' on your keyboard
- click Edit > Undo

Let's first walk through how this happens at a high level then we'll dig into some details in a function call trace.

## Preparation Deletion Overview
1. ApplicationCommandHandler registers the selection of Edit > Undo, ApplicationCommandManager registers when you press 'CMD + z' or 'delete' on your keyboard.
2. We remove the direct preparation from the ValueTree
3. We remove it from `objects` - the ValueTreeObjectList's array of PluginInstanceWrapper objects
4. We find the preparation's PreparationSection in `plugin_components` and remove its listener
5. We remove the PreparationSection from `sub_sections_` - the list of things that openGL renders (being careful to do this on the OpenGL thread)
6. We delete the OpenGL components of the PreparationSection and remove the PreparationSection from the ConstructionSite's `plugin_components`
7. We remove the preparation's AudioProcessor from the AudioProcessorGraph
8. Our direct preparation disappears from the screen and is removed from the audio processing.

## Preparation Deletion Function Call Trace
Before tracing, let's think about everything we had to do to create the preparation. To make sure we clean up all of the moving pieces, we need to make sure that we: 1) remove the parameter from the ValueTree, 2) remove the parameter from the AudioProcessorGraph, 3) remove the parameter from `objects`, 4) remove the parameter from `sub_sections_`, and 5) remove the parameter from `plugin_components`. Not only do we need to remove it from all of these things, but we need to make sure that it is deleted as well. Starting from the top...

1a) Deleting with Undo:

   - When we select Edit > Undo, [ApplicationCommandHandler::perform()](../source/common/ApplicationCommandHandler.cpp) will call
      ```c++
      parent->getUndoManager()->undo();
      ```
      This gets the [UndoManager](#the-undomanager) from `parent`, which is a pointer to the SynthGuiInterface, and calls the UndoManager's undo() function 
   - When we press 'CMD + z', [SynthGuiInterface::perform()](../source/common/synth_gui_interface.cpp) will simply call `getUndoManager()->undo()`.
   - In both cases, [UndoManager::undo()](../JUCE/modules/juce_data_structures/undomanager/juce_UndoManager.cpp) is called. This function is implemented by JUCE and takes care of removing our preparation from the ValueTree.

1b) Deleting with 'Backspace' key:

   - When we press the 'Backspace' key, ConstructionSite::perform() calls `prep_list.removeChild(prep->state, &undo)`, which looks like this
       ```c++
       void removeChild (juce::ValueTree& child, juce::UndoManager* undoManager)
       {
           undoManager->beginNewTransaction();
           this->parent.removeChild(child,undoManager);
       }
       ```
   - Side note: Since we've passed the UndoManager into this function and called `undoManager->beginNewTransaction()`, if someone deletes their preparation on accident, they can undo to get it back!
   - Since `parent` is a ValueTree, this calls JUCE's ValueTree::removeChild() which takes care of removing our preparation from the ValueTree.
   
   **Great! In all 3 ways of deleting a preparation, we've removed from the ValueTree - 1 of 5 removal steps accomplished**
2. When our preparation is removed from the ValueTree, the [valueTreeChildRemoved()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) function from the [ValueTreeObjectList](#the-valuetreeobjectlist) is listening...
   - In this function, we call
      ```c++
      o = objects.removeAndReturn (oldIndex);
      ```
      where `objects` is an array of pointers to [PluginInstanceWrapper](#the-plugininstancewrapper) objects and removeAndReturn() is a juce_Array function that returns a pointer to the preparation (PluginInstanceWrapper) that we're removing.
   - **We've removed from `objects` - 2 of 5 removal steps accomplished**
4. Then it calls [deleteObject()](../source/interface/Preparations/PreparationList.cpp) which has been implemented by our [PreparationList](#the-preparationlist) object.
   - deleteObject() says "Hey, who's listening? Who needs to know if we've removed something?" And the ConstructionSite says "Ooo! Me!"
5. From deleteObject(), we call the ConstructionSite's [removeModule()](../source/interface/sections/ConstructionSite.cpp) function.

   - Here, we go through `plugin_components`, find the PreparationSection with the same ID as the PluginInstanceWrapper that we’re removing.
   - Now that we have the PreparationSection, we have to remove the preparation's listener with the following code
   ```c++
    preparationSelector.getLassoSelection().removeChangeListener (plugin_components[index].get());
   ```
   - If any preparations have open popups, we close them with this code
   ```c++
    interface->getGui()->mod_popup->reset();
    interface->getGui()->prep_popup->reset();
    ```
   - Finally, we delete any cables attached to our preparation with the following code
    ```c++
    cableView.deleteConnectionsWithId(wrapper->node_id);
    ```
   - Next, we need to remove the PreparationSection from both `sub_sections_` and `plugin_components`.
6. From removeModule(), we call [removeSubSection()](../source/interface/sections/synth_section.cpp) to remove the PreparationSection from `sub_sections_`.

   - This function lives in SynthSection (which ConstructionSite inherits from) and removes the PreparationSection from `sub_sections_` (the vector of SynthSection pointers that [renderOpenGlComponents()](../source/interface/sections/synth_section.cpp) draws) by calling 
   ```c++
   sub_sections_.erase(location);
   ```
   - But this isn't 100% right yet. Since OpenGL uses `sub_sections_` to render things, we need to make sure that manipulating `sub_sections_` happens on the OpenGL thread, which looks like this
   ```c++
    {
        juce::ScopedLock lock(open_gl_critical_section_);
        removeSubSection (plugin_components[index].get());
    }
   ```
   - If you want to know more about what this is doing, check out the section I've written about [locking](#locking).
   - **We've removed from `sub_sections_` - 3 of 5 removal steps accomplished**
7. Now we go back to the ConstructionSite's [removeModule()](../source/interface/sections/ConstructionSite.cpp) and remove our parameter's PreparationSection from `plugin_components`.

   - If simply try to remove the pointer from the vector, we run into an error. Before we can remove the PreparationSection pointer from `plugin_component`, we need to destroy the OpenGL components (the buffer, shader, and texture) of our PreparationSection like so
      ```c++
      plugin_components[index]->destroyOpenGlComponents (open_gl);     
      plugin_components.erase(plugin_components.begin()+index);
      ```
   - But wait! Deleting OpenGL components should be happening on the OpenGL thread, so why aren't we locking like we did in step 7? It's already taken care of for us in the destroyOpenGlComponents() function.
   - **We've removed from `plugin_components` - 4 of 5 removal steps accomplished**
   - Now, if we build and run, create a direct preparation, and click Edit > Undo, it works! The direct preparation disappears! But we have one removal step left: Removing our preparation from the AudioProcessorGraph.
8. Back in step 4 of the deletion process, we were in the PreparationList's [deleteObject()](../source/interface/Preparations/PreparationList.cpp) function. After it calls the ConstructionSite's removeModule() function, we call SynthBase's [synth.removeProcessor()](../source/synthesis/synth_base.cpp), which calls [engine_->removeNode()](../source/synthesis/sound_engine/sound_engine.h)
   - removeNode() is a JUCE function that takes care of removing the node for our preparation from the [AudioProcessorGraph](#the-audioprocessorgraph)
   - **We've removed from the AudioProcessorGraph - 5 of 5 removal steps accomplished**
9. We might think we're done, but we have one more bug to take care of. If we create a direct preparation and undo, it appears to work just fine. If we create two direct preparation and undo, both of our direct preparations disappear. To fix this, we modify the PreparationList's [appendChild()](../source/interface/Preparations/PreparationList.h) function like so
   ```c++
    void appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        undoManager->beginNewTransaction();
        this->parent.appendChild(child,undoManager);
    }
   ```
   - This just tells the UndoManager that appending a Child is something that can be undone
10. **ALL DONE!** Now when we undo, the direct preparation disappears our screen, and we've removed it from all the important places:

- the ValueTree
- the AudioProcessorGraph
- [`objects`](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h): the array of PluginInstanceWrapper pointers in the ValueTreeObjectList
- [`sub_sections_`](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h): the vector of SynthSection pointers in SynthSection
- [`plugin_components`](../source/interface/sections/ConstructionSite.h): the vector of unique pointers to PreparationSections in ConstructionSite

**NICE!**

# Creating and Deleting Cables
## Creating Cables
Let's say you create a direct preparation and a keymap preparation. You can connect them by clicking on the port of one and dragging to the port of the other. This creates a "cable line" - the thick black lines that connect preparations. Here's a brief overview of how we create a cable line

1. JUCE recognizes the mouse up and calls BKPort's mouseUp() function. 
2. BKPort goes through its listeners on mouseUp and calls PreparationSection's endDraggingConnector()
3. PreparationSection's endDraggingConnector() likewise goes through its listeners on endDraggingConnector() and calls CableView's endDraggingConnector()
4. CableView's endDraggingConnector() figures out some stuff and adds the CONNECTION value tree to the CONNECTIONS value tree
5. valueTreeChildAdded() is listening (as we remember from adding preparations!) and calls createNewObject()
6. CableView's createNewObject() function does a few things for us

    - creates a new Cable
    - adds the new Cable as a childComponent to the CableView
    - adds the Cable OpenGL component to the SynthSection
    - makes the Cable component visible on the OpenGL thread
    - associates the CONNECTION value tree we just made with the Cable we just made
7. Next, valueTreeChildAdded() calls CableView's newObjectAdded() function, which adds the connection to the AudioProcessorGraph

## Deleting Cables
Currently, there are two ways to delete a cable: 1) press 'CMD + z' immediately after making the connection, or 2) delete one of the preparations it's connected to.

When you draw a cable then press 'CMD + z', here's what happens...
1. When we press 'CMD + z', [SynthGuiInterface::perform()](../source/common/synth_gui_interface.cpp) will simply call [UndoManager::undo()](../JUCE/modules/juce_data_structures/undomanager/juce_UndoManager.cpp), which is implemented by JUCE and takes care of removing our connection from the ValueTree.
2. When our connection is removed from the ValueTree, the [valueTreeChildRemoved()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) function from the [ValueTreeObjectList](#the-valuetreeobjectlist) is listening...
3. It calls [deleteObject()](../source/interface/Preparations/PreparationList.cpp) which has been implemented by our [CableView](#the-cableview) object.
    - deleteObject() removes the connection from the AudioProcessorGraph and deletes the OpenGL components
4. The last thing we need to do to ensure undo functionality is begin a new undo transaction to the CableView's [endDraggingConnector()](../source/interface/sections/CableView.cpp) function right before the CONNECTION value tree is added to the CONNECTIONS value tree like so...
    ```c++
    void CableView::endDraggingConnector (const juce::MouseEvent& e)
    {
        // ...
        undoManager.beginNewTransaction();
        parent.appendChild(_connection, &undoManager);
    }
    ```
When we delete a cable by deleting a preparation that it's attached to, we follow all of the same steps outlined earlier in [Preparation Deletion Function Call Trace](#preparation-deletion-function-call-trace). In step 4, we call
```c++
cableView.deleteConnectionsWithId(wrapper->node_id);
```
from the ConstructionSite's removeModule() function. CableView's deleteConnectionsWithId() function simply goes through the list of cable connections and deletes any cables whose source or destination preparations match the preparation that is being deleted.
```c++
void CableView::deleteConnectionsWithId(juce::AudioProcessorGraph::NodeID delete_id)
{
    for (auto connection : objects){
        if (connection->src_id == delete_id || connection->dest_id == delete_id){
            parent.removeChild (connection->state, &undoManager);
        }
    }
}
```
Note that we're using CachedValues to get the Cable's `src_id` and `dest_id`. I originally had the referTo() functions in the wrong place. They're supposed to live in Cable's [setValueTree()](../source/interface/components/Cable/Cable.h) function, which is where they are now.

# All Things Modulation
Let's say you have a direct preparation. That direct preparation has a resonance parameter that you want to modulate. Here's how you would do that:
1. Create a Modulation Preparation by pressing 'c' on your keyboard or right-clicking in the ConstructionSite and selecting 'Modulation'.
2. Connect the Modulation Preparation to the Direct Preparation by dragging the Modulation on top of the Direct. You'll see a thin "Modulation Line" appear between the two preparations
3. Double-click on the Modulation Preparation, and the ModulationModuleSection will appear.
4. Right click in the ModulationModuleSection and we have the option to create either a RampModulator or a StateModulator. In this example, the Direct's resonance parameter will use a ramp modulator, so we select that.
5. In the top left corner of the RampModulator, we can click and drag a modulator button onto the Direct parameter that we want to modulate.

Let's take a deeper look at how this is being implemented. Creating the Modulation Preparation is easy - it's the same as any other preparation. The creation process is explained in the [Preparation Creation Function Call Trace](#preparation-creation-function-call-trace).

## Creating a Modulation Connection Between Preparations (MODCONNECTION)
Creating a Modulation Connection is similar to creating a Cable connection. It's actually a bit simpler since you only draw the ModulationLine after connecting the preparations (whereas the Cable is drawn before you connect it to anything). It's also a bit simpler because you don't actually hook anything up to the AudioProcessorGraph until you start modulating parameters.
1. Once you drop the Modulation onto the Direct, JUCE recognizes the mouse up and after going through a chain of listeners, PreparationSection's itemDropped() function gets called
2. PreparationSection's itemDropped() likewise goes through its listeners and since we've dropped a Modulation, it calls ModulationLineView's modulationDropped() function 
3. ModulationLineView's modulationDropped() figures out some stuff and adds the MODCONNECTION value tree to the CONNECTIONS value tree
4. valueTreeChildAdded() is listening (as we remember from adding preparations!) and calls createNewObject()
5. ModulationLineView's createNewObject() simply creates a new ModulationLine
6. The ModulationLine's constructor takes care of drawing the line between the preparations
7. Next, valueTreeChildAdded() calls ModulationLineView's newObjectAdded() function, which only does something if you're loading a preset (it adds the parameter modulation connections)

## Deleting a Modulation Connection Between Preparations (MODCONNECTION)
Currently, there are two ways to delete a ModulationLine: 1) press 'CMD + z' immediately after making the connection, or 2) delete one of the preparations it's connected to.

When you draw a ModulationLine then press 'CMD + z', here's what happens...
1. When we press 'CMD + z', [SynthGuiInterface::perform()](../source/common/synth_gui_interface.cpp) will simply call [UndoManager::undo()](../JUCE/modules/juce_data_structures/undomanager/juce_UndoManager.cpp), which is implemented by JUCE and takes care of removing our connection from the ValueTree.
2. When our connection is removed from the ValueTree, the [valueTreeChildRemoved()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) function from the [ValueTreeObjectList](#the-valuetreeobjectlist) is listening...
3. It calls [deleteObject()](../source/interface/sections/ModulationLineView.cpp) which has been implemented by our ModulationLineView object.
    - deleteObject() simply deletes the OpenGL components of our ModulationLine
4. The last thing we need to do to ensure undo functionality is begin a new undo transaction to the ModulationLineView's [modulationDropped()](../source/interface/sections/ModulationLineView.cpp) function right before the MODCONNECTION value tree is added to the CONNECTIONS value tree

When we delete a ModulationLine by deleting a preparation that it's attached to, we follow all of the same steps outlined earlier in [Preparation Deletion Function Call Trace](#preparation-deletion-function-call-trace). In step 4, we call
```c++
    modulationLineView.deleteConnectionsWithId(wrapper->node_id);
```
from the ConstructionSite's removeModule() function. ModulationLineView's deleteConnectionsWithId() function simply goes through the list of modulation connections and deletes any ModulationLines whose source or destination preparations match the preparation that is being deleted.
```c++
void ModulationLineView::deleteConnectionsWithId(juce::AudioProcessorGraph::NodeID delete_id)
{
    for (auto connection : objects){
        if (connection->src_id == delete_id || connection->dest_id == delete_id){
            parent.removeChild (connection->state, &undoManager);
        }
    }
}
```
Note that we're using CachedValues to get the ModulationLine's `src_id` and `dest_id`. The referTo() functions live in the ModulationLine constructor.

## Creating a RampModulator (or StateModulator)
When you right-click in the ModulationModuleSection, you have the option to create a RampModulator or a StateModulator. In our example, we select a RampModulator. Here's everything that goes into creating our RampModulator:
1. ModulationModuleSection's handlePopupResult() adds a modulationproc value tree to the PREPARATION value tree for the Modulation Preparation.
2. When this processor gets added to the PREPARATION ValueTree, the [valueTreeChildAdded()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) function from the [ValueTreeObjectList](#the-valuetreeobjectlist) is listening... this function sees that we're adding a preparation to the ValueTree so it calls createNewObject() which has been implemented by our ModulationList object.

    - This function creates a ModulatorBase (specifically a RampModulator ModulatorBase) and passes it as an argument to the ModulationProcessor's addModulator() function. This ModulationProcessor was created when we created our ModulationPreparation (see step 4 in the [Preparation Creation Function Call Trace](#preparation-creation-function-call-trace)). 
3. ModulationProcessor's addModulator() function sets the ModulatorBase's parent and adds the ModulatorBase to the `modulators_` vector.
4. After calling createNewObject(), the ValueTreeObject list calls newObjectAdded(), which calls the ModulationModuleSection's modulatorAdded() function.

    - Here, we create a ModulationSection - the GUI component for our RampModulator
    - Then we call [addSubSection()](../source/interface/sections/synth_section.cpp), which lives in SynthSection (we can access this because ModulationModuleSection inherits from ModulesInterface, which creates a ModulesContainer that inherits from SynthSection). It adds a pointer to our ModuleSection to `sub_sections_` (the vector of SynthSection pointers) that [renderOpenGlComponents()](../source/interface/sections/synth_section.cpp) draws

## Deleting a RampModulator (or StateModulator)
Currently, there are two ways to delete a a ModulatorBase: 1) press 'CMD + z' immediately after creating it, or 2) click the 'X' in the top right corner of the ModuleSection for the modulator.

1a) Deleting with Undo:

- When we press 'CMD + z', [SynthGuiInterface::perform()](../source/common/synth_gui_interface.cpp) will simply call [UndoManager::undo()](../JUCE/modules/juce_data_structures/undomanager/juce_UndoManager.cpp), which is implemented by JUCE and takes care of removing our connection from the ValueTree.

1b) Deleting with 'X' button:

- After calling a sequence of listeners, ModulationSection's butttonClicked() function gets called. This makes the ModulationSection invisible then removes the modulationproc from the ValueTree
2. When our modulationproc is removed from the ValueTree, the [valueTreeChildRemoved()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) function from the [ValueTreeObjectList](#the-valuetreeobjectlist) is listening... and calls ModulationList's deleteObject() function, which calls the ModulationProcessor's removeModulator() function.
3. removeModulator() sets info relevant to the ModulatorBase to null so that deallocation doesn't happen on the audio thread.
4. Next, deleteObject() calls ModulationModuleSection's removeModulator(), which removes our ModulationSection from `modulation_sections_` and removes it from the GUI interface (while on the OpenGL thread of course).

## Creating Modulation Connections to Parameters (ModulationConnection)
When we want to modulate a parameter, we drag the ModulationButton from the ramp modulator onto one of our Direct parameters. When we drag the ModulationButton and hover over a Direct parameter, two components will appear: an outline around the Direct parameter knob and a small knob below it. Once we release it on top of a parameter, we can change the amount that the modulation will affect the parameter using the small knob. changes are reflected in the outline around the Direct knob. Here's how this is working in the background...

1. Once you start dragging the ModulationButton, the ModulationButton's mouseDrag() function calls the ModulationManager's modulationDragged() function.

    - This function gets whatever component is beneath the mouse and passes it as an argument to modulationDraggedToComponent() (also within the ModulationManager).
2. modulationDraggedToComponent() uses conditionals to figure out whether it's a ramp or state modulator and after maintaining/changing some relevant variables, it calls connectModulation() with the source_name (ModulationButton) and destination (the Direct parameter to be modulated) as arguments.
3. ModulationManager's connectModulation() calls SynthGuiInterface's connectModulation(), which calls SynthBase's connectModulation().

   - SynthBase has three connectModulation() functions. This one takes strings as arguments and checks if we already have a connection to the component we're hovering over. If not, we create a connection using the source and destination strings and use it to call the connectModulation() function that takes a ModulationConnection as an argument.
4. In this other connectModulation() function, we do most of the heavy lifting.

   - We get the value trees for the modulation preparation (the source of our modulation connection) and the direct preparation (the destination of our modulation connection) and use these to get their respective nodes from the audio processor graph
   - Then we populate the ModulationConnection that we passed into the function with the backend information (processor and bus stuff)
   - If everything is good and well, add the ModulationConnection to `mod_connections_` and add its value tree (ModulationConnection) as a child of the MODCONNECTION value tree
   - After that, we make sure the ramp modulator's processor is hooked up correctly with a call to the ModulationProcessor's addModulationConnection() function
   - Finally, we connect the modulation processor to the preparation parameter in the audio processor graph
5. Once a connection is made, the ModulationManager's modulationDraggedToComponent() function draws the modulation knob and outline.

## Deleting Modulation Connections to Parameters (ModulationConnection)
There are a couple ways for a ModulationConnection to be deleted: 1) Right-click on the wheel and select 'Remove', 2) Press 'CMD-z' after creating it, and 3) deleting the entire modulation preparation. Here's how all of these options work.

**Deleting with right-click > Remove:**
1. When you select 'Remove' from the popup menu, a listener calls the ModulationManager's disconnectModulation() function, which calls removeModulation with the ModulationConnection's source and destination names
2. This does some stuff then calls SynthGuiInterface's disconnectModulation(), which just calls SynthBase's disconnectModulation(), which calls a different disconnectModulation() in SynthBase. This one takes a connection as an argument and removes it from `mod_connections_`. Then it removes the connection from the ModulationProcessor and AudioProcessorGraph before removing the connection's value tree from MODCONNECTION.
3. Going back to the ModulationManager's removeModulation() function... after it calls disconnectModulation(), it calls its own modulationsChanged() function which takes care of removing the GUI components.

1b) Deleting with 'CMD + z':

1. When we press 'CMD + z', [SynthGuiInterface::perform()](../source/common/synth_gui_interface.cpp) will simply call [UndoManager::undo()](../JUCE/modules/juce_data_structures/undomanager/juce_UndoManager.cpp), which is implemented by JUCE and takes care of removing our connection from the ValueTree.
2. Once the ModulationConnection is removed from the value tree, SynthBase is listening and its valueTreeChildRemoved() function gets called. This function calls disconnectModulation() and passes the value tree that got removed.
3. This disconnectModulation() gets the connection associated with the value tree and calls the other disconnectModulation() function. This one takes a connection as an argument and removes it from `mod_connections_`. Then it removes the connection from the ModulationProcessor and AudioProcessorGraph before removing the connection's value tree from MODCONNECTION.
4. Once this all completes successfully, valueTreeChildRemoved() calls SynthGuiInterface's notifyModulationsChanged() to remove the GUI components.

1c) Deleting with deletion of the modulation preparation:


# Undoing Preparation Dragging
Here's what happens when you drag and undo:

Our ConstructionSite inherits from JUCE's DragAndDropContainer class and PreparationSection inherits from JUCE's DragAndDropTarget class. When we drag and drop a preparation somewhere, JUCE takes care of calling ConstructionSite's [dragOperationEnded()](../source/interface/sections/ConstructionSite.cpp) function.

From within dragOperationEnded(), we begin a new undo transaction and set the PreparationSection's point property to be wherever the "mouse up" occurred like so

```c++
void dragOperationEnded (const juce::DragAndDropTarget::SourceDetails& source)
{
    // ...
    fc->undo.beginNewTransaction();
    fc->curr_point = mouse_drag_position_;
    // ...
}
```

Note that curr_point is a `juce::CachedValue<juce::Point<int>>` object as defined in the PreparationSection [header file](../source/interface/Preparations/PreparationSection.h). In the PreparationSection [source file](../source/interface/Preparations/PreparationSection.cpp), we call `curr_point.referTo(state,IDs::x_y,&undo)`, which allows us to access the point property of `state` (which is the value tree associated with this PreparationSection).

The PreparationSection has a [valueTreePropertyChanged()](../source/interface/Preparations/PreparationSection.h) function that is listening... when we change the CONNECTION value tree, PreparationSection sets its center position to equal the point where the user releases their mouse.

**Nice!** This works! Dragging a preparation to a new location then pressing 'CMD + z' returns the preparation to its original position!

This took some refactoring and finagling. We had to change the x and y properties of our preparation value trees to be a juce::Point objects since we couldn't change both the x and y properties with one undo transaction without defining our own UndoableAction. This meant replacing `DECLARE_ID(x)` and `DECLARE_ID(y)` with `DECLARE_ID(x_y)` in the Identifiers [header file](../source/common/Identifiers.h). We also had to change everywhere we set the x and y properties of the value trees. And finally, we created a struct in tracktion's ValueTreeUtilities [header file](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) to convert a juce::Point to a juce::Var
```c++
template <>
struct VariantConverter<juce::Point<int>>
{
    static Point<int> fromVar (const var& v)
    {
        auto parts = StringArray::fromTokens (v.toString(), ",", "");
        if (parts.size() == 2)
        {
            int x = parts[0].getIntValue();
            int y = parts[1].getIntValue();
            return { x, y };
        }
    }
    static var toVar (const Point<int>& p) { return p.toString(); }
};
```
# The ApplicationCommandTarget
In our codebase, some of our classes - currently SynthGuiInterface and ConstructionSite - inherit from JUCE's ApplicationCommandTarget class (documentation [here](https://docs.juce.com/master/classApplicationCommandTarget.html)). By inheriting ApplicationCommandTarget, the ApplicationCommandManager knows to check our classes when certain commands are called. Below, I've included the functions that our classes need to implement and the way that we did so in the ConstructionSite

- **getNextCommandTarget()** - returns the next target to try after this one
   ```c++
    ApplicationCommandTarget* getNextCommandTarget() override {
        return findFirstTargetParentComponent();
    }
   ```
- **getAllCommands()** - adds the commandIDs to the ApplicationCommandManager's commands array.
   ```c++
   enum CommandIDs {
       direct = 0x0613,
       nostalgic = 0x0614,
       keymap = 0x0615,
       resonance = 0x0616,
       synchronic   = 0x0617,
       blendronic = 0x0618,
       tempo = 0x0619,
       tuning = 0x0620,
       modulation = 0x0621,
       deletion = 0x0622
   };
   
   void ConstructionSite::getAllCommands(juce::Array<juce::CommandID> &commands) {
       commands.addArray({direct, nostalgic, keymap, resonance, synchronic, blendronic, tempo, modulation, deletion});
   }
   ```
  The commandIDs can be whatever number you want
- **getCommandInfo()** - associates the commandID with a name, description, category, and default key press
   ```c++
   void ConstructionSite::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo &info) {
     switch (id) {
         case direct:
             info.setInfo("Direct", "Create Direct Preparation", "Edit", 0);
             info.addDefaultKeypress('d', juce::ModifierKeys::noModifiers);
         break;
         case nostalgic:
             info.setInfo("Nostalgic", "Create Nostalgic Preparation", "Edit", 0);
         info.addDefaultKeypress('n', juce::ModifierKeys::noModifiers);
         break;
         // info for the rest of the commands...
      }
   }
   ```
- **perform()** - holds the code that is supposed to be executed once the command is triggered
   ```c++
   bool ConstructionSite::perform(const InvocationInfo &info) {
     switch (info.commandID) {
         case direct:
         {
             juce::ValueTree t(IDs::PREPARATION);
             t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeDirect, nullptr);
             t.setProperty(IDs::width, 245, nullptr);
             t.setProperty(IDs::height, 125, nullptr);
             t.setProperty(IDs::x, lastX - 245 / 2, nullptr);
             t.setProperty(IDs::y, lastY - 125 / 2, nullptr);
             // prep_list.appendChild(t,  interface->getUndoManager());
             prep_list.appendChild(t,  &undo);
             return true;
         }
         // implementations for the rest of the commands...
      }
   }
   ```
Once these are implemented, ConstructionSite and SynthGuiInterface have done everything that they need to do to be ApplicationCommandTargets! Now we need an [ApplicationCommandManager](https://docs.juce.com/master/classApplicationCommandManager.html) to take care of the commands, so we create that in SynthGuiInterface and pass it around. Note that an ApplicationCommandManager cannot be copied!

So, to get the command manager from SynthGuiInterface to the ConstructionSite, we pass it as a constructor argument for the FullInterface, MainSection, and finally the ConstructionSite. In the constructor of both SynthGuiInterface and ConstructionSite (and any ApplicationCommandTarget), we need to tell the command manager to register the commands in this target like so
```c++
commandManager.registerAllCommandsForTarget (this);
```
Nice! Now everything should be working! A quick trace of what happens when we run bitKlavier:

1. SynthGuiInterface creates FullInterface creates MainSection creates ConstructionSite, passing its ApplicationCommandManager the whole way down.
2. Once we're in the ConstructionSite constructor, we call commandManager.registerAllCommandsForThisTarget()
3. This calls getAllCommands() on our ConstructionSite, which adds the ConstructionSite's list of commandIDs to the ApplicationCommandManager's list of commandIDs
4. From registerAllCommandsForThisTarget() we also call the ConstructionSite's [getCommandInfo()](../source/interface/sections/ConstructionSite.cpp) which sets up the ApplicationCommandInfo object for each CommandID.
5. The command manager registers each ApplicationCommandInfo object and then returns
6. Once we're done constructing the ConstructionSite, we crawl back up to the SynthGuiInterface's constructor where we call commandManager.registerAllCommandsForThisTarget(). This repeats steps 3-5 for the SynthGuiInterface.
7. Finally, we press 'd' and whole bunch of people start talking to each other. ComponentPeer::handleKeyPress tells KeyPressMappingSet tells ApplicationCommandManager tells ApplicationCommandTarget tells ConstructionSite::perform to do whatever we should be doing when 'd' is pressed. And it does! Our direct preparation shows up!
8. Next, we press 'CMD + Z', and all the same people start talking to each other but this time ApplicationCommandTarget tells SynthGuiInterface::perform to do whatever we should do when 'CMD + Z' is pressed.

# Important Pieces of The Puzzle
If you read through function call traces and you had a hard time keeping track of all the moving pieces... me too. So, here's some more information about the important parts of the puzzle, namely:

- [The ValueTree](#the-valuetree)
- [The ValueTreeObjectList](#the-valuetreeobjectlist)
- [The PluginInstanceWrapper](#the-plugininstancewrapper)
- [The PreparationList](#the-preparationlist)
- [The UndoManager](#the-undomanager)
- [The CableView](#the-cableview)
- [The AudioProcessorGraph](#the-audioprocessorgraph)
- [Locking](#locking)
- [Listeners](#listeners)
- Sections?

## The ValueTree
The ValueTree is implemented by JUCE and provides a whole lot of useful functionality for us. The creators explain it best, so you can read more about it [here](https://juce.com/tutorials/tutorial_value_tree/). A ValueTree is made up of other ValueTrees, and our main ValueTree gets created in [SynthBase](../source/synthesis/synth_base.cpp), which is the start of our backend stuff. There, we create a ValueTree with ID "GALLERY" then create another ValueTree with ID "PIANO". We make the PIANO ValueTree a child of the GALLERY ValueTree. Then we make three more ValueTrees that are all children of the "PIANO" ValueTree. Once we're done setting it up, it looks like this
``` xml
<GALLERY>
   <PIANO>
      <PREPARATIONS/>
      <CONNECTIONS/>
      <MODCONNECTIONS/>
   </PIANO>
</GALLERY>
```
For creating and deleting preparations, you just need to concern yourself with the PREPARATIONS ValueTree. This is because anytime you want to create a preparation, you need to create a ValueTree for the preparation and add it as a child of the PREPARATIONS ValueTree like so
``` xml
<GALLERY>
   <PIANO>
      <PREPARATIONS>
         <PREPARATION type=”” width=”” height=”” x=”” y=”” … numOuts=”” />
      </PREPARATIONS> 
      <CONNECTIONS/>
      <MODCONNECTIONS/>
   </PIANO>
</GALLERY>
```
## The ValueTreeObjectList
The ValueTreeObjectList is a class created by Tracktion that facilitates responding to changes in the Value Tree. You can read the documentation [here](https://tracktion.github.io/tracktion_engine/classtracktion_1_1engine_1_1ValueTreeObjectList.html). In general, the ValueTreeObjectList listens to the value tree and implements any necessary functions. This class has a few virtual void functions, which means we have to write our own versions of these functions. Such functions include newObjectAdded(), objectRemoved(), etc. We implement these functions in the [PreparationList](#the-preparationlist) class.

## The PluginInstanceWrapper
The [PluginInstanceWrapper](../source/interface/Preparations/PreparationList.h) is a class that we have created to bundle all of the important information for an instance of a plugin (a preparation in our case). The member variables are:

- a pointer to a JUCE AudioProcessor
- a node ID for a JUCE AudioProcessorGraph
- a JUCE ValueTree

You can simply think of this as the parameter (with all of its important information)

## The PreparationList
The [PreparationList](../source/interface/Preparations/PreparationList.h) is a class that we have created to keep track of all of the preparations. It has an array of [PluginInstanceWrappers](#the-plugininstancewrapper) called `objects` that holds all of the preparations. It also has a value tree called `parent`, which is the PREPARATIONS value tree. Any preparation that we add will be a child of PREPARATIONS.

Whenever the value tree is changed, the PreparationList will notice (it's always [listening](#listeners)...) and it will update any variables and run any functions that it needs to.

How is our PreparationList always listening? And why don't I see `objects` or `parent` in the PreparationList files? This is because PreparationList inherits from [tracktion::engine::ValueTreeObjectList\<PluginInstanceWrapper>](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h), which is a third-party class called the [ValueTreeObjectList](#the-valuetreeobjectlist). Our PreparationList is like an extension of the [ValueTreeObjectList](#the-valuetreeobjectlist) that makes their awesome functions work on our specific project. We just have to implement our versions of certain ValueTreeObjectList functions, such as newObjectAdded() and objectRemoved() in PreparationList to make sure everything works correctly.

The PreparationList object is created in SynthBase and is passed into the ConstructionSite as part of the SynthGuiData. The PreparationList only lives in the ConstructionSite (no where else) as `prep_list`.

## The UndoManager
One of the convenient things about using JUCE's value tree is that we get access to undo/redo functions implemented by their UndoManager. You can read the documentation [here](https://docs.juce.com/master/classUndoManager.html).

Like our top-level value tree, the UndoManager gets created in [SynthBase](../source/synthesis/synth_base.cpp), which is the start of our backend stuff. This UndoManager gets passed around all over the place. We first use it in ConstructionSite, but how does it get there? Buckle up... ConstructionSite receives it as an argument when it is constructed in MainSection. Similarly, MainSection receives it as an argument when it is constructed in FullInterface. FullInterface is constructed in SynthGuiInterface with a single argument: `SynthGuiData* synth_data`. The UndoManager is lumped into that SynthGuiData object, which holds the SynthBase where the UndoManager was created.

## The CableView
The [CableView](../source/interface/sections/CableView.cpp) is a class that we have created to keep track of all of the cable connections. It has an array of Cables called `objects` that holds the cable connections. It also has a value tree called `parent`, which is the CONNECTIONS value tree. Any connection that we add will be a child of CONNECTIONS.

Whenever the value tree is changed, the CableView will notice (it's always [listening](#listeners)...) and it will update any variables and run any functions that it needs to.

How is our CableView always listening? And why don't I see `objects` or `parent` in the CableView files? This is because CableView inherits from [tracktion::engine::ValueTreeObjectList\<PluginInstanceWrapper>](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h), which is a third-party class called the [ValueTreeObjectList](#the-valuetreeobjectlist). Our CableView is like an extension of the [ValueTreeObjectList](#the-valuetreeobjectlist) that makes their awesome functions work on our specific project. We just have to implement our versions of certain ValueTreeObjectList functions, such as newObjectAdded() and objectRemoved() in CableView to make sure everything works correctly.

The CableView object is created in the ConstructionSite and only exists in the ConstructionSite (no where else) as `cableView`.

An astute observer will realize that I copied and pasted the explanation of [PreparationList](#the-preparationlist) and tweaked the names and files to fit for CableView. This is because the CableView is to cable connections as PreparationList is to preparations. We just don't call it CableList because it also takes care of the GUI elements of the cables.

## The AudioProcessorGraph
## Locking
## Listeners
## Sections
