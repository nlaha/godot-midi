extends Node3D

@export var note_material : Material

var mp
var notes = []

var notes_on = {}

# Called when the node enters the scene tree for the first time.
func _ready():
	mp = $MidiPlayer
	mp.note.connect(on_note)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	for note in notes_on:
		# spawn a cube
		var box = MeshInstance3D.new()
		box.mesh = BoxMesh.new()
		box.scale = Vector3(0.1, 0.05, 0.1)
		box.material_override = note_material
		add_child(box)
		box.owner = get_tree().edited_scene_root
		box.position.x = remap(note, 0, 127, -10, 10)
		notes.append(box)
		
	for note in notes:
		note.position.y += delta
		if note.position.y > 20:
			notes.remove_at(notes.find(note))
			note.queue_free()

func on_note(event):
	if (event['subtype'] == MIDI_MESSAGE_NOTE_ON): # note on
		notes_on[note] = type
	elif (type['subtype'] == MIDI_MESSAGE_NOTE_OFF): # note off
		notes_on.erase(note)

