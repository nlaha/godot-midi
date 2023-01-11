#include "midi_parser.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

/// @brief override method for registering c++ functions in godot
void MIDIParser::_bind_methods()
{
}

MIDIParser::MIDIParser()
{
}

MIDIParser::~MIDIParser()
{
}

/// @brief Loads a MIDI chunk from a stream of bytes
/// @param bytes the input stream of bytes
/// @return the original byte stream minus the read data
PackedByteArray MIDIParser::RawMidiChunk::load_from_bytes(PackedByteArray bytes)
{
    chunk_id = String((char *)bytes.slice(0, 4).ptr());
    // we have to use this custom function because chunk size is stored in big endian
    chunk_size = Utility::decode_int32_be(bytes, 4);
    chunk_data = bytes.slice(8, chunk_size);
    // remove the chunk from the input stream
    PackedByteArray new_bytes = bytes.slice(chunk_size + 8, bytes.size() - chunk_size - 8);

    if (chunk_id == "MThd")
    {
        chunk_type = MidiChunkType::Header;
    }
    else if (chunk_id == "MTrk")
    {
        chunk_type = MidiChunkType::Track;
    }
    else
    {
        // unknown header type
        // this will be ignored per the midi specification
        chunk_type = MidiChunkType::Unknown;
    }

    return new_bytes;
}

MIDIParser::MidiHeaderChunk::MidiHeaderChunk()
{
    file_format = MidiFileFormat::SingleTrack;
    num_tracks = 0;
    division_type = MidiDivisionType::TicksPerQuarterNote;
    division = 0;
    tempo = 500000;
    end_of_track = false;
}

bool MIDIParser::MidiHeaderChunk::parse_chunk(RawMidiChunk raw, MidiHeaderChunk &header)
{
    if (raw.chunk_type != MidiChunkType::Header)
        return false;

    // data section of a header contains 3 16-bit words
    // first word is format
    // 0 - single track, 1 - multiple tracks, 2 - multiple songs
    file_format = (MidiFileFormat)Utility::decode_int16_be(raw.chunk_data, 0);

    // second word is number of tracks
    num_tracks = Utility::decode_int16_be(raw.chunk_data, 2);

    // third word is time division
    // ticks per quarter note or (negative SMPTE format + ticks per frame)
    // if bit 15 is 0, then it's ticks per quarter note
    // if bit 15 is 1, then it's SMPTE format

    // we use bit 15 to determine the division type
    // the bytes are in big endian, so we have to shift the byte to the right
    // and then mask it with 0x01 to get the bit
    division_type = (MidiDivisionType)((raw.chunk_data[4] >> 7) & 0x01);

    // if it's ticks per quarter note, then we just read the value
    if (division_type == MidiDivisionType::TicksPerQuarterNote)
    {
        division = Utility::decode_int16_be(raw.chunk_data, 4);
    }
    else
    {
        // TODO: implement SMPTE format
    }

    return true;
}

/// @brief constructor
/// @param channel
/// @param delta_time
MIDIParser::MidiEvent::MidiEvent(int32_t channel, int32_t delta_time)
{
    this->channel = channel;
    this->delta_time = delta_time;
}

/// @brief copy constructor
/// @param other
MIDIParser::MidiEvent::MidiEvent(const MidiEvent &other)
{
    this->channel = other.channel;
    this->delta_time = other.delta_time;
    this->bytes_used = other.bytes_used;
}

int32_t MIDIParser::MidiEvent::get_bytes_used() const
{
    return this->bytes_used;
}

/// @brief prints the contents of the event to a nice string
/// @return
String MIDIParser::MidiEvent::to_string() const
{
    return String("MidiEvent: channel=") + String::num_int64(channel) + String(", delta_time=") + String::num_int64(delta_time);
}

MIDIParser::MidiEventNote::MidiEventNote(int32_t channel, int32_t delta_time, PackedByteArray data, NoteType event_type) : MidiEvent(channel, delta_time)
{
    this->event_type = event_type;

    if (
        event_type == NoteType::NoteOn ||
        event_type == NoteType::NoteOff ||
        event_type == NoteType::Aftertouch ||
        event_type == NoteType::Controller ||
        event_type == NoteType::PitchBend)
    {
        this->channel = channel;
        this->note = data[1];
        this->data = data[2];
        this->bytes_used = 2;
    }
    else if (event_type == NoteType::ProgramChange ||
             event_type == NoteType::ChannelPressure)
    {
        this->channel = channel;
        this->note = data[1];
        bytes_used = 2;
    }
    else
    {
        // unknown/invalid event type
    }
}

MIDIParser::MidiEventSystem::MidiEventSystem(int32_t delta_time, PackedByteArray data) : MidiEvent(0, delta_time)
{
    event_type = (MidiSystemEventType)data[0];
    bytes_used = 1;
}

MIDIParser::MidiEventMeta::MidiEventMeta(int32_t delta_time, PackedByteArray data) : MidiEvent(0, delta_time)
{
    // the first byte is always 0xFF
    // the second and third bytes are the event type and length
    event_type = (MidiMetaEventType)data[1];
    event_data_length = Utility::decode_varint_be(data, 2, bytes_used);
    data = data.slice(bytes_used + 2, event_data_length);
    bytes_used = event_data_length + bytes_used + 1;
}

void MIDIParser::MidiTrackChunk::IngestMetaEvent(MidiEventMeta meta_event, MidiHeaderChunk &header)
{
    switch (meta_event.event_type)
    {
    case MidiEventMeta::MidiMetaEventType::Marker:
    {
        // marker
        // variable length
        // first byte is always 0x06
        // second byte is the length of the text
        // the rest of the bytes are the text

        // TODO: implement markers
        break;
    }
    case MidiEventMeta::MidiMetaEventType::SetTempo:
    {
        // set tempo
        // 3 bytes
        // the bytes are the tempo in microseconds per quarter note

        // set tempo of the track
        header.tempo = Utility::decode_int24_be(meta_event.data, 0);
        break;
    }
    case MidiEventMeta::MidiMetaEventType::TimeSignature:
    {
        // time signature
        // 4 bytes
        // first byte is always 0x04
        // the rest of the bytes are the time signature
        // the first byte is the numerator
        // the second byte is the denominator (2^x)
        // the third byte is the clocks per metronome click
        // the fourth byte is the number of 32nd notes per quarter note

        // set time signature of the track
        MidiTimeSignature time_signature = MidiTimeSignature();
        time_signature.numerator = meta_event.data[0];
        time_signature.denominator = (int32_t)pow(2, meta_event.data[1]);
        time_signature.clocks_per_tick = meta_event.data[2];
        time_signature.num_32nd_notes_per_quarter = meta_event.data[3];
        this->time_signature = time_signature;
        break;
    }
    case MidiEventMeta::MidiMetaEventType::EndOfTrack:
    {
        // end of track
        // no data

        // set end of track flag
        header.end_of_track = true;
    }
    }
}

bool MIDIParser::MidiTrackChunk::parse_chunk(RawMidiChunk raw, MidiHeaderChunk &header)
{
    if (raw.chunk_type != MidiChunkType::Track)
        return false;

    // read events until we reach the end of the chunk
    // we have to use a while loop because we don't know how many events there are
    // we can only know when we reach the end of the chunk
    int32_t offset = 0;
    header.end_of_track = false;
    while (offset < raw.chunk_size)
    {
        if (header.end_of_track)
            break;

        int32_t bytes_used = 0;
        // first variable length quantity is delta time
        int32_t delta_time = Utility::decode_varint_be(raw.chunk_data, offset, bytes_used);
        offset += bytes_used;

        // next byte is the event type | channel
        int32_t event_type = raw.chunk_data[offset];
        offset += 1;

        // the event type is the first 4 bits of the byte
        // the channel is the last 4 bits
        int32_t event_code = event_type >> 4;
        int32_t channel = event_type & 0x0F;

        // special case for meta events
        if (event_type == 0xFF)
        {
            event_code = 0xFF;
        }

        PackedByteArray event_data = raw.chunk_data.slice(offset - 1, raw.chunk_size - (offset - 1));

        // the event code determines the type of the event
        std::unique_ptr<MidiEvent> ptr;
        switch (event_code)
        {
        case 0x08: // note off
        {
            MidiEventNote note_off_event = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::NoteOff);
            offset += note_off_event.get_bytes_used();
            note_events.push_back(note_off_event);
            ptr = std::make_unique<MidiEvent>(note_off_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x09: // note on
        {
            MidiEventNote note_on_event = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::NoteOn);
            offset += note_on_event.get_bytes_used();
            note_events.push_back(note_on_event);
            ptr = std::make_unique<MidiEvent>(note_on_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0A: // note aftertouch
        {
            MidiEventNote note_aftertouch_event = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::Aftertouch);
            offset += note_aftertouch_event.get_bytes_used();
            note_events.push_back(note_aftertouch_event);
            ptr = std::make_unique<MidiEvent>(note_aftertouch_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0B: // controller
        {
            MidiEventNote note_controller_event = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::Controller);
            offset += note_controller_event.get_bytes_used();
            note_events.push_back(note_controller_event);
            ptr = std::make_unique<MidiEvent>(note_controller_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0C: // program change
        {
            MidiEventNote note_program_change = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::ProgramChange);
            offset += note_program_change.get_bytes_used();
            note_events.push_back(note_program_change);
            ptr = std::make_unique<MidiEvent>(note_program_change);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0D: // channel pressure
        {
            MidiEventNote note_channel_pressure = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::ChannelPressure);
            offset += note_channel_pressure.get_bytes_used();
            note_events.push_back(note_channel_pressure);
            ptr = std::make_unique<MidiEvent>(note_channel_pressure);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0E: // pitch bend
        {
            MidiEventNote note_pitch_blend = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::PitchBend);
            offset += note_pitch_blend.get_bytes_used();
            note_events.push_back(note_pitch_blend);
            ptr = std::make_unique<MidiEvent>(note_pitch_blend);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0F: // system event
        {
            MidiEventSystem system_event = MidiEventSystem(delta_time, event_data);
            offset += system_event.get_bytes_used();
            system_events.push_back(system_event);
            ptr = std::make_unique<MidiEvent>(system_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0xFF: // meta event
        {
            MidiEventMeta meta_event = MidiEventMeta(delta_time, event_data);
            offset += meta_event.get_bytes_used();
            IngestMetaEvent(meta_event, header);
            meta_events.push_back(meta_event);
            ptr = std::make_unique<MidiEvent>(meta_event);
            events.push_back(std::move(ptr));
            break;
        }
        }
    }
}
