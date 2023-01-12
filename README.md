# Godot Midi 4.0+
A MIDI file importer for Godot 4.x
**Note: this is a re-write of my Godot 3.5 extension GodotMidi**

This plugin aims to make rythm game development and music syncing easier than ever before. Simply import a midi file like you would any other godot asset and it will get converted to an animation, this animation can then be paired with a "MidiManager" node that sends out signals every time a midi event is fired. This project is a work in progress and is lacking some features, so feel free to contribute any code or ideas over on the pull requests page.

<img width="928" alt="image" src="https://user-images.githubusercontent.com/10292944/209430133-356d68e8-1399-49c2-aace-00449e016006.png">

GodotMidi makes it easy to sync midi files to their corresponding rendered audio, just add an audio stream to the auto-generated audio stream player (a child of MidiManager) and it will play when the midi file starts.

<img width="325" alt="image" src="https://user-images.githubusercontent.com/10292944/209430189-eb94371b-b78c-4fda-8f2d-e694ec6fabe4.png">

## Installation

1. Download the latest release over at https://github.com/nlaha/GodotMidi/releases

2. Extract it to your `res://addons/` folder

3. Enable the plugin in the godot project settings menu

## Usage

1. Import a midi file by adding it to your project folder

2. Add a "MidiManager" node to your scene, it will automatically add an AnimationPlayer as well

**NOTE: MidiManager must be under the root of your scene, do not put it as a child of any other node**

<img width="213" alt="image" src="https://user-images.githubusercontent.com/10292944/209430279-a2897206-9d93-4072-8b84-cf3cb9e3f16d.png">

3. Select the animation player and click the animation button to load an animation resource into the player

<img width="164" alt="image" src="https://user-images.githubusercontent.com/10292944/209430316-0de7f136-a876-4a44-9f73-25763b238774.png">

4. Browse to your midi file location, if the import worked correctly, it should be selectable as an animation resource just like a normal animation would.

<img width="391" alt="image" src="https://user-images.githubusercontent.com/10292944/209430341-31bc7db6-b14b-497f-b584-f21a8361e6a7.png">

5. Have fun! The animation player should work just as a normal animation player would, each green dot corresponds to a MIDI event and a function call to `note_event_input(...)` in the MidiManager node.