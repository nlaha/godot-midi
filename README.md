![banner_logo_long](https://github.com/nlaha/godot-midi/assets/10292944/4e5b5125-0453-4f92-9ac7-048cfb2c8067)

[![Builds](https://github.com/nlaha/godot-midi/actions/workflows/builds.yml/badge.svg)](https://github.com/nlaha/godot-midi/actions/workflows/builds.yml)
[![CodeQL](https://github.com/nlaha/godot-midi/actions/workflows/codeql.yml/badge.svg)](https://github.com/nlaha/godot-midi/actions/workflows/codeql.yml)

## Overview

This plugin aims to make rhythm game development and music syncing easier than ever before. Import a midi file like you would any other Godot asset, this can then be paired with a "MidiPlayer" node that sends out signals every time a midi event is fired. This project is a work in progress and lacks some features, so feel free to contribute any code or ideas on the pull requests page.

https://github.com/nlaha/godot-midi/assets/10292944/f88acfac-1ff3-49ee-8d25-9ee0ee585d09

https://github.com/nlaha/godot-midi/assets/10292944/543646ec-ed45-406b-a3e5-f2b26caabfbe

## Compatibility

This GDExtension addon is compatible with Godot version 4.2.2 and higher. Earlier 4.x versions may work, however they are not officially supported.

### Platforms

This extension is compatible with Windows, Mac and Linux. Support for Android is also availble but is not yet integrated into the CI pipelines. For more information on how to compile for Android, see this pull request:
https://github.com/nlaha/godot-midi/pull/35

## Installation from binaries

1. Download the latest release from https://github.com/nlaha/godot-midi/releases
> If you'd like to download a newer version that hasn't been released, download it from the latest Github Actions run: https://github.com/nlaha/godot-midi/actions/workflows/builds.yml

2. Copy the `godot-midi` folder to your project's `addons` folder
3. Enable the addon in Godot's project settings

## Building from source

1. Clone the repository with `git clone --recursive https://github.com/nlaha/godot-midi.git`

2. Make sure you have SCons installed

3. Run `cd godot-cpp` and `scons target=template_debug` or `scons target=template_release` to build godot-cpp

4. Run `scons target=template_debug` or `scons target=template_release` in the root directory to build the extension

5. Copy the `game/addons/godot_midi` folder to your project's addons folder

6. Enable the plugin in the Godot project settings menu

## Usage

1. Import a midi file by adding it to your project folder

> **NOTE:** If you run into import errors or problems with a midi file you downloaded from the internet, it's likely there is a midi event or format that isn't supported by Godot Midi. The best way to fix this is to import the midi file into a DAW (digital audio workstation) or similar software and re-export it. This should convert the midi file into a format easily readable by Godot Midi. I do all my testing with FL Studio so I'd recommend that, you can use the free demo version if you don't have a license.

3. Add a "MidiPlayer" node to your scene

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

## Syncing with Music (AudioStreamPlayer)

Because the game thread frame time can fluctuate depending on the system load, GodotMidi's player is run on a separate thread. Because of this, it's best to use the built-in synchronization feature if you want to sync MIDI events to music.

The good news: it's easy to use! Just call `link_audio_stream_player(...)` with your ASP and it will automatically start/stop/pause the ASP for you!

```gdscript
   func _ready():
      midi_player.loop = true
      midi_player.note.connect(my_note_callback)

      # link the AudioStreamPlayer in your scene
      # that contains the music associated with the midi
      # NOTE: this must be an array, you can link multiple ASPs or one as 
      # shown below and they will all sync with playback of the MIDI
      midi_player.link_audio_stream_player([asp])

      # this will also start the audio stream player (music)
      midi_player.play()

   func my_note_callback(event, track):
      if (event['subtype'] == MIDI_MESSAGE_NOTE_ON): # note on
         # do something on note on
      elif (event['subtype'] == MIDI_MESSAGE_NOTE_OFF): # note off
         # do something on note off

      print("[Track: " + str(track) + "] Note played: " + str(event['note']))
```

Open the demo project for an included music visualizer script!

## Importing MIDI files at runtime

A similar approach to how the plugin imports MIDI files in the editor can also be used to import them at runtime. Create a `MidiResource` manually, and call the `load_midi` method with a path to the source MIDI file.
https://github.com/nlaha/godot-midi/blob/a7d40af0083c8e314b6de619126f87f199d6b661/game/addons/godot_midi/midi_import_plugin.gd#L52-L55
