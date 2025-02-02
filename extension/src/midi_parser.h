#ifndef MIDI_PARSER_CLASS_H
#define MIDI_PARSER_CLASS_H

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <memory>
#include "utility.h"

using namespace godot;

/// @brief MidiParser class, contains various classes and functions for parsing midi files
class MidiParser : public RefCounted
{
    GDCLASS(MidiParser, RefCounted);

protected:
    static void _bind_methods();

public:
    enum MidiChunkType
    {
        Header,
        Track,
        Unknown
    };

    class RawMidiChunk
    {
    public:
        String chunk_id;
        uint32_t chunk_size;
        PackedByteArray chunk_data;
        MidiChunkType chunk_type;

        RawMidiChunk()
        {
            chunk_id = "";
            chunk_size = 0;
            chunk_type = MidiChunkType::Unknown;
        };

        PackedByteArray load_from_bytes(PackedByteArray bytes);
    };

    class MidiHeaderChunk
    {
    public:
        enum MidiFileFormat
        {
            SingleTrack = 0,
            MultipleSimultaneousTracks = 1,
            MultipleIndependentTracks = 2
        };

        enum MidiDivisionType
        {
            TicksPerQuarterNote = 0,
            FramesPerSecond = 1
        };

        MidiFileFormat file_format;
        int32_t num_tracks;
        MidiDivisionType division_type;
        int32_t division;
        int32_t tempo;
        bool end_of_track;
        bool only_notes;

        MidiHeaderChunk();
        bool parse_chunk(RawMidiChunk raw, MidiHeaderChunk &header);
    };

    class MidiChunk
    {
    public:
        virtual bool parse_chunk(RawMidiChunk raw, MidiHeaderChunk &header) = 0;
    };

    class MidiEvent
    {
    public:
        enum EventType
        {
            Note,
            Meta,
            System
        };

        int32_t channel;
        double delta;

        virtual ~MidiEvent() = default;
        MidiEvent(int32_t channel, double delta);
        MidiEvent(const MidiEvent &other);

        virtual int32_t get_bytes_used() const;

        String to_string() const;

        virtual EventType get_type() const = 0;

    protected:
        int32_t bytes_used;
    };

    class MidiEventNote : public MidiEvent
    {
    public:
        enum NoteType
        {
            NoteOn = 0x09,
            NoteOff = 0x08,
            Aftertouch = 0x0A,
            Controller = 0x0B,
            ProgramChange = 0x0C,
            ChannelPressure = 0x0D,
            PitchBend = 0x0E,
            Unknown = 0xFF
        };

        uint8_t note;
        uint8_t data;
        NoteType event_type;

        MidiEventNote(int32_t channel, double delta_time, PackedByteArray data, NoteType event_type);

        MidiEventNote(const MidiEventNote &other) : MidiEvent(other)
        {
            note = other.note;
            data = other.data;
            event_type = other.event_type;
        }

        EventType get_type() const override
        {
            return MidiEvent::EventType::Note;
        };
    };

    class MidiEventSystem : public MidiEvent
    {
    public:
        enum MidiSystemEventType
        {
            TimingClock = 0xF8,
            Start = 0xFA,
            Continue = 0xFB,
            Stop = 0xFC,
            ActiveSensing = 0xFE,
            Reset = 0xFF
        };

        MidiSystemEventType event_type;

        MidiEventSystem(double delta_time, PackedByteArray data);

        MidiEventSystem(const MidiEventSystem &other) : MidiEvent(other)
        {
            event_type = other.event_type;
        }

        EventType get_type() const override
        {
            return MidiEvent::EventType::System;
        };
    };

    class MidiEventMeta : public MidiEvent
    {
    public:
        enum MidiMetaEventType
        {
            SequenceNumber = 0x00,
            TextEvent = 0x01,
            CopyRightNotice = 0x02,
            SequenceOrTrackName = 0x03,
            InstrumentName = 0x04,
            Lyric = 0x05,
            Marker = 0x06,
            CuePoint = 0x07,
            ProgramName = 0x08,
            DeviceName = 0x09,
            ArtistName = 0x0A, // note, this isn't in the spec
            EndOfTrack = 0x2F,
            SetTempo = 0x51,
            SMPTEOffset = 0x54,
            TimeSignature = 0x58,
            KeySignature = 0x59,
        };

        MidiMetaEventType event_type;
        PackedByteArray data;
        int32_t event_data_length;

        // the actual processed data
        Variant meta_data;

        MidiEventMeta(double delta_time, PackedByteArray data);
        MidiEventMeta(const MidiEventMeta &other) : MidiEvent(other)
        {
            event_type = other.event_type;
            data = other.data;
            event_data_length = other.event_data_length;
            meta_data = other.meta_data;
        }

        EventType get_type() const override
        {
            return MidiEvent::EventType::Meta;
        };
    };

    class MidiTrackChunk : MidiChunk
    {
    public:
        struct MidiTimeSignature
        {
            int32_t numerator;
            int32_t denominator;
            int32_t clocks_per_tick;
            int32_t num_32nd_notes_per_quarter;
        };
        struct MidiKeySignature
        {
            int32_t sharps_flats;
            int32_t major_minor;
        };

        enum MidiEventType
        {
            Note,
            Meta,
            System
        };

        std::vector<MidiEventNote> note_events;
        std::vector<MidiEventMeta> meta_events;
        std::vector<MidiEventSystem> system_events;
        std::vector<std::unique_ptr<MidiEvent>> events;

        MidiTimeSignature time_signature;
        MidiKeySignature key_signature;

        MidiTrackChunk()
        {
            note_events = std::vector<MidiEventNote>();
            meta_events = std::vector<MidiEventMeta>();
            system_events = std::vector<MidiEventSystem>();
            events = std::vector<std::unique_ptr<MidiEvent>>();

            time_signature = {
                4,
                4,
                24,
                8};
        }

        void IngestMetaEvent(MidiEventMeta &meta_event, MidiHeaderChunk &header);
        bool parse_chunk(RawMidiChunk raw, MidiHeaderChunk &header);
    };

    MidiParser();
    ~MidiParser();
};

#endif // MIDI_PARSER_CLASS_H