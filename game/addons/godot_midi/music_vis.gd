extends Node3D

var mm
var notes = []

var notes_on = {}

# Called when the node enters the scene tree for the first time.
func _ready():
	mm = get_node("../MidiManager")
	mm.note_event.connect(on_note)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	for note in notes_on:
		# spawn a cube
		var box = MeshInstance3D.new()
		box.mesh = BoxMesh.new()
		box.scale = Vector3(0.1, 0.01, 0.1)
		add_child(box)
		box.owner = get_tree().edited_scene_root
		box.position.x = remap(note, 0, 127, -10, 10)
		notes.append(box)
		
	for note in notes:
		note.position.y += delta

func on_note(note, data, type, track):
	if (type == 0): # note on
		notes_on[note] = type
	elif (type == 1): # note off
		notes_on.erase(note)

