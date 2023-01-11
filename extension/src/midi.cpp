#include "midi.h"
#include "midi_parser.h"
#include <algorithm>

using namespace godot;

MIDI::MIDI()
{
}

MIDI::~MIDI()
{
}

/// @brief converts a midi file to an animation resource
/// @param file_path the path to the midi file
/// @return an animation resource
Animation MIDI::load_from_file(String file_path)
{
    // get midi file data
    godot::Ref<godot::FileAccess> midi_file = FileAccess::open(file_path, FileAccess::READ);
    PackedByteArray midi_data = midi_file->get_buffer(midi_file->get_length());
    // file will be auto-closed when midi_file goes out of scope

    Animation anim;

    // read header chunk
    MIDIParser::RawMidiChunk header_chunk;
    midi_data = header_chunk.load_from_bytes(midi_data);

    // parse header chunk
    MIDIParser::MidiHeaderChunk header;
    header.parse_chunk(header_chunk, header);

    // create animation track for each midi track
    for (int i = 0; i < header.num_tracks; i++)
    {
        // create animation track
        anim.add_track(Animation::TrackType::TYPE_METHOD);
        anim.track_set_path(i, NodePath("../MidiManager"));
    }

    std::vector<double> track_times(header.num_tracks, 0);
    for (int trk_idx = 0; trk_idx < header.num_tracks; trk_idx++)
    {
        double track_time = 0;
        // read track chunk
        MIDIParser::RawMidiChunk trackChunk;
        midi_data = trackChunk.load_from_bytes(midi_data);

        // parse track chunk
        MIDIParser::MidiTrackChunk track;
        track.parse_chunk(trackChunk, header);

        // loop through note events
        float time = 0;
        for (int i = 0; i < track.events.size(); i++)
        {
            // get event
            std::unique_ptr<MIDIParser::MidiEvent> p_event = std::move(track.events[i]);

            double delta_time = 0.0f;
            double tick_duration = (double)header.tempo / (double)header.division;

            if (p_event->get_type() == MIDIParser::MidiEvent::EventType::Meta)
            {
                MIDIParser::MidiEventMeta meta_event = *dynamic_cast<MIDIParser::MidiEventMeta *>(p_event.get());

                // insert as key in animation track
                // key will contain type and data
                Dictionary evt_dict;
                evt_dict["method"] = "MetaEventInput";
                Variant args[3] = {meta_event.event_type, meta_event.data, trk_idx};
                evt_dict["args"] = args;

                // if it's a tempo change event, update the tempo
                if (meta_event.event_type == MIDIParser::MidiEventMeta::MidiMetaEventType::SetTempo)
                {
                    header.tempo = Utility::decode_int24_be(meta_event.data, 0);
                    tick_duration = (double)header.tempo / (double)header.division;
                }

                anim.track_insert_key(trk_idx, time, evt_dict);
            }

            if (p_event->get_type() == MIDIParser::MidiEvent::EventType::Note)
            {
                MIDIParser::MidiEventNote note_event = *dynamic_cast<MIDIParser::MidiEventNote *>(p_event.get());

                // insert event as key in animation track
                Dictionary evt_dict;
                evt_dict["method"] = "NoteEventInput";
                Variant args[4] = {note_event.note, note_event.data, note_event.event_type, trk_idx};
                evt_dict["args"] = args;

                anim.track_insert_key(trk_idx, time, evt_dict);

                delta_time = (double)note_event.delta_time;
            }

            if (p_event->get_type() == MIDIParser::MidiEvent::EventType::System)
            {
                MIDIParser::MidiEventSystem system_event = *dynamic_cast<MIDIParser::MidiEventSystem *>(p_event.get());
                // insert event as key in animation track
                // note dict will store note, data, note type and track
                Dictionary evt_dict;
                evt_dict["method"] = "SystemEventInput";
                Variant args[2] = {system_event.event_type, trk_idx};
                evt_dict["args"] = args;

                anim.track_insert_key(trk_idx, time, evt_dict);

                delta_time = (double)system_event.delta_time;
            }

            double delta_microseconds = (double)delta_time * tick_duration;
            double delta_seconds = delta_microseconds / 1000000.0;
            time += (float)delta_seconds;
        }
        track_time += time;
        track_times[trk_idx] = track_time;
    }

    anim.set_length(*std::max_element(std::begin(track_times), std::end(track_times)));

    return anim;
}

/// @brief override method for registering c++ functions in godot
void MIDI::_bind_methods()
{
}
