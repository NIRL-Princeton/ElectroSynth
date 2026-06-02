# What is ElectroSynth?
ElectroSynth is a VST for synthesis. You can create oscillators, filters, LFOs, envelopes, etc and connect them together to make your sounds. Jeff has already written the implementations for these components, which live in the LEAF library. The algorithms used for a filter, for example, can be found here [here](../third_party/leaf_modules/LEAF/leaf/Src/leaf-filters.c).

Once you have a sound that you like in ElectroSynth, you can export it as a preset and load it onto your Electrosteel or Electrobass. This means that you can make your instrument sound however you'd like and then play it without being hooked up to your computer! You can also open ElectroSynth as a plugin in your DAW and record the sounds that you've created there.

## How to create a preset in ElectroSynth

When you first open ElectroSynth, you'll see that the screen is sectioned into 4 parts. Most of the screen is taken up by a dark section called the AudioChainSection. This is where you can add processors (oscillators, filters, strings). It could be a single oscillator or a combination (summation) of oscillators, strings, and filters. 

To the right of the AudioChainSection are three EffectModuleSections. These three columns are where you can add - you guessed it - effects (delay, reverb, filters, etc).

Below these two sections is the ModulationModuleSection, which is where you can add your modulators (envelopes, LFOs). These can modulate parameters in the AudioChains or Effects.

Finally, in the bottom right corner is our MasterVoiceEnvelopeSection. This is where everything gets sent before it's output.

Audio Chains -> Effects -> Master Voice
(Modulators can affect any parameter)

# How ElectroSynth is structured on the backend


# Creating an Effect
Here are the steps that I took to create a delay module:

In third_party/leaf_modules,
- Create DelayModule.c, DelayModule.h, and DelayModule.cpp in the modules folder
- In [DelayModule.c](../third_party/leaf_modules/modules/DelayModule.c), simply `#include "DelayModule.cpp"`
- In [DelayModule.h](../third_party/leaf_modules/modules/DelayModule.h),
- In [DelayModule.cpp](../third_party/leaf_modules/modules/DelayModule.cpp),
- In [CMakeLists.txt](../third_party/leaf_modules/CMakeLists.txt), add `modules/DelayModule.cpp` as an input to add_library
- In [defs.h](../third_party/leaf_modules/processors/defs.h), add `ModuleTypeDelayModule` to the ModuleType enum
- In [funcmaps.h](../third_party/leaf_modules/processors/funcmaps.h), `#include "DelayModule.h"`
- In [funcmaps.cpp](../third_party/leaf_modules/processors/funcmaps.cpp), add `(createModuleFunc) tDelayModule_init` to the `module_init_map[]`
- In [mapping.cpp](../third_party/leaf_modules/processors/mapping.cpp)
  - `#include "DelayModule.h"`
  - add the following switch case to tMapping_setParameter
   ```
     case ModuleTypeDelayModule:
         tDelayModule_setParameter ((tDelayModule) module, (DelParams) paramID, value);
         break;
   ```

In ElectroSynth/source,
- Create DelayModuleProcessor.h/cpp
- In [EffectsModuleSection.cpp](../source/interface/sections/EffectsModuleSection.cpp), 
  - In createPopupMenu(), add `options.addItem(2, "add delay");`
- In handlePopupResult(), add
   ```
    else if (result == 2) {
        juce::ValueTree t(IDs::EFFECTMODULE);
        t.setProperty(IDs::type, "delay", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    }
   ```
- In [module_type_info.h](../source/common/module_type_info.h),
  - Add `TypeValuePair<_tDelayModule, 7>{},` to the map
  - Increment the length of module_strings and add `"delay"`
- In [ModuleList.cpp](../source/interface/sections/ModuleList.cpp),
  - `#include "DelayModuleProcessor.h"`
  - In ModuleList(), add
  ```
  factory.template registerType<DelayModuleProcessor,electrosynth::SoundEngine*, juce::ValueTree, LEAF*, juce::UndoManager*>("delay");
  ```
- In [parameterArrays.h](../source/synthesis/framework/parameterArrays.h), add the delay params
   ```
    // delay
    {
        "eventWatch",
        "delayTime",
        "gain",
        "",
        ""
    }
   ```