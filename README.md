# Godot Midi

## Overview

This plugin aims to make rhythm game development and music syncing easier than ever before. Import a midi file like you would any other Godot asset, this can then be paired with a "MidiPlayer" node that sends out signals every time a midi event is fired. This project is a work in progress and lacks some features, so feel free to contribute any code or ideas on the pull requests page.

https://github.com/nlaha/godot-midi-4.0/assets/10292944/85e1444e-3503-4e44-a62a-c7efc8025ddc

## Compatibility

This GDExtension addon is only compatible with Godot version 4.1.2 and higher.

## Installation from binaries

Note, due to https://github.com/nlaha/godot-midi/issues/11, binaries for MacOS are currently unavailable, please build from source.

1. Download the latest release from https://github.com/nlaha/godot-midi/releases

2. Copy the `godot-midi` folder to your project's `addons` folder
   
4. Enable the addon in Godot's project settings

## Building from source

1. Clone the repository with `git clone --recursive https://github.com/nlaha/godot-midi.git`

2. Make sure you have SCons installed

3. Run `cd godot-cpp` and `scons target=template_debug` or `scons target=template_release` to build godot-cpp

4. Run `scons target=template_debug` or `scons target=template_release` in the root directory to build the extension

5. Copy the `game/addons/godot_midi` folder to your project's addons folder

6. Enable the plugin in the Godot project settings menu

## Usage

1. Import a midi file by adding it to your project folder

2. Add a "MidiPlayer" node to your scene

![image](https://github.com/nlaha/godot-midi-4.0/assets/10292944/30c15ea4-ae06-4baf-8248-c995b0a2dc2f)

4. Set the midi resource you want the MidiPlayer to play

![image](https://github.com/nlaha/godot-midi-4.0/assets/10292944/7e2e019e-290b-4580-b2ca-4506263f14c0)

4. Connect to the "note" signal in a GDScript

   ```gdscript

   func _ready():
    midi_player.note.connect(my_note_callback)
    midi_player.play()

   func my_note_callback(event, track):
       if (event['subtype'] == MIDI_MESSAGE_NOTE_ON): # note on
           # do something on note on
       elif (event['subtype'] == MIDI_MESSAGE_NOTE_OFF): # note off
           # do something on note off

       print("[Track: " + str(track) + "] Note played: " + str(event['note']))


   ```

Open the demo project for an included music visualizer script!
