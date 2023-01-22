extends Node3D

@export var note_material : Material

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

func on_note(note, data, type, track):
	if (type == 0): # note on
		notes_on[note] = type
	elif (type == 1): # note off
		notes_on.erase(note)

