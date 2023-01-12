#ifndef MIDI_CLASS_H
#define MIDI_CLASS_H

// We don't need windows.h in this plugin but many others do and it throws up on itself all the time
// So best to include it and make sure CI warns us when we use something Microsoft took for their own goals....
#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

class Midi : public RefCounted
{
    GDCLASS(Midi, RefCounted);

protected:
    static void _bind_methods();

public:
    Midi();
    ~Midi();

    void load_from_file(String source_path, String save_path);
};

#endif // MIDI_CLASS_H