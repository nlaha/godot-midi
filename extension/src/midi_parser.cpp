#include "midi_parser.h"

#include <godot_cpp/core/class_db.hpp>

#include <algorithm>

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
    // guard against truncated/malformed files that don't even have enough
    // data left for a chunk header (8 bytes: 4 byte id + 4 byte size)
    if (bytes.size() < 8)
    {
        if (bytes.size() > 0)
        {
            UtilityFunctions::printerr("[GodotMidi] Warning: not enough data remaining for a chunk header, ignoring trailing bytes");
        }
        chunk_id = "";
        chunk_size = 0;
        chunk_data = PackedByteArray();
        chunk_type = MidiChunkType::Unknown;
        return PackedByteArray();
    }

    chunk_id = bytes.slice(0, 4).get_string_from_ascii();
    // we have to use this custom function because chunk size is stored in big endian
    chunk_size = Utility::decode_int32_be(bytes, 4);

    // clamp the declared chunk size to the data actually available, rather
    // than reading (or requesting a slice) past the end of the buffer if the
    // file is truncated or the size field is corrupt
    uint32_t available = static_cast<uint32_t>(bytes.size() - 8);
    if (chunk_size > available)
    {
        UtilityFunctions::printerr("[GodotMidi] Warning: chunk '" + chunk_id + "' declares size " + String::num_int64(chunk_size) + " but only " + String::num_int64(available) + " bytes remain, truncating");
        chunk_size = available;
    }

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
    frames_per_second = 0;
    ticks_per_frame = 0;
    tempo = 500000;
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

    // an MThd chunk must contain at least 6 bytes (format, ntrks, division);
    // anything shorter is malformed and can't be safely parsed
    if (raw.chunk_data.size() < 6)
    {
        UtilityFunctions::printerr("[GodotMidi] Error: MThd chunk is too short to contain a valid header");
        return false;
    }

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
        division = static_cast<int32_t>(Utility::decode_int16_be(raw.chunk_data, 4));
    }
    else
    {
        // SMPTE format: byte 4 is a negative signed value giving the frame
        // rate (-24, -25, -29 for 29.97 drop-frame, or -30), byte 5 is the
        // resolution in ticks per frame
        int8_t smpte_format = static_cast<int8_t>(raw.chunk_data[4]);
        frames_per_second = -static_cast<int32_t>(smpte_format);
        ticks_per_frame = static_cast<int32_t>(raw.chunk_data[5]);

        // tempo-based (ticks per quarter note) math doesn't apply in SMPTE
        // mode since tick duration is fixed by the frame rate, not tempo;
        // consumers should check division_type before using `division`
        division = frames_per_second * ticks_per_frame;
    }

    return true;
}

/// @brief constructor
/// @param channel
/// @param delta
MidiParser::MidiEvent::MidiEvent(int32_t channel, double delta)
{
    this->channel = channel;
    this->delta = delta;
}

/// @brief copy constructor
/// @param other
MidiParser::MidiEvent::MidiEvent(const MidiEvent &other)
{
    this->channel = other.channel;
    this->delta = other.delta;
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
    return String("MidiEvent: channel=") + String::num_int64(channel) + String(", delta=") + String::num_int64(delta);
}

/// @brief Constructor for MIDI note events
/// @param channel the MIDI channel
/// @param delta the delta time in microseconds
/// @param data the data for the event
/// @param event_type the type of the event
MidiParser::MidiEventNote::MidiEventNote(int32_t channel, double delta, PackedByteArray data, NoteType event_type) : MidiEvent(channel, delta)
{
    this->event_type = event_type;
    this->channel = channel;
    this->note = 0;
    this->data = 0;

    // data[0] is the status byte; guard against truncated payloads where
    // fewer bytes were actually available than this event type expects
    if (
        event_type == NoteType::NoteOn ||
        event_type == NoteType::NoteOff ||
        event_type == NoteType::Aftertouch ||
        event_type == NoteType::Controller ||
        event_type == NoteType::PitchBend)
    {
        if (data.size() > 1)
        {
            this->note = data[1];
        }
        if (data.size() > 2)
        {
            this->data = data[2];
        }
        this->bytes_used = 2;
    }
    else if (event_type == NoteType::ProgramChange ||
             event_type == NoteType::ChannelPressure)
    {
        if (data.size() > 1)
        {
            this->note = data[1];
        }
        bytes_used = 1;
    }
    else
    {
        // unknown/invalid event type
        // let's assume it's some weird controller event and set the bytes used to 2
        if (data.size() > 1)
        {
            this->note = data[1];
        }
        if (data.size() > 2)
        {
            this->data = data[2];
        }
        this->bytes_used = 2;
    }
}

/// @brief Constructor for MIDI system events
/// @param delta
/// @param data
MidiParser::MidiEventSystem::MidiEventSystem(double delta, PackedByteArray data) : MidiEvent(0, delta)
{
    event_type = (MidiSystemEventType)data[0];

    // number of data bytes following the status byte varies per the MIDI spec:
    //  - system exclusive (0xF0) and the sysex escape/continuation (0xF7) are
    //    followed by a variable length quantity giving the length of the
    //    data that follows
    //  - system common messages have a fixed number of data bytes
    //  - system real-time messages (0xF8-0xFE) have no data bytes at all,
    //    and can legally appear in the middle of another message without
    //    disturbing it
    switch (data[0])
    {
    case MidiSystemEventType::SystemExclusiveStart:
    case MidiSystemEventType::SystemExclusiveEscape:
    {
        int32_t length_bytes = 0;
        int64_t sysex_length = Utility::decode_varint_be(data, 1, length_bytes);
        bytes_used = length_bytes + static_cast<int32_t>(sysex_length);
        break;
    }
    case MidiSystemEventType::MTCQuarterFrame:
    case MidiSystemEventType::SongSelect:
        bytes_used = 1;
        break;
    case MidiSystemEventType::SongPositionPointer:
        bytes_used = 2;
        break;
    case MidiSystemEventType::TuneRequest:
    case MidiSystemEventType::TimingClock:
    case MidiSystemEventType::Start:
    case MidiSystemEventType::Continue:
    case MidiSystemEventType::Stop:
    case MidiSystemEventType::ActiveSensing:
    default:
        // real-time messages (and Reset/0xFF, which never reaches here since
        // it's intercepted as a meta event when parsing SMF tracks) have no
        // data bytes
        bytes_used = 0;
        break;
    }
}

/// @brief Constructor for MIDI meta events
/// @param delta
/// @param data
MidiParser::MidiEventMeta::MidiEventMeta(double delta, PackedByteArray data) : MidiEvent(0, delta)
{
    // the first byte is always 0xFF
    // the second and third bytes are the event type and length
    event_type = data.size() > 1 ? (MidiMetaEventType)data[1] : MidiMetaEventType::TextEvent;
    event_data_length = data.size() > 2 ? Utility::decode_varint_be(data, 2, bytes_used) : 0;

    // if we have a non-zero length, then we have data; clamp to what's
    // actually available in case the source data was truncated
    int32_t data_start = bytes_used + 2;
    if (event_data_length > 0 && data_start < data.size())
    {
        int32_t data_end = data_start + event_data_length;
        if (data_end > data.size())
        {
            data_end = data.size();
        }
        this->data = data.slice(data_start, data_end);
    }
    event_data_length = this->data.size();

    // increment bytes used
    bytes_used = event_data_length + bytes_used + 1;

    // Begin processing the various subtypes of meta events
    // text events
    if (
        this->event_type == MidiEventMeta::MidiMetaEventType::TextEvent ||
        this->event_type == MidiEventMeta::MidiMetaEventType::CopyRightNotice ||
        this->event_type == MidiEventMeta::MidiMetaEventType::SequenceOrTrackName ||
        this->event_type == MidiEventMeta::MidiMetaEventType::InstrumentName ||
        this->event_type == MidiEventMeta::MidiMetaEventType::Lyric ||
        this->event_type == MidiEventMeta::MidiMetaEventType::Marker ||
        this->event_type == MidiEventMeta::MidiMetaEventType::CuePoint ||
        this->event_type == MidiEventMeta::MidiMetaEventType::ProgramName ||
        this->event_type == MidiEventMeta::MidiMetaEventType::DeviceName ||
        this->event_type == MidiEventMeta::MidiMetaEventType::ArtistName)
    {
        // marker
        // variable length
        // first byte is always 0x06
        // second byte is the length of the text
        // the rest of the bytes are the text
        this->meta_data = this->data.slice(0, this->event_data_length).get_string_from_ascii();

        return;
    }

    switch (this->event_type)
    {
    case MidiEventMeta::MidiMetaEventType::SetTempo:
    {
        // set tempo
        // 3 bytes
        // the bytes are the tempo in microseconds per quarter note

        // set tempo of the track
        this->meta_data = Utility::decode_int24_be(this->data, 0);
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
        Dictionary time_signature;
        time_signature["numerator"] = this->data.size() > 0 ? this->data[0] : 4;
        time_signature["denominator"] = this->data.size() > 1 ? (int32_t)pow(2, this->data[1]) : 4;
        time_signature["clocks_per_tick"] = this->data.size() > 2 ? this->data[2] : 24;
        time_signature["num_32nd_notes_per_quarter"] = this->data.size() > 3 ? this->data[3] : 8;
        meta_data = time_signature;
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
        Dictionary key_signature;
        key_signature["sharps_flats"] = this->data.size() > 0 ? this->data[0] : 0;
        key_signature["major_minor"] = this->data.size() > 1 ? this->data[1] : 0;
        meta_data = key_signature;

        break;
    }
    case MidiEventMeta::MidiMetaEventType::EndOfTrack:
    {
        // end of track
        // no data

        // set end of track flag
        this->meta_data = true;
        break;
    }
    case MidiEventMeta::MidiMetaEventType::SequenceNumber:
    case MidiEventMeta::MidiMetaEventType::TextEvent:
    case MidiEventMeta::MidiMetaEventType::CopyRightNotice:
    case MidiEventMeta::MidiMetaEventType::SequenceOrTrackName:
    case MidiEventMeta::MidiMetaEventType::InstrumentName:
    case MidiEventMeta::MidiMetaEventType::Lyric:
    case MidiEventMeta::MidiMetaEventType::Marker:
    case MidiEventMeta::MidiMetaEventType::CuePoint:
    case MidiEventMeta::MidiMetaEventType::ProgramName:
    case MidiEventMeta::MidiMetaEventType::DeviceName:
    case MidiEventMeta::MidiMetaEventType::ArtistName:
    case MidiEventMeta::MidiMetaEventType::SMPTEOffset:
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
    // per the MIDI spec, consecutive channel voice/mode messages may omit
    // a repeated status byte and reuse the previous one ("running status")
    uint8_t running_status = 0;
    while (offset < raw.chunk_size)
    {
        if (header.end_of_track)
            break;

        int32_t bytes_used = 0;

        // first variable length quantity is delta time
        int32_t delta_time = Utility::decode_varint_be(raw.chunk_data, offset, bytes_used);
        offset += bytes_used;

        if (offset >= raw.chunk_size)
        {
            // truncated track: nothing left to read after the delta time
            UtilityFunctions::printerr("[GodotMidi] Warning: track data ended unexpectedly after a delta time, stopping");
            break;
        }

        int32_t event_type;
        bool explicit_status;

        uint8_t next_byte = raw.chunk_data[offset];
        if (next_byte & 0x80)
        {
            // explicit status byte present
            explicit_status = true;
            event_type = next_byte;
            offset += 1;

            if (event_type < 0xF0)
            {
                // channel voice/mode message, updates running status
                running_status = static_cast<uint8_t>(event_type);
            }
            else if (event_type <= 0xF7)
            {
                // system common messages cancel running status
                running_status = 0;
            }
            // system realtime messages (0xF8-0xFE) and meta events (0xFF)
            // don't affect running status
        }
        else
        {
            // running status: this byte is actually the first data byte of
            // a channel voice/mode message, reusing the last status byte
            if (running_status == 0)
            {
                UtilityFunctions::printerr("[GodotMidi] Malformed MIDI track: data byte encountered with no active running status");
                break;
            }

            explicit_status = false;
            event_type = running_status;
        }

        // offset points at the first payload byte in both cases
        // Work out exactly how many payload bytes this event needs *without*
        // copying anything yet, so that we only ever copy the bytes a given
        // event actually needs; slicing to the end of the chunk for every
        // single event would make parsing large tracks quadratic.
        int32_t payload_needed = 0;

        if (event_type == 0xFF)
        {
            // meta event: <type byte> <length varint> <data...>
            int32_t length_bytes = 0;
            int64_t data_len = Utility::decode_varint_be(raw.chunk_data, offset + 1, length_bytes);
            payload_needed = 1 + length_bytes + static_cast<int32_t>(data_len);
        }
        else if (event_type == 0xF0 || event_type == 0xF7)
        {
            // system exclusive / escape: <length varint> <data...>
            int32_t length_bytes = 0;
            int64_t data_len = Utility::decode_varint_be(raw.chunk_data, offset, length_bytes);
            payload_needed = length_bytes + static_cast<int32_t>(data_len);
        }
        else if (event_type < 0xF0)
        {
            // channel voice/mode message
            int32_t code = (event_type >> 4) & 0x0F;
            payload_needed = (code == 0x0C || code == 0x0D) ? 1 : 2;
        }
        else
        {
            // remaining system common/real-time messages
            switch (event_type)
            {
            case 0xF1:
            case 0xF3:
                payload_needed = 1;
                break;
            case 0xF2:
                payload_needed = 2;
                break;
            default: // F6, F8, FA, FB, FC, FE
                payload_needed = 0;
                break;
            }
        }

        // clamp to what's actually available in case of a truncated file
        int32_t remaining_in_chunk = static_cast<int32_t>(raw.chunk_size) - offset;
        if (payload_needed > remaining_in_chunk)
        {
            payload_needed = remaining_in_chunk;
        }
        if (payload_needed < 0)
        {
            payload_needed = 0;
        }

        // build a small, self-contained buffer with the (real or reused)
        // status byte followed by exactly the payload bytes needed for this
        // event; bounded copies keep parsing linear in the size of the track
        PackedByteArray event_data;
        event_data.append(static_cast<uint8_t>(event_type));
        if (payload_needed > 0)
        {
            event_data.append_array(raw.chunk_data.slice(offset, offset + payload_needed));
        }

        // the event type is the first 4 bits of the byte
        // the channel is the last 4 bits
        int32_t event_code = event_type >> 4;
        int32_t channel = event_type & 0x0F;

        // special case for meta events
        if (event_type == 0xFF)
        {
            event_code = 0xFF;
        }

        // the event code determines the type of the event
        std::unique_ptr<MidiEvent> ptr;
        switch (event_code)
        {
        case 0x08: // note off
        {
            MidiEventNote note_off_event = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::NoteOff);
            offset += note_off_event.get_bytes_used();
            note_events.push_back(note_off_event);
            ptr = std::make_unique<MidiEventNote>(note_off_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x09: // note on
        {
            MidiEventNote note_on_event = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::NoteOn);
            offset += note_on_event.get_bytes_used();
            note_events.push_back(note_on_event);
            ptr = std::make_unique<MidiEventNote>(note_on_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0A: // note aftertouch
        {
            MidiEventNote note_aftertouch_event = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::Aftertouch);
            offset += note_aftertouch_event.get_bytes_used();
            note_events.push_back(note_aftertouch_event);
            ptr = std::make_unique<MidiEventNote>(note_aftertouch_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0B: // controller
        {
            MidiEventNote note_controller_event = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::Controller);
            offset += note_controller_event.get_bytes_used();
            note_events.push_back(note_controller_event);
            ptr = std::make_unique<MidiEventNote>(note_controller_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0C: // program change
        {
            MidiEventNote note_program_change = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::ProgramChange);
            offset += note_program_change.get_bytes_used();
            note_events.push_back(note_program_change);
            ptr = std::make_unique<MidiEventNote>(note_program_change);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0D: // channel pressure
        {
            MidiEventNote note_channel_pressure = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::ChannelPressure);
            offset += note_channel_pressure.get_bytes_used();
            note_events.push_back(note_channel_pressure);
            ptr = std::make_unique<MidiEventNote>(note_channel_pressure);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0E: // pitch bend
        {
            MidiEventNote note_pitch_blend = MidiEventNote(channel, delta_time, event_data, MidiEventNote::NoteType::PitchBend);
            offset += note_pitch_blend.get_bytes_used();
            note_events.push_back(note_pitch_blend);
            ptr = std::make_unique<MidiEventNote>(note_pitch_blend);
            events.push_back(std::move(ptr));
            break;
        }
        case 0x0F: // system event
        {
            MidiEventSystem system_event = MidiEventSystem(delta_time, event_data);
            offset += system_event.get_bytes_used();
            system_events.push_back(system_event);
            ptr = std::make_unique<MidiEventSystem>(system_event);
            events.push_back(std::move(ptr));
            break;
        }
        case 0xFF: // meta event
        {
            MidiEventMeta meta_event = MidiEventMeta(delta_time, event_data);
            offset += meta_event.get_bytes_used();
            meta_events.push_back(meta_event);

            if (meta_event.event_type == MidiEventMeta::MidiMetaEventType::EndOfTrack)
            {
                header.end_of_track = true;
            }

            ptr = std::make_unique<MidiEventMeta>(meta_event);
            events.push_back(std::move(ptr));
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
