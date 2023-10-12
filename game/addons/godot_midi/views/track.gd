@tool
extends Control

var Note = preload("./note.tscn")

# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass

# A function that converts midi note numbers to note names, using modulo for octaves
func midi_to_name(midi_number):
	# A list of note names for one octave
	var note_names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
	# The number of notes in one octave
	var notes_per_octave = 12
	# The offset of the octave number from the midi number
	var octave_offset = 0
	# Calculate the note name and the octave number using modulo and division
	var note_name = note_names[midi_number % notes_per_octave]
	var octave_number = midi_number / notes_per_octave + octave_offset
	# Return the note name and the octave number as a string
	return note_name + str(octave_number)

func update_track_data(track: Dictionary):
	%TrackName.text = track["name"]
	%TrackEvents.text = "# Events: " + str(track["events"].size())

	var note_map: Dictionary

	var max_horizontal_pos = 0
	var max_vertical_pos = 0
	var min_vertical_pos = 1000.0

	# first, find lowest note
	for event in track.events:
		if event["type"] == "note":
			if event["subtype"] == MIDI_MESSAGE_NOTE_ON:
				var vertical_position = (127 - event["note"]) * 10.0
				if vertical_position < min_vertical_pos:
					min_vertical_pos = vertical_position

	for event in track.events:
		# check if we're parsing note events
		if event["type"] == "note":
			# check for note on events
			if event["subtype"] == MIDI_MESSAGE_NOTE_ON:
				note_map[event["note"]] = event["time"]
				
			# check for note off events
			if event["subtype"] == MIDI_MESSAGE_NOTE_OFF or (event["subtype"] == MIDI_MESSAGE_NOTE_ON and event["data"] == 0):
				# calculate offsets/timing info for display
				var on_time = note_map[event["note"]] * 100.0
				var off_time = event["time"] * 100.0
				var horizontal_size = (event["time"] - note_map[event["note"]]) * 100.0
				var vertical_position = ((127 - event["note"]) * 10.0) - min_vertical_pos
				
				# instantiate the note scene
				var note = Note.instantiate()
				note.note_name = midi_to_name(event["note"])
				note.note = event["note"]
				%NoteContainer.add_child(note)
				note.position.x = on_time
				note.position.y = vertical_position
				note.size.x = horizontal_size
				if off_time > max_horizontal_pos:
					max_horizontal_pos = off_time
				if vertical_position > max_vertical_pos:
					max_vertical_pos = vertical_position
	
	# resize container so scrolling works
	%NoteContainer.custom_minimum_size.y = max_vertical_pos
	%NoteContainer.custom_minimum_size.x = max_horizontal_pos
