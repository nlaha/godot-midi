#include "register_types.h"

#include "midi_parser.h"
#include "midi_resource.h"
#include "midi_player.h"
#include "audio_stream_player_midi.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/editor_import_plugin.hpp>

using namespace godot;

void initialize_godotmidi_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		return;
	}
	ClassDB::register_class<MidiParser>();
	ClassDB::register_class<MidiResource>();
	ClassDB::register_class<MidiPlayer>();

	ClassDB::register_class<Sf2Resource>();

	ClassDB::register_class<AudioStreamPlayerMidi>();
}

void uninitialize_godotmidi_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		return;
	}
}

extern "C"
{

	// Initialization.

	GDExtensionBool GDE_EXPORT godotmidi_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{

		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize_godotmidi_types);
		init_obj.register_terminator(uninitialize_godotmidi_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
