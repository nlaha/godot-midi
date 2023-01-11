#include "register_types.h"

#include "midi_importer.h"
#include "midi.h"
#include "midi_parser.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_godotmidi_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		return;
	}
	ClassDB::register_class<MIDI>();
	ClassDB::register_class<MidiImporter>();
	ClassDB::register_class<MIDIParser>();
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

	GDExtensionBool GDE_EXPORT godotmidi_library_init(const GDExtensionInterface *p_interface, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

		init_obj.register_initializer(initialize_godotmidi_types);
		init_obj.register_terminator(uninitialize_godotmidi_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
