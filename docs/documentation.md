# ElectroSynth

## Saving and Loading Presets
So you've created some cool things in ElectroSynth and you want to save them for later... Here's how saving and loading your presets works behind the scenes.

### Saving Presets
When you click 'File > Save' or 'CMD + s' both of these will call openSaveDialog() in [SynthGuiInterface](../source/common/synth_gui_interface.cpp). This creates and launches the FileChooser with the correct flags. 

## Creating an Effect
On the left-hand side of ElectroSynth, there are 3 effect lanes. Currently, you can only add a filter to these lanes. Let's change that. Here's everything that needs to happen to create a delay effect.

At its core, the delay effect is implemented by [Jeff's LEAF Code](../third_party/leaf_modules/LEAF/leaf/Src/leaf-delay.c). Since LEAF Code is written in C and our ElectroSynth application is in C++, we create a [DelayModule](../third_party/leaf_modules/modules/DelayModule.cpp) that acts as our interface between languages.

This DelayModule lives in our third_party files and not in our source files. In order to use the DelayModule for audio things in our JUCE application, we create a [DelayModuleProcessor](../source/synthesis/framework/Processors/DelayModuleProcessor.cpp).

We'll talk a bit more about the details of these later. For now, just know that these are the big 3 components of our delay effect: the LEAF Delay, the DelayModule, and the DelayModuleProcessor.

To make sure that everything is hooked up correctly, here are the places you need to change things:
1. module_type_info.h: TypeMap has a bunch of TypeValue pairs
2. funcmaps.cpp module_init_map[]
3. ModuleList adds the factory template to create things
4. defs.h ModuleType: ModuleTypeDelayModule
5. leaf_modules/CMakeLists.txt
6. factory.template for the DelayModuleProcessor in ModuleList.cpp

### Effect Creation Walkthrough
Now that all of our files exist and are hooked up properly, let's take a look at how they all come together to create the delay an ElectroSynth user experiences. When a user right-clicks on an effect lane and selects "add delay," here's what happens.

### Brief Overview
1. EffectModuleSection creates EFFECTMODULE value tree and adds to `list` (ModuleList of ProcessorBases that lives in ModulesInterface)
2. Changing the ValueTree creates a DelayModuleProcessor, which gets added to its respective `effects` vector in SynthBase and sets it up with the sample rate and buffer size
3. Changing the ValueTree also creates the corresponding ModuleSection (GUI component) for the Delay and adds it to the SynthSection's `sub_sections_`.

Creating the Processor for the Delay sets up the backend for the audio things and also determines what the GUI will look like

### Function Call Trace
1. EffectModuleSection's [handlePopupResult()](../source/interface/sections/EffectsModuleSection.cpp) function sees that you selected "add delay" and creates an EFFECTMODULE value tree that gets added to `list`, which is a ModuleList that holds all of our ProcessorBases. It's associated with the EFFECTS value tree.
2. Since our ModuleList is a ValueTreeObjectList, it listens for changes on the EFFECTS value tree. And since we just added an EFFECTMODULE to it, valueTreeChildAdded() gets called and does a few things... 
3. Here we call EffectList's [createNewObject()](../source/interface/sections/EffectsModuleSection.cpp) function, which creates the DelayModuleProcessor (using the factory.template we define in [ModuleList.cpp](../source/interface/sections/ModuleList.cpp)) and adds the processor to the SynthBase (which calls prepareToPlay() on the processor and adds the processor to the effects vector for that lane)
4. Then we call the ModuleList's [newObjectAdded()](../source/interface/sections/ModuleList.cpp), which calls moduleAdded on our listeners, which is just the [EffectModuleSection](../source/interface/sections/EffectsModuleSection.cpp). The moduleAdded() function adds the module section to the container