extends Node

@export var note_materials: Array[Material]

var notes = []
var notes_on = {}

var midi_player: MidiPlayer
var aspm: AudioStreamPlayerMidi
#var asp: AudioStreamPlayer

# Called when the node enters the scene tree for the first time.
func _ready():
	midi_player = $MidiPlayer

	midi_player.note.connect(on_note)

	# linking an ASP allows for async playback of audio with midi events
	# for better syncing when using pre-rendered audio
	# alternatively, link an audio stream player midi for synthesized audio
	# asp = $AudioStreamPlayer
	aspm = $AudioStreamPlayerMidi
	midi_player.play()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	# spawn notes
	for note in notes_on:
		# spawn a cube
		var box = MeshInstance3D.new()
		box.mesh = BoxMesh.new()
		box.scale = Vector3(0.1, 0.05, 0.1)
		var mat_index = notes_on[note] - 1
		if mat_index < len(note_materials):
			box.material_override = note_materials[mat_index]
		else:
			box.material_override = note_materials[0]
		add_child(box)
		box.owner = get_tree().edited_scene_root
		box.position.x = remap(note, 0, 127, -15, 15)
		notes.append(box)

	# remove notes when they go off screen
	for note in notes:
		note.position.y += delta
		if note.position.y > 20:
			notes.remove_at(notes.find(note))
			note.queue_free()

# Called when a "note" type event is played
func on_note(event, track):
	match event['subtype']:
		MIDI_MESSAGE_NOTE_ON:
			notes_on[event['note']] = track
			aspm.note_on(event['note'], event['data'], event['channel'])
		MIDI_MESSAGE_NOTE_OFF:
			notes_on.erase(event['note'])
			aspm.note_off(event['note'], event['channel'])
		MIDI_MESSAGE_PROGRAM_CHANGE:
			aspm.program_change(event['channel'], event['note'])
		MIDI_MESSAGE_PITCH_BEND:
			var bend = (event['data'] << 7) | event['note']
			aspm.pitch_bend(event['channel'], bend)
		MIDI_MESSAGE_CONTROL_CHANGE:
			aspm.control_change(event['channel'], event['note'], event['data'])
