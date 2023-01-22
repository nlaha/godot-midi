@tool
extends Node

class_name MidiManager

# inspector exported vars
@export var auto_play_midi = true
@export var auto_play_music = true
@export var loop_midi = true

# output signal(s)
signal note_event
signal meta_event
signal system_event

# private vars
var midi_ap
var music_player
var midi_started
var midi_start_signal = false

# Called when the node enters the scene tree for the first time.
func _ready():
	
	# set up nodes in editor
	if (Engine.is_editor_hint()):
		# auto create animation player
		if (get_node_or_null("MidiPlayer") == null):
			var ap = AnimationPlayer.new()
			ap.set_name("MidiPlayer")
			add_child(ap)
			
			ap.owner = get_tree().edited_scene_root
			midi_ap = ap
		
		# auto create audio stream player
		if (get_node_or_null("MusicPlayer") == null):
			var asp = AudioStreamPlayer.new()
			asp.set_name("MusicPlayer")
			add_child(asp)
			
			asp.owner = get_tree().edited_scene_root
			music_player = asp
		
	# get nodes at runtime
	if (midi_ap == null):
		midi_ap = get_node_or_null("MidiPlayer")
		
	if (music_player == null):
		music_player = get_node_or_null("MusicPlayer")
	
	# if auto play is enabled play the animation
	if (!Engine.is_editor_hint()):
		if (auto_play_midi):
			var anim_name = midi_ap.get_animation_list()[0]
			# loop the animation if loop_midi is enabled
			midi_ap.get_animation(anim_name).loop_mode = Animation.LOOP_LINEAR
			midi_ap.play(anim_name)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if (auto_play_music && midi_start_signal && !music_player.playing):
		music_player.play()
			
func note_event_input(note, data, type, track):
	emit_signal("note_event", note, data, type, track)
	
func meta_event_input(type, data, text, track):
	midi_start_signal = true
	emit_signal("meta_event", type, data, text, track)

func system_event_input(type, track):
	emit_signal("system_event", type, track)
	
