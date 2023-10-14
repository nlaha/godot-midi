#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "midi_parser.h"

using namespace godot;

TEST_CASE("Test parsing of MIDI header chunks")
{
    // MidiParser::RawMidiChunk chunk;

    uint8_t raw_midi_header_bytes[] = {
        0x4D, 0x54, 0x68, 0x64, // chunk_id
        0x00, 0x00, 0x00, 0x06, // chunk_size
        0x00, 0x01,             // format
        0x00, 0x02,             // number_of_tracks
        0x04, 0x00,             // time_division
    };

    // load into byte array
    PackedByteArray raw_midi_header_ba;
    // raw_midi_header_ba.resize(sizeof(raw_midi_header_bytes));
    // memcpy(raw_midi_header_ba.ptrw(), raw_midi_header_bytes, sizeof(raw_midi_header_bytes));

    // // parse
    // chunk.load_from_bytes(raw_midi_header_ba);

    // // check
    // CHECK(chunk.chunk_type == MidiParser::MidiChunkType::Header);
    // CHECK(chunk.chunk_size == 6);
    // CHECK(chunk.chunk_data.size() == 6);

    // MidiParser::MidiHeaderChunk header_chunk;

    // header_chunk.parse_chunk(chunk, header_chunk);

    // CHECK(header_chunk.file_format == MidiParser::MidiHeaderChunk::MidiFileFormat::MultipleSimultaneousTracks);
    // CHECK(header_chunk.num_tracks == 2);
    // CHECK(header_chunk.division == 1024);
    // CHECK(header_chunk.division_type == MidiParser::MidiHeaderChunk::MidiDivisionType::TicksPerQuarterNote);
}