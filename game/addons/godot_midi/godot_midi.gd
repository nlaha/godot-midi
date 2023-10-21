@tool
extends EditorPlugin

const MainView = preload("./views/main_view.tscn")

var import_plugin
var main_view

func _enter_tree():
	import_plugin = preload("midi_import_plugin.gd").new()
	add_import_plugin(import_plugin)

	if Engine.is_editor_hint():
		main_view = MainView.instantiate()
		# Add the main panel to the editor's main viewport.
		get_editor_interface().get_editor_main_screen().add_child(main_view)
		# Hide the main panel. Very much required.
		_make_visible(false)

func _exit_tree():
	remove_import_plugin(import_plugin)
	import_plugin = null

	if is_instance_valid(main_view):
		main_view.queue_free()

func _has_main_screen():
	return true
	
func _make_visible(visible):
	if is_instance_valid(main_view):
		main_view.visible = visible

func _get_plugin_name():
	return "Godot Midi"

func _get_plugin_icon():
	# Must return some kind of Texture for the icon.
	return get_editor_interface().get_base_control().get_theme_icon("AudioStreamPolyphonic", "EditorIcons")
