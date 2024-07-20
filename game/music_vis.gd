extends Node

@export var note_materials : Array[Material]

var notes = []
var notes_on = {}

var midi_player: MidiPlayer
var asp: AudioStreamPlayer

# Called when the node enters the scene tree for the first time.
func _ready():
	midi_player = $MidiPlayer
	asp = $AudioStreamPlayer

	# linking an ASP allows for async playback of audio with midi events
	# for better syncing
	midi_player.note.connect(on_note)
	midi_player.link_audio_stream_player([asp])
	midi_player.play()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	# spawn notes
	for note in notes_on:
		# spawn a cube
		var box = MeshInstance3D.new()
		box.mesh = BoxMesh.new()
		box.scale = Vector3(0.1, 0.05, 0.1)
		box.material_override = note_materials[notes_on[note] - 1]
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
	if (event['subtype'] == MIDI_MESSAGE_NOTE_ON): # note on
		notes_on[event['note']] = track
		print(event)
		#$SFX.play()
	elif (event['subtype'] == MIDI_MESSAGE_NOTE_OFF): # note off
		notes_on.erase(event['note'])
	#print("[Track: " + str(track) + "] Note on: " + str(event['note']))
