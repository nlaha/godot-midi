#ifndef SF2_RESOURCE_H
#define SF2_RESOURCE_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "tsf.h"

using namespace godot;

/// @brief Sf2Resource class, simply loads the sf2 file into memory
class Sf2Resource : public Resource
{
    GDCLASS(Sf2Resource, Resource);

protected:
    static void _bind_methods()
    {
        // load method
        ClassDB::bind_method(D_METHOD("load_file", "path"), &Sf2Resource::load_file);

        // get file buffer method
        ClassDB::bind_method(D_METHOD("set_file_buffer", "file_buffer"), &Sf2Resource::set_file_buffer);
        ClassDB::bind_method(D_METHOD("get_file_buffer"), &Sf2Resource::get_file_buffer);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "file_buffer"), "set_file_buffer", "get_file_buffer");
    }

private:
    PackedByteArray file_buffer;

public:
    Error load_file(const String &p_path);
    Error save_file(const String &p_path, const Ref<Resource> &p_resource);

    // getters and setters

    inline PackedByteArray get_file_buffer() const { return this->file_buffer; }
    inline void set_file_buffer(const PackedByteArray &p_file_buffer) { this->file_buffer = p_file_buffer; }
};

#endif // SF2_RESOURCE_H