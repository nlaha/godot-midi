@tool
extends EditorImportPlugin

enum Presets { DEFAULT }

func _get_importer_name():
	return "com.nlaha.godotmidi_sf2"

func _get_visible_name():
	return "Godot Midi SF2"

func _get_recognized_extensions() -> PackedStringArray:
	return PackedStringArray(["sf2"])

func _get_save_extension():
	return "res"

func _get_resource_type():
	return "Sf2Resource"

func _get_preset_count():
	return Presets.size()
	
func _get_priority():
	return 1.0
	
func _get_option_visibility(option, name, options):
	return true
	
func _get_import_options(name, preset):
	match preset:
		Presets.DEFAULT:
			return []
		_:
			return []

func _get_preset_name(preset):
	match preset:
		Presets.DEFAULT:
			return "Default"
		_:
			return "Unknown"

func _get_import_order():
	return 0

func _import(source_file, save_path, options, r_platform_variants, r_gen_files):

	print("[GodotMidi] Importing SoundFont file: " + source_file)

	var save_file = save_path + "." + _get_save_extension()
	var sf2_resource = Sf2Resource.new()
	if sf2_resource.load_file(source_file) != OK:
		printerr("[GodotMidi] Failed to load SoundFont file: " + source_file)
		return FAILED

	return ResourceSaver.save(sf2_resource, save_file)
