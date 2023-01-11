#include "midi_importer.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

/// Importer for .midi files.
/// This importer is used to import MIDI files into Godot.

String MidiImporter::_get_importer_name() const
{
    // The name of the importer.
    return String("midi.importer.godotmidi");
}

String MidiImporter::_get_visible_name() const
{
    // The name of the importer as it appears in the import dialog.
    return String("Standard Midi File (MIDI)");
}

PackedStringArray MidiImporter::_get_recognized_extensions() const
{
    PackedStringArray extensions;
    extensions.append("midi");
    extensions.append("mid");
    return extensions;
}

String MidiImporter::_get_save_extension() const
{
    // The extension that will be used for the imported resource.
    return String("res");
}

String MidiImporter::_get_resource_type() const
{
    // The type of the imported resource.
    return String("Animation");
}

int64_t MidiImporter::_get_preset_count() const
{
    // The number of presets that this importer supports.
    return 0;
}

String MidiImporter::_get_preset_name(int64_t preset_index) const
{
    // The name of the preset at the given index.
    return String("Default");
}

TypedArray<Dictionary> MidiImporter::_get_import_options(const String &path, int64_t preset_index) const
{
    // The list of import options that this importer supports.
    return TypedArray<Dictionary>();
}

int64_t MidiImporter::_import(const String &source_file, const String &save_path, const Dictionary &options, const TypedArray<String> &platform_variants, const TypedArray<String> &gen_files) const
{
    MIDI midi;

    Animation midi_res = midi.load_from_file(source_file);

    String save_file = save_path + String(".") + _get_save_extension();
    ResourceSaver saver;
    Error status = saver.save(&midi_res, save_file);

    if (status == OK)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/// @brief constructor
MidiImporter::MidiImporter()
{
}

/// @brief destructor
MidiImporter::~MidiImporter()
{
}

/// @brief override method for registering c++ functions in godot
void MidiImporter::_bind_methods()
{
}