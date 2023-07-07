#include "midi.h"
#include "midi_parser.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <algorithm>

using namespace godot;

Midi::Midi()
{
}

Midi::~Midi()
{
}

/// @brief converts a midi file to an animation resource
/// @param file_path the path to the midi file
/// @return an animation resource
void Midi::load_from_file(String source_path, String save_path, bool only_notes)
{
    UtilityFunctions::print(String("loading midi file: ") + source_path);

    // get midi file data
    godot::Ref<godot::FileAccess> midi_file = FileAccess::open(source_path, FileAccess::READ);
    PackedByteArray midi_data = midi_file->get_buffer(midi_file->get_length());
    // file will be auto-closed when midi_file goes out of scope

    // create new animation resource on heap
    Ref<Animation> p_anim = memnew(Animation);

    UtilityFunctions::print(String("file size: ") + String::num_int64(midi_data.size()));

    // read header chunk
    MidiParser::RawMidiChunk header_chunk;
    midi_data = header_chunk.load_from_bytes(midi_data);

    UtilityFunctions::print(String("header chunk size: ") + String::num_int64(midi_data.size()));
    // print header type
    UtilityFunctions::print(String("header chunk type: ") + String::num_int64(header_chunk.chunk_type));
    // print header id
    UtilityFunctions::print(String("header chunk id: ") + header_chunk.chunk_id);

    // parse header chunk
    MidiParser::MidiHeaderChunk header;
    header.parse_chunk(header_chunk, header);
    header.only_notes = only_notes;

    // print header info
    UtilityFunctions::print(String("midi file format: ") + String::num_int64(header.file_format));
    UtilityFunctions::print(String("midi division type: ") + String::num_int64(header.division_type));
    UtilityFunctions::print(String("midi division: ") + String::num_int64(header.division));
    UtilityFunctions::print(String("midi tempo: ") + String::num_int64(header.tempo));
    UtilityFunctions::print(String("midi num tracks: ") + String::num_int64(header.num_tracks));

    // create animation track for each midi track
    // plus 3 for the 2 more meta tracks
    for (int i = 0; i < (header.num_tracks) + 3; i++)
    {
        // create animation track
        p_anim->add_track(Animation::TrackType::TYPE_METHOD);
        p_anim->track_set_path(i, NodePath("../MidiManager"));
    }

    Array layers;
    Array sections;
    int layers_idx = 0;
    int sections_idx = 0;

    std::vector<double> track_times(header.num_tracks, 0);
    int trk_idx = 0;
    for (trk_idx = 0; trk_idx < header.num_tracks; trk_idx += 1)
    {
        double track_time = 0;
        // read track chunk
        MidiParser::RawMidiChunk trackChunk;
        midi_data = trackChunk.load_from_bytes(midi_data);

        // parse track chunk
        MidiParser::MidiTrackChunk track;
        track.parse_chunk(trackChunk, header);

        // loop through note events
        double time = 0;
        for (int i = 0; i < track.events.size(); i++)
        {
            // get event pointer
            std::unique_ptr<MidiParser::MidiEvent> p_event = std::move(track.events[i]);

            double delta_time = 0.0f;
            double tick_duration = (double)header.tempo / (double)header.division;

            if (p_event->get_type() == MidiParser::MidiEvent::EventType::Meta)
            {

                MidiParser::MidiEventMeta meta_event = *dynamic_cast<MidiParser::MidiEventMeta *>(p_event.get());
                delta_time = (double)meta_event.delta_time;

                UtilityFunctions::print(String("Parsing meta event: " + String::num_int64(meta_event.event_type)));

                // insert as key in animation track
                // key will contain type and data
                Dictionary evt_dict;
                evt_dict["method"] = "meta_event_input";
                Array args;
                args.append(meta_event.event_type);
                args.append(meta_event.data);

                if (meta_event.event_type == MidiParser::MidiEventMeta::MidiMetaEventType::SequenceOrTrackName)
                {
                    if (meta_event.text == "LayerChange")
                        layers_idx = trk_idx;
                    else if (meta_event.text == "SectionChange")
                        sections_idx = trk_idx;
                }

                if (meta_event.text != "")
                {
                    args.append(meta_event.text);
                }
                else
                {
                    args.append(String("GM_Null"));
                }
                args.append(trk_idx);
                evt_dict["args"] = args;

                // if it's a tempo change event, update the tempo
                if (meta_event.event_type == MidiParser::MidiEventMeta::MidiMetaEventType::SetTempo)
                {
                    header.tempo = Utility::decode_int24_be(meta_event.data, 0);
                    tick_duration = (double)header.tempo / (double)header.division;
                }

                double delta_microseconds = (double)delta_time * tick_duration;
                double delta_seconds = delta_microseconds / 1000000.0;
                time += delta_seconds;

                p_anim->track_insert_key(trk_idx, time, evt_dict);
            }

            if (p_event->get_type() == MidiParser::MidiEvent::EventType::Note)
            {
                MidiParser::MidiEventNote note_event = *dynamic_cast<MidiParser::MidiEventNote *>(p_event.get());
                delta_time = (double)note_event.delta_time;
                double delta_microseconds = (double)delta_time * tick_duration;
                double delta_seconds = delta_microseconds / 1000000.0;
                time += delta_seconds;

                if (trk_idx == layers_idx) {
                    
                    if (note_event.event_type == MidiParser::MidiEventNote::NoteType::NoteOn) {
                        layers.append(time);
                    }
                }

                if (trk_idx == sections_idx) {
                    if (note_event.event_type == MidiParser::MidiEventNote::NoteType::NoteOn) {
                        sections.append(time);
                    }
                }

                // if we're only playing notes, skip other events
                if (header.only_notes && note_event.event_type == MidiParser::MidiEventNote::NoteType::Controller
                || header.only_notes && note_event.event_type == MidiParser::MidiEventNote::NoteType::ProgramChange
                || header.only_notes && note_event.event_type == MidiParser::MidiEventNote::NoteType::PitchBend
                || header.only_notes && note_event.event_type == MidiParser::MidiEventNote::NoteType::ChannelPressure)
                {
                    continue;
                }

                // insert event as key in animation track
                Dictionary evt_dict;
                evt_dict["method"] = "note_event_input";
                Array args;
                args.append(note_event.note);
                args.append(note_event.data);
                args.append(note_event.event_type);
                args.append(trk_idx);
                evt_dict["args"] = args;

                p_anim->track_insert_key(trk_idx, time, evt_dict);
            }

            if (p_event->get_type() == MidiParser::MidiEvent::EventType::System)
            {
                MidiParser::MidiEventSystem system_event = *dynamic_cast<MidiParser::MidiEventSystem *>(p_event.get());
                delta_time = (double)system_event.delta_time;
                double delta_microseconds = (double)delta_time * tick_duration;
                double delta_seconds = delta_microseconds / 1000000.0;
                time += delta_seconds;
                UtilityFunctions::print(String("Parsing system event: " + String::num_int64(system_event.event_type)));

                // insert event as key in animation track
                // note dict will store note, data, note type and track
                Dictionary evt_dict;
                evt_dict["method"] = "system_event_input";
                Array args;
                args.append(system_event.event_type);
                args.append(trk_idx);
                evt_dict["args"] = args;

                p_anim->track_insert_key(trk_idx, time, evt_dict);
            }

            if (trk_idx == layers_idx) {
                UtilityFunctions::print("delta: " + String::num_real(delta_time));
                // print type
                UtilityFunctions::print("type: " + String::num_int64(p_event->get_type()));
            }
        }
        track_time += time;
        track_times[trk_idx] = track_time;
    }

    // remove all empty tracks
    for (int i = 0; i < p_anim->get_track_count(); i++)
    {
        if (p_anim->track_get_key_count(i) == 0)
        {
            p_anim->remove_track(i);
            i--;
            trk_idx -= 1;
        }
    }

    p_anim->set_length(*std::max_element(std::begin(track_times), std::end(track_times)));

    // marker_input(type, times)
    Dictionary evt_dict;
    evt_dict["method"] = "marker_input";
    Array args;
    args.append("LayerChange");
    args.append(layers);
    evt_dict["args"] = args;
    p_anim->track_insert_key(trk_idx + 1, 0, evt_dict);

    // marker_input(type, times)
    evt_dict["method"] = "marker_input";
    args.clear();
    args.append("SectionChange");
    args.append(sections);
    evt_dict["args"] = args;
    p_anim->track_insert_key(trk_idx + 2, 0, evt_dict);

    // save animation resource
    ResourceSaver saver;
    saver.save(p_anim, save_path);
}

/// @brief override method for registering c++ functions in godot
void Midi::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("load_from_file", "source_path", "save_path", "only_notes"), &Midi::load_from_file, DEFVAL(""), DEFVAL(""), DEFVAL(false));
}
