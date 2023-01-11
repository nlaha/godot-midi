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
/// @param bytes
/// @param offset
/// @param length [out] length of varint
/// @return
int64_t Utility::decode_varint_be(PackedByteArray bytes, int32_t offset, int32_t &length)
{
    int value = 0;
    length = 0;
    while (true)
    {
        value = (value << 7) | (bytes[offset] & 0x7F);
        length++;
        if ((bytes[offset] & 0x80) == 0)
            break;
        offset++;
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
