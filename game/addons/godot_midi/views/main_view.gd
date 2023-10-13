@tool
extends Control

var MidiTrack = preload("./track.tscn")

var scroll_value = 0

@onready var midi_file_open_dialogue = $MidiFileOpenDialogue

# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass
	
func _on_load_midi_file_btn_pressed():
	midi_file_open_dialogue.popup()

func _on_midi_file_open_dialogue_file_selected(path):
	print("[GodotMidi] Loading MidiResource for editing from path: " + path)
	# load midi file resource
	var midi: MidiResource = load(path)
	
	# update midi info from header
	%MidiFileInfo.text = "Midi File: " \
	+ "Path: " + path + "\n" \
	+ "Track Count: " + str(midi.track_count) + "\n" \
	+ "Time Division: " + str(midi.division) + "\n" \
	+ "Tempo: " + str(60000000 / midi.tempo)
	
	# remove old tracks
	for child in %Tracks.get_children():
		child.queue_free()
	
	# update track data
	for track in midi.tracks:
		var track_scn = MidiTrack.instantiate()
		track_scn.update_track_data(track, midi)
		%Tracks.add_child(track_scn)
