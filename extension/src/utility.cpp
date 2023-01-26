#include "utility.h"

/// @brief decode bytes to int from 32 bit big endian stream
/// @param bytes
/// @param offset
/// @return
int32_t Utility::decode_int32_be(PackedByteArray bytes, int32_t offset)
{
    return (bytes[offset] << 24) | (bytes[offset + 1] << 16) | (bytes[offset + 2] << 8) | (bytes[offset + 3]);
}

/// @brief decode bytes to int from 16 bit big endian stream
/// @param bytes
/// @param offset
/// @return
int16_t Utility::decode_int16_be(PackedByteArray bytes, int32_t offset)
{
    return (bytes[offset] << 8) | (bytes[offset + 1]);
}

/// @brief decode bytes to int from variable big endian stream
/// note this is unsigned
/// @param bytes
/// @param offset
/// @param length [out] length of varint
/// @return
int64_t Utility::decode_varint_be(PackedByteArray bytes, int32_t offset, int32_t &length)
{
    int32_t value = 0;
    uint8_t byte = 0;
    int i = offset;
    length = 1;
    if ((value = bytes[i]) & 0x80)
    {
        value &= 0x7f;
        do
        {
            value = (value << 7) + ((byte = bytes[++i]) & 0x7f);
            length++;
        } while (byte & 0x80);
    }

    return value;
}

/// @brief decode bytes to int from 24 bit big endian stream
/// @param bytes
/// @param offset
/// @return
int32_t Utility::decode_int24_be(PackedByteArray bytes, int32_t offset)
{
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
