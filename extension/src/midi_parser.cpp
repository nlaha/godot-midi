#include "midi_parser.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

/// @brief override method for registering c++ functions in godot
void MidiParser::_bind_methods()
{
}

MidiParser::MidiParser()
{
}

MidiParser::~MidiParser()
{
}

/// @brief Loads a Midi chunk from a stream of bytes
/// @param bytes the input stream of bytes
/// @return the original byte stream minus the read data
PackedByteArray MidiParser::RawMidiChunk::load_from_bytes(PackedByteArray bytes)
{
    chunk_id = bytes.slice(0, 4).get_string_from_ascii();
    // we have to use this custom function because chunk size is stored in big endian
    chunk_size = Utility::decode_int32_be(bytes, 4);
    chunk_data = bytes.slice(8, 8 + chunk_size);
    // remove the chunk from the input stream
    PackedByteArray new_bytes = bytes.slice(8 + chunk_size);

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

/// @brief default constructor for header
MidiParser::MidiHeaderChunk::MidiHeaderChunk()
{
    file_format = MidiFileFormat::SingleTrack;
    num_tracks = 0;
    division_type = MidiDivisionType::TicksPerQuarterNote;
    division = 48;
    tempo = 500000;
    end_of_track = false;
    only_notes = false;
}

/// @brief parses a chunk of raw bytes into a header chunk
/// @param raw the raw chunk of bytes
/// @param header the header chunk to populate
/// @return true if the chunk was parsed successfully, false otherwise
bool MidiParser::MidiHeaderChunk::parse_chunk(RawMidiChunk raw, MidiHeaderChunk &header)
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
/// @param delta_microseconds
MidiParser::MidiEvent::MidiEvent(int32_t channel, double delta_microseconds)
{
    this->channel = channel;
    this->delta_microseconds = delta_microseconds;
}

/// @brief copy constructor
/// @param other
MidiParser::MidiEvent::MidiEvent(const MidiEvent &other)
{
    this->channel = other.channel;
    this->delta_microseconds = other.delta_microseconds;
    this->bytes_used = other.bytes_used;
}

/// @brief gets the number of bytes used by the event
/// @return
int32_t MidiParser::MidiEvent::get_bytes_used() const
{
    return this->bytes_used;
}

/// @brief prints the contents of the event to a nice string
/// @return
String MidiParser::MidiEvent::to_string() const
{
    return String("MidiEvent: channel=") + String::num_int64(channel) + String(", delta_microseconds=") + String::num_int64(delta_microseconds);
}

/// @brief Constructor for MIDI note events
/// @param channel the MIDI channel
/// @param delta_microseconds the delta time in microseconds
/// @param data the data for the event
/// @param event_type the type of the event
MidiParser::MidiEventNote::MidiEventNote(int32_t channel, double delta_microseconds, PackedByteArray data, NoteType event_type) : MidiEvent(channel, delta_microseconds)
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
        bytes_used = 1;
    }
    else
    {
        // unknown/invalid event type
        // let's assume it's some weird controller event and set the bytes used to 2
        this->channel = channel;
        this->note = data[1];
        this->data = data[2];
        this->bytes_used = 2;
    }
}

/// @brief Constructor for MIDI system events
/// @param delta_microseconds
/// @param data
MidiParser::MidiEventSystem::MidiEventSystem(double delta_microseconds, PackedByteArray data) : MidiEvent(0, delta_microseconds)
{
    event_type = (MidiSystemEventType)data[0];
    bytes_used = 1;
}

/// @brief Constructor for MIDI meta events
/// @param delta_microseconds
/// @param data
MidiParser::MidiEventMeta::MidiEventMeta(double delta_microseconds, PackedByteArray data) : MidiEvent(0, delta_microseconds)
{
    // the first byte is always 0xFF
    // the second and third bytes are the event type and length
    // UtilityFunctions::print(data.slice(0, 2).hex_encode());
    event_type = (MidiMetaEventType)data[1];
    event_data_length = Utility::decode_varint_be(data, 2, bytes_used);
    // print data length
    // UtilityFunctions::print(String::num_int64(event_data_length));

    if (event_data_length > 0)
    {
        this->data = data.slice(bytes_used + 2, (bytes_used + 2) + event_data_length);
    }

    bytes_used = event_data_length + bytes_used + 1;
    // UtilityFunctions::print(String("NEW META EVENT: "));
    // UtilityFunctions::print(String("Event type: ") + String::num_int64(event_type));
    // UtilityFunctions::print(String("Bytes used: ") + String::num_int64(bytes_used));
}

/// @brief Parses the different meta event types
/// @param meta_event the meta event to parse
/// @param header the header chunk (will be modified for tempo changes, etc.)
void MidiParser::MidiTrackChunk::IngestMetaEvent(MidiEventMeta &meta_event, MidiHeaderChunk &header)
{
    // text events
    if (
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::TextEvent ||
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::CopyRightNotice ||
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::SequenceOrTrackName ||
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::InstrumentName ||
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::Lyric ||
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::Marker ||
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::CuePoint ||
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::ProgramName ||
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::DeviceName ||
        meta_event.event_type == MidiEventMeta::MidiMetaEventType::ArtistName)
    {
        // marker
        // variable length
        // first byte is always 0x06
        // second byte is the length of the text
        // the rest of the bytes are the text
        meta_event.text = meta_event.data.slice(0, meta_event.event_data_length).get_string_from_ascii();

        return;
    }

    switch (meta_event.event_type)
    {
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
    case MidiEventMeta::MidiMetaEventType::KeySignature:
    {
        // key signature
        // 2 bytes
        // first byte is always 0x02
        // the rest of the bytes are the key signature
        // the first byte is the number of flats (-ve) or sharps (+ve)
        // the second byte is the major (0) or minor (1) key

        // set key signature of the track
        MidiKeySignature key_signature = MidiKeySignature();
        key_signature.sharps_flats = meta_event.data[0];
        key_signature.major_minor = meta_event.data[1];
        this->key_signature = key_signature;
        break;
    }
    case MidiEventMeta::MidiMetaEventType::EndOfTrack:
    {
        // end of track
        // no data

        // set end of track flag
        header.end_of_track = true;
    }
    default:
    {
        // unknown meta event
        break;
    }
    }
}

/// @brief The main chunk parser, takes bytes from the input stream and parses them into MIDI chunks
/// @param raw the raw chunk of bytes
/// @param header the header chunk (will be modified for tempo changes, etc.)
/// @return
bool MidiParser::MidiTrackChunk::parse_chunk(RawMidiChunk raw, MidiHeaderChunk &header)
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
        
        // compute final delta time (with tempo applied)
        double tick_duration = (double)header.tempo / (double)header.division;
        double delta_microseconds = (double)delta_time * tick_duration;
        
        // print event code
        // UtilityFunctions::print(String("EVENT: ") + String::num_int64(event_code));

        PackedByteArray event_data = raw.chunk_data.slice(offset - 1, raw.chunk_size);

        // the event code determines the type of the event
        std::unique_ptr<MidiEvent> ptr;
        switch (event_code)
        {
        case 0x08: // note off
        {
            MidiEventNote note_off_event = MidiEventNote(channel, delta_microseconds, event_data, MidiEventNote::NoteType::NoteOff);
            offset += note_off_event.get_bytes_used();
            note_events.push_back(note_off_event);
            ptr = std::make_unique<MidiEventNote>(note_off_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x09: // note on
        {
            MidiEventNote note_on_event = MidiEventNote(channel, delta_microseconds, event_data, MidiEventNote::NoteType::NoteOn);
            offset += note_on_event.get_bytes_used();
            note_events.push_back(note_on_event);
            ptr = std::make_unique<MidiEventNote>(note_on_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0A: // note aftertouch
        {
            MidiEventNote note_aftertouch_event = MidiEventNote(channel, delta_microseconds, event_data, MidiEventNote::NoteType::Aftertouch);
            offset += note_aftertouch_event.get_bytes_used();
            note_events.push_back(note_aftertouch_event);
            ptr = std::make_unique<MidiEventNote>(note_aftertouch_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0B: // controller
        {
            MidiEventNote note_controller_event = MidiEventNote(channel, delta_microseconds, event_data, MidiEventNote::NoteType::Controller);
            offset += note_controller_event.get_bytes_used();
            note_events.push_back(note_controller_event);
            ptr = std::make_unique<MidiEventNote>(note_controller_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0C: // program change
        {
            MidiEventNote note_program_change = MidiEventNote(channel, delta_microseconds, event_data, MidiEventNote::NoteType::ProgramChange);
            offset += note_program_change.get_bytes_used();
            note_events.push_back(note_program_change);
            ptr = std::make_unique<MidiEventNote>(note_program_change);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0D: // channel pressure
        {
            MidiEventNote note_channel_pressure = MidiEventNote(channel, delta_microseconds, event_data, MidiEventNote::NoteType::ChannelPressure);
            offset += note_channel_pressure.get_bytes_used();
            note_events.push_back(note_channel_pressure);
            ptr = std::make_unique<MidiEventNote>(note_channel_pressure);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0E: // pitch bend
        {
            MidiEventNote note_pitch_blend = MidiEventNote(channel, delta_microseconds, event_data, MidiEventNote::NoteType::PitchBend);
            offset += note_pitch_blend.get_bytes_used();
            note_events.push_back(note_pitch_blend);
            ptr = std::make_unique<MidiEventNote>(note_pitch_blend);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0F: // system event
        {
            MidiEventSystem system_event = MidiEventSystem(delta_microseconds, event_data);
            offset += system_event.get_bytes_used();
            system_events.push_back(system_event);
            ptr = std::make_unique<MidiEventSystem>(system_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0xFF: // meta event
        {
            MidiEventMeta meta_event = MidiEventMeta(delta_microseconds, event_data);
            offset += meta_event.get_bytes_used();
            meta_events.push_back(meta_event);
            // Ingest the meta event, this parsers the meta event and updates the header
            // for things like tempo changes and time signature changes
            this->IngestMetaEvent(meta_event, header);
            ptr = std::make_unique<MidiEventMeta>(meta_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0xF0: // system exclusive
        {
            int bytes_used = 0;
            int length = Utility::decode_varint_be(event_data, 0, bytes_used);
            UtilityFunctions::print(String("System exclusive event, length: ") + String::num_int64(length));
            offset += length + bytes_used;
            break;
        }
        default:
        {
            // unknown event type
            UtilityFunctions::print(String("Unknown event type: ") + String::num_int64(event_code));
            // print data as hex
            UtilityFunctions::print(event_data.slice(0, 10).hex_encode() + String("..."));
            //  print delta time (parsed)
            UtilityFunctions::print(String("Delta time: ") + String::num_int64(delta_time));
        }
        }
    }

    return true;
}
