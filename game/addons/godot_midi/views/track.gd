@tool
extends Control

var Note = preload("./note.tscn")

const VERTICAL_ZOOM: int = 5.0
const TIME_ZOOM: int = 100.0

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

func update_track_data(track: Dictionary, midi: MidiResource):
	%TrackName.text = track["name"]
	%TrackEvents.text = "# Events: " + str(track["events"].size())

	var note_map: Dictionary

	var max_horizontal_pos = 0
	var max_vertical_pos = 0
	var min_vertical_pos = 1000.0
	
	var has_notes = false

	# first, find lowest note
	for event in track.events:
		if event["type"] == "note":
			has_notes = true
			if event["subtype"] == MIDI_MESSAGE_NOTE_ON:
				var vertical_position = (127 - event["note"]) * VERTICAL_ZOOM
				if vertical_position < min_vertical_pos:
					min_vertical_pos = vertical_position

	if not has_notes:
		queue_free()
		return

	for event in track.events:
		# check if we're parsing note events
		if event["type"] == "note":
			# check for note on events
			if event["subtype"] == MIDI_MESSAGE_NOTE_ON:
				note_map[event["note"]] = event["time"]
				
			# check for note off events
			if event["subtype"] == MIDI_MESSAGE_NOTE_OFF or (event["subtype"] == MIDI_MESSAGE_NOTE_ON and event["data"] == 0):
				# calculate offsets/timing info for display
				var off_time = event["time"] * TIME_ZOOM
				var on_time = off_time
				var horizontal_size = 0
				if note_map.has(event["note"]):
					on_time = note_map[event["note"]] * TIME_ZOOM
					horizontal_size = (event["time"] - note_map[event["note"]]) * TIME_ZOOM
				var vertical_position = ((127 - event["note"]) * VERTICAL_ZOOM) - min_vertical_pos
				
				# instantiate the note scene
				var note = Note.instantiate()
				note.note_name = midi_to_name(event["note"])
				note.note = event["note"]
				%NoteContainer.add_child(note)
				note.position.x = on_time
				note.position.y = vertical_position
				note.size.x = horizontal_size
				note.size.y = VERTICAL_ZOOM
				if off_time > max_horizontal_pos:
					max_horizontal_pos = off_time
				if vertical_position > max_vertical_pos:
					max_vertical_pos = vertical_position
	
	# resize container so scrolling works
	%NoteContainer.custom_minimum_size.y = max_vertical_pos
	%NoteContainer.custom_minimum_size.x = max_horizontal_pos
	
	# calculate quarter note seconds
	var bpm = 60000000 / midi.tempo
	var qnote_seconds = bpm / 60
	
	if max_horizontal_pos == 0:
		max_horizontal_pos = TIME_ZOOM
	var max_time = max_horizontal_pos / TIME_ZOOM
	
	# ceil max_horizontal_pos to nearest multiple of qnote_seconds
	var max_time_ceil = 0
	var rem = ceili(max_time) % qnote_seconds
	if rem == 0:
		max_time_ceil = max_time
	else:
		max_time_ceil = max_time + qnote_seconds - rem
	
	# set grid shader
	%GridBackground.material.set_shader_parameter("scale", Vector2((max_time_ceil * qnote_seconds) * 2, max_vertical_pos / VERTICAL_ZOOM))
