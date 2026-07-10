@tool
extends EditorPlugin

var midi_import_plugin
var sf2_import_plugin

var export_plugin: AndroidExportPlugin

func _enter_tree():
	midi_import_plugin = preload("midi_import_plugin.gd").new()
	add_import_plugin(midi_import_plugin)

	sf2_import_plugin = preload("sf2_import_plugin.gd").new()
	add_import_plugin(sf2_import_plugin)

	export_plugin = AndroidExportPlugin.new()
	add_export_plugin(export_plugin)


func _exit_tree():
	remove_import_plugin(midi_import_plugin)
	midi_import_plugin = null
	
	remove_import_plugin(sf2_import_plugin)
	sf2_import_plugin = null

	remove_export_plugin(export_plugin)
	export_plugin = null

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
