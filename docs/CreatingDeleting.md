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