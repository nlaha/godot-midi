#include "utility.h"

/// @brief decode bytes to int from 32 bit big endian stream
/// @param bytes
/// @param offset
/// @return
int32_t Utility::decode_int32_be(PackedByteArray bytes, int32_t offset)
{
    if (offset < 0 || offset + 4 > bytes.size())
    {
        return 0;
    }
    return (bytes[offset] << 24) | (bytes[offset + 1] << 16) | (bytes[offset + 2] << 8) | (bytes[offset + 3]);
}

/// @brief decode bytes to int from 16 bit big endian stream
/// @param bytes
/// @param offset
/// @return
int16_t Utility::decode_int16_be(PackedByteArray bytes, int32_t offset)
{
    if (offset < 0 || offset + 2 > bytes.size())
    {
        return 0;
    }
    return (bytes[offset] << 8) | (bytes[offset + 1]);
}

/// @brief decode bytes to int from variable big endian stream
/// note this is unsigned
/// per the MIDI spec, variable length quantities are at most 4 bytes (28 bits);
/// this function is also bounds-checked against the array so that malformed
/// or truncated MIDI data can never cause an out-of-bounds read
/// @param bytes
/// @param offset
/// @param length [out] length of varint (number of bytes consumed, 0 if offset was out of bounds)
/// @return
int64_t Utility::decode_varint_be(PackedByteArray bytes, int32_t offset, int32_t &length)
{
    int64_t value = 0;
    length = 0;

    if (offset < 0 || offset >= bytes.size())
    {
        return 0;
    }

    const int32_t max_bytes = 4;
    uint8_t byte = 0;
    int32_t i = offset;

    do
    {
        if (i >= bytes.size())
        {
            // truncated/malformed data, stop reading rather than going out of bounds
            break;
        }

        byte = bytes[i];
        value = (value << 7) | (byte & 0x7f);
        length++;
        i++;
    } while ((byte & 0x80) && length < max_bytes);

    return value;
}

/// @brief decode bytes to int from 24 bit big endian stream
/// @param bytes
/// @param offset
/// @return
int32_t Utility::decode_int24_be(PackedByteArray bytes, int32_t offset)
{
    if (offset < 0 || offset + 3 > bytes.size())
    {
        return 0;
    }
    return (bytes[offset] << 16) | (bytes[offset + 1] << 8) | bytes[offset + 2];
}

String Utility::print_bits(PackedByteArray bytes)
{
    // print bits
    String bits = "";
    for (int i = 0; i < bytes.size(); i++)
    {
        for (int j = 7; j >= 0; j--)
        {
            bits = bits + String(((bytes[i] >> j) & 1) ? "1" : "0");
        }
        bits = bits + String(" ");
    }

    return bits;
}
