extends Node

@export var loop = true
@export var note_materials : Array[Material]

var notes = []
var notes_on = {}

var midi_player: MidiPlayer
var asp: AudioStreamPlayer

@onready var last_time = 0

# Called when the node enters the scene tree for the first time.
func _ready():
	midi_player = $MidiPlayer
	asp = $AudioStreamPlayer
	# we set the following to true so we can tick the midi player from
	# within our script, this is necessary for accurate syncing with the asp
	midi_player.manual_process = true
	midi_player.loop = false
	midi_player.note.connect(on_note)
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
	elif (event['subtype'] == MIDI_MESSAGE_NOTE_OFF): # note off
		notes_on.erase(event['note'])
	#print("[Track: " + str(track) + "] Note on: " + str(event['note']))
