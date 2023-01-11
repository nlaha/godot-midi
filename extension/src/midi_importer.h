#ifndef MIDIIMPORTER_CLASS_H
#define MIDIIMPORTER_CLASS_H

// We don't need windows.h in this plugin but many others do and it throws up on itself all the time
// So best to include it and make sure CI warns us when we use something Microsoft took for their own goals....
#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/editor_import_plugin.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/classes/resource_importer.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/animation.hpp>
#include "midi.h"

using namespace godot;

class MidiImporter : public EditorImportPlugin
{
    GDCLASS(MidiImporter, RefCounted);

protected:
    static void _bind_methods();
    String _get_importer_name() const;
    String _get_visible_name() const;
    int64_t _get_preset_count() const;
    String _get_preset_name(int64_t preset_index) const;
    PackedStringArray _get_recognized_extensions() const;
    TypedArray<Dictionary> _get_import_options(const String &path, int64_t preset_index) const;
    String _get_save_extension() const;
    String _get_resource_type() const;
    int64_t _import(const String &source_file, const String &save_path, const Dictionary &options, const TypedArray<String> &platform_variants, const TypedArray<String> &gen_files) const;

public:
    MidiImporter();
    ~MidiImporter();
};

#endif // MIDIIMPORTER_CLASS_H