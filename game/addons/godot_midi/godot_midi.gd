@tool
extends EditorPlugin

const MainView = preload("./views/main_view.tscn")

var midi_import_plugin
var sf2_import_plugin

var main_view
var export_plugin : AndroidExportPlugin

func _enter_tree():
	midi_import_plugin = preload("midi_import_plugin.gd").new()
	add_import_plugin(midi_import_plugin)

	sf2_import_plugin = preload("sf2_import_plugin.gd").new()
	add_import_plugin(sf2_import_plugin)

	if Engine.is_editor_hint():
		main_view = MainView.instantiate()
		# Add the main panel to the editor's main viewport.
		get_editor_interface().get_editor_main_screen().add_child(main_view)
		# Hide the main panel. Very much required.
		_make_visible(false)

	export_plugin = AndroidExportPlugin.new()
	add_export_plugin(export_plugin)


func _exit_tree():
	remove_import_plugin(midi_import_plugin)
	midi_import_plugin = null
	
	remove_import_plugin(sf2_import_plugin)
	sf2_import_plugin = null

	if is_instance_valid(main_view):
		main_view.queue_free()

	remove_export_plugin(export_plugin)
	export_plugin = null


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


class AndroidExportPlugin extends EditorExportPlugin:
	var _plugin_name = "GodotMidi"

	func _supports_platform(platform):
		if platform is EditorExportPlatformAndroid:
			return true
		return false

	func _get_android_libraries(platform, debug):
		if debug:
			return PackedStringArray(["godot_midi/bin/GodotMidi-debug.aar"])
		else:
			return PackedStringArray(["godot_midi/bin/GodotMidi-release.aar"])

	func _get_name():
		return _plugin_name
