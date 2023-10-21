@tool
extends Control

var note_name: String = "" :
	set(val):
		note_name = val
		$Tooltip/NoteName.text = val
	get:
		return note_name
		
var note: int = 0 :
	set(val):
		note = val
		$Tooltip/NoteNumber.text = str(val)
	get:
		return note

# Called when the node enters the scene tree for the first time.
func _ready():
	pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass

func _on_mouse_entered():
	$Tooltip.visible = true

func _on_mouse_exited():
	$Tooltip.visible = false
