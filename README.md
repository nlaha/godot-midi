# Godot Midi 4.x
A MIDI file importer for Godot 4.x

**Note: this is a re-write of my Godot 3.5 extension GodotMidi**

Now supporting polyphony!

This plugin aims to make rythm game development and music syncing easier than ever before. Simply import a midi file like you would any other godot asset and it will get converted to an animation, this animation can then be paired with a "MidiManager" node that sends out signals every time a midi event is fired. This project is a work in progress and is lacking some features, so feel free to contribute any code or ideas over on the pull requests page.

![image](https://user-images.githubusercontent.com/10292944/212020820-62d88977-ff34-455a-a45d-f334cef63396.png)


https://user-images.githubusercontent.com/10292944/212436300-37939dc6-e345-4300-8543-a16e4a451f3f.mp4


GodotMidi makes it easy to sync midi files to their corresponding rendered audio, just add an audio stream to the auto-generated audio stream player (a child of MidiManager) and it will play when the midi file starts.

![image](https://user-images.githubusercontent.com/10292944/212021070-42f3728a-c3ec-43da-9173-035cd7812817.png)

## Installation

1. Download the latest release over at https://github.com/nlaha/godot-midi-4.0/releases

2. Extract it to your `res://` folder such that both addons and bin are in the root of your project

3. Enable the plugin in the godot project settings menu

## Usage

1. Import a midi file by adding it to your project folder

2. Add a "MidiManager" node to your scene, it will automatically add an AnimationPlayer as well

![image](https://user-images.githubusercontent.com/10292944/212020953-fe813fde-bc58-40a2-aad8-bef984282c78.png)

3. Select the animation player and click the animation button to load an animation resource into the player

![image](https://user-images.githubusercontent.com/10292944/212021217-c7924909-de4f-44ab-800f-a67a92f91420.png)

![image](https://user-images.githubusercontent.com/10292944/212021511-eee304b4-328b-41d7-a1b0-faaca74448ef.png)

4. Browse to your midi file location, if the import worked correctly, it should be selectable as an animation resource just like a normal animation would.

![image](https://user-images.githubusercontent.com/10292944/212021652-0c1357c3-cc50-4f5c-b582-74b664db70c3.png)

5. Have fun! The animation player should work just as a normal animation player would, each green dot corresponds to a MIDI event and a function call to `note_event_input(...)` in the MidiManager node.

Open the demo project for an included music visualizer script!
