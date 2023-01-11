#ifndef MIDI_UTILITY_H
#define MIDI_UTILITY_H

#include <godot_cpp/variant/packed_byte_array.hpp>

using namespace godot;

// utility class

class Utility
{
public:
    static int32_t decode_int32_be(PackedByteArray bytes, int32_t offset);
    static int16_t decode_int16_be(PackedByteArray bytes, int32_t offset);
    static int64_t decode_varint_be(PackedByteArray bytes, int32_t offset, int32_t &length);
    static int32_t decode_int24_be(PackedByteArray bytes, int32_t offset);
};

#endif // MIDI_UTILITY_H