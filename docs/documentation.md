# ElectroSynth

Callista Chong! 

## Saving and Loading Presets
So you've created some cool things in ElectroSynth and you want to save them for later... Here's how saving and loading your presets works behind the scenes.

### Saving Presets
When you click 'File > Save' or 'CMD + s' both of these will call openSaveDialog() in [SynthGuiInterface](../source/common/synth_gui_interface.cpp). This creates and launches the FileChooser with the correct flags. 

