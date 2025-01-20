# Electrosynth 
plugin front end for electrobass and electrosteel

Electrosynth the [PampleJuce](docs/PampleJuceREADME.md) CMake framework 
to avoid the the use of the Projucer and make it easier to include external 
libraries

## External Libraries
External libraries can be found in both the [modules](modules) 
and [third_party](third_party) folders. 
### modules
Modules is intended for use with
JUCE specific modules which have their proprietary include structure set by the
JUCE standard.Information about how to include these in cmake can be found 
[here](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module)
Information about the format for creating a juce module can be found [here](https://github.com/juce-framework/JUCE/blob/master/docs/JUCE%20Module%20Format.md)
build structure 


 - information about how this works can be
   found in this [melatonin_audio_sparklines blog post]([here](https://melatonin.dev/blog/audio-sparklines/))
-  [melatonin_audio_sparklines](https://github.com/sudara/melatonin_audio_sparklines)
  - allows us to print audio to the console in debug mode using  `melatonin::printSparkline(myAudioBlock)`


### third_party

this folder is third party libraries that are included without the `juce_add_submodule` command.
The usual way these libraries are added is with
[add_subdirectory](https://cmake.org/cmake/help/latest/command/add_subdirectory.html) 
in our [CMakeLists.txt](CMakeLists.txt) file. `add_subdirectory` works by searching 
the directory listed for its own `CMakeLists.txt` to determine how to build the library.

- juce-toys
  - using the juce_lldb_xcode.py file to make juce::string and juce::component
    display more usable debug information. Installation instructions can be found at
    the top of the [file](modules/juce-toys/juce_lldb_xcode.py)
  - also [value_tree_debugger](source/common/valuetree_utils/value_tree_debugger.h)
  has been removed form the module and put in our source
- sfzq 
  - sound fonts (unimplemented)
- [melatonin_inspector](https://github.com/sudara/melatonin_inspector)
  - allows us to view information and move around `juce::Components` in our UI
  - *NOTE: the features of this module do not all work due to the use of
    openGLRendering* 
- [tracktion_engine](https://github.com/Tracktion/tracktion_engine)
  - we were previouisly linking against this whole library as a submodule
  but after various headaches from using it [davispolito](https://github.com/davispolito) 
  decided to just grab the utility files we wanted and put them here
- chowdsp
  - more to come

https://git-lfs.com 