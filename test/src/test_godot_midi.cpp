#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <godot_cpp/variant/string.hpp>
#include <midi_parser.h>

using namespace godot;

TEST_CASE("Test can parse midi header") {
	PackedByteArray* midi_data = memnew(PackedByteArray({0x4D, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, 0x02, 0x00, 0x30}));
	
    // MidiParser::RawMidiChunk header_chunk;
    // midi_data = header_chunk.load_from_bytes(midi_data);
	
	CHECK_EQ(midi_data->size(), 14);
}