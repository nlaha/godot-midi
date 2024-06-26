#include "midi_resource.h"

#include "midi_parser.h"

Error MidiResource::load_file(const String &p_path)
{
    UtilityFunctions::print(String("[GodotMidi] Reading midi file data: ") + p_path);

    // get midi file data
    godot::Ref<godot::FileAccess> midi_file = FileAccess::open(p_path, FileAccess::READ);
    if (midi_file == NULL)
    {
        UtilityFunctions::print(String("[GodotMidi] Error: Could not open file: ") + p_path);
        return FAILED;
    }

    PackedByteArray midi_data = midi_file->get_buffer(midi_file->get_length());
    // file will be auto-closed when midi_file goes out of scope

    // read header chunk
    MidiParser::RawMidiChunk header_chunk;
    midi_data = header_chunk.load_from_bytes(midi_data);

    // parse header chunk
    MidiParser::MidiHeaderChunk header;
    if (!header.parse_chunk(header_chunk, header))
    {
        UtilityFunctions::print("[GodotMidi] Error: Could not parse header chunk.");
        return FAILED;
    }

    header.only_notes = false;

    // load header into resource
    this->format = header.file_format;
    this->track_count = header.num_tracks;
    this->division = header.division;
    this->tempo = header.tempo;

    for (int trk_idx = 0; trk_idx < header.num_tracks; ++trk_idx)
    {
        // read track chunk
        MidiParser::RawMidiChunk trackChunk;
        midi_data = trackChunk.load_from_bytes(midi_data);

        // parse track chunk
        MidiParser::MidiTrackChunk track;
        if (!track.parse_chunk(trackChunk, header))
        {
            UtilityFunctions::print("[GodotMidi] Error: Could not parse track chunk: " + String::num_int64(trk_idx));
            return FAILED;
        }

        // add track
        Dictionary track_dict;
        track_dict["name"] = String("Track ") + String::num_int64(trk_idx);
        track_dict["events"] = Array();

        this->tracks.push_back(track_dict);

        // loop through events
        for (int i = 0; i < track.events.size(); i++)
        {
            // get event pointer
            std::unique_ptr<MidiParser::MidiEvent> p_event = std::move(track.events[i]);

            double delta = 0.0;

            // meta events
            if (p_event->get_type() == MidiParser::MidiEvent::EventType::Meta)
            {
                MidiParser::MidiEventMeta meta_event = *dynamic_cast<MidiParser::MidiEventMeta *>(p_event.get());
                delta = (double)meta_event.delta;

                // load meta event into current track
                Dictionary event_dict;
                event_dict["type"] = "meta";
                event_dict["track"] = trk_idx;
                // cast to int
                event_dict["subtype"] = static_cast<int64_t>(meta_event.event_type);
                event_dict["delta"] = delta;
                // since raw data is almost never useful for meta events, we store it as a variant
                // and put it in the data field
                event_dict["data"] = static_cast<Variant>(meta_event.meta_data);
                event_dict["channel"] = meta_event.channel;

                // add event to track
                Array event_array = this->tracks[trk_idx].get("events");
                event_array.push_back(event_dict);

                // if we have a track name event, update the track name
                if (meta_event.event_type == MidiParser::MidiEventMeta::MidiMetaEventType::SequenceOrTrackName)
                {
                    this->tracks[trk_idx].set("name", meta_event.meta_data);
                }
            }

            // note events
            if (p_event->get_type() == MidiParser::MidiEvent::EventType::Note)
            {
                MidiParser::MidiEventNote note_event = *dynamic_cast<MidiParser::MidiEventNote *>(p_event.get());
                delta = (double)note_event.delta;

                // load note event into current track
                Dictionary event_dict;
                event_dict["type"] = "note";
                event_dict["track"] = trk_idx;
                event_dict["subtype"] = note_event.event_type;
                event_dict["delta"] = delta;
                event_dict["note"] = note_event.note;
                event_dict["data"] = note_event.data;
                event_dict["channel"] = note_event.channel;

                // add event to track
                Array event_array = this->tracks[trk_idx].get("events");
                event_array.push_back(event_dict);
            }

            // system events
            if (p_event->get_type() == MidiParser::MidiEvent::EventType::System)
            {
                MidiParser::MidiEventSystem system_event = *dynamic_cast<MidiParser::MidiEventSystem *>(p_event.get());
                delta = (double)system_event.delta;

                // load system event into current track
                Dictionary event_dict;
                event_dict["type"] = "system";
                event_dict["track"] = trk_idx;
                event_dict["subtype"] = system_event.event_type;
                event_dict["delta"] = delta;
                event_dict["channel"] = system_event.channel;

                // add event to track
                Array event_array = this->tracks[trk_idx].get("events");
                event_array.push_back(event_dict);
            }
        }
    }

    return OK;
}

Error MidiResource::save_file(const String &p_path, const Ref<Resource> &p_resource)
{
    return OK;
}
