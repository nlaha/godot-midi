![banner_logo_long](https://github.com/nlaha/godot-midi/assets/10292944/4e5b5125-0453-4f92-9ac7-048cfb2c8067)

## Overview

This plugin aims to make rhythm game development and music syncing easier than ever before. Import a midi file like you would any other Godot asset, this can then be paired with a "MidiPlayer" node that sends out signals every time a midi event is fired. This project is a work in progress and lacks some features, so feel free to contribute any code or ideas on the pull requests page.

https://github.com/nlaha/godot-midi/assets/10292944/f88acfac-1ff3-49ee-8d25-9ee0ee585d09

## Compatibility

This GDExtension addon is only compatible with Godot version 4.1.2 and higher.

## Installation from binaries

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

> **NOTE:** If you run into import errors or problems with a midi file you downloaded from the internet, it's likely there is a midi event or format that isn't supported by Godot Midi. The best way to fix this is to import the midi file into a DAW (digital audio workstation) or similar software and re-export it. This should convert the midi file into a format easily readable by Godot Midi.

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

## Manual Process (Syncing with Music)

Godot Midi supports two sync modes, **automatic process** and **manual process**. The example above shows automatic process, this is great for use cases where you don't need to sync midi playback with an audio player or a game system. However, in rhythm games and other time-sensitive projects, it's recommended you use manual process. Manual process allows you to "tick" the midi player with an arbitrary delta time value. Below is an example of manual process being used to sync an audio stream player

Note the mechanism for looping the audio and midi. Because Godot doesn't have a signal for detecting when the AudioStreamPlayer has looped, we need to manually implement looping logic for both the midi and the music.

```gdscript
   func _ready():
      midi_player.note.connect(my_note_callback)
      midi_player.manual_process = true
      midi_player.play()

   	if loop:
   		asp.finished.connect(on_loop)

   func on_loop():
   	# loop midi player
   	print("Looping MIDI")
   	last_time = 0
   	midi_player.current_time = 0
   	midi_player.stop() # resets time
   	midi_player.play() # plays midi
   	
   	# play audio stream
   	asp.play()

   # Called every frame. 'delta' is the elapsed time since the previous frame.
   func _process(delta):
   
      # get asp playback time
      var time = asp.get_playback_position() + AudioServer.get_time_since_last_mix()
      # Compensate for output latency.
      time -= AudioServer.get_output_latency()
      
      # tick the midi player with the delta from our audio stream player
      # this syncs the midi player with the audio server
      # this is a more accurate way of doing it than using the delta from _process
      var asp_delta = time - last_time
      last_time = time
      midi_player.process_delta(asp_delta)

   func my_note_callback(event, track):
      if (event['subtype'] == MIDI_MESSAGE_NOTE_ON): # note on
         # do something on note on
      elif (event['subtype'] == MIDI_MESSAGE_NOTE_OFF): # note off
         # do something on note off
      
      print("[Track: " + str(track) + "] Note played: " + str(event['note']))
```

Open the demo project for an included music visualizer script!
