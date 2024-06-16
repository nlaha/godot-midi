#include "midi_player.h"

#define CUTE_SOUND_IMPLEMENTATION
#include "cute_sound.h"

#define DEFAULT_MIDI_TEMPO 500000

MidiPlayer::MidiPlayer()
{
    // initialize variables
    this->current_time = 0;
    this->prev_track_times = Array();
    this->track_index_offsets = Array();

    this->speed_scale = 1;
    this->loop = false;
    this->state = PlayerState::Stopped;
    this->manual_process = false;

    //set_process_thread_group(ProcessThreadGroup::PROCESS_THREAD_GROUP_MAIN_THREAD);

    HWND hwnd = GetConsoleWindow();
    cs_init(hwnd, 44100, 1024, NULL);
}

MidiPlayer::~MidiPlayer()
{
    // free audio sources
    for (auto it = audio_sources.begin(); it != audio_sources.end(); ++it)
    {
        cs_free_audio_source(it->second);
    }
    audio_sources.clear();
    cs_shutdown();
}

/// @brief Set the midi resource to play and start the playback thread
void MidiPlayer::play()
{
    if (this->midi == NULL)
    {
        UtilityFunctions::printerr("[GodotMidi] No midi resource set");
        return;
    }

    // resize track index offsets and previous track times
    this->track_index_offsets.resize(this->midi->get_track_count());
    this->prev_track_times.resize(this->midi->get_track_count());
    // set initial prev_track_times to zero
    for (uint64_t i = 0; i < this->midi->get_track_count(); i++)
    {
        this->prev_track_times[i] = 0;
    }

    this->state = PlayerState::Playing;
    UtilityFunctions::print("[GodotMidi] Playing");
}

/// @brief Stop the midi playback and reset the clock
void MidiPlayer::stop()
{
    // reset time to zero
    this->current_time = 0;
    this->prev_track_times.clear();
    this->prev_track_times.resize(this->midi->get_track_count());
    this->track_index_offsets.clear();
    this->track_index_offsets.resize(this->midi->get_track_count());
    this->state = PlayerState::Stopped;
    UtilityFunctions::print("[GodotMidi] Stopped");
}

/// @brief Pause the midi playback
void MidiPlayer::pause()
{
    this->state = PlayerState::Paused;
    UtilityFunctions::print("[GodotMidi] Paused");
}

/// @brief Internal playback function
void MidiPlayer::_process(double delta)
{
    if (this->manual_process == false)
    {
        process_delta(delta);
    }
}

void MidiPlayer::process_delta(double delta)
{
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    if (this->midi == NULL)
    {
        UtilityFunctions::printerr("[GodotMidi] No midi resource set");
        return;
    }

    if (!Engine::get_singleton()->is_editor_hint())
    {
        if (this->state == PlayerState::Playing)
        {
            // process each track
            bool has_more_events = false;
            for (uint64_t track_idx = 0; track_idx < this->midi->get_track_count(); track_idx++)
            {
                // get events for this track
                Array events = this->midi->get_tracks()[track_idx].get("events");

                // starting at index offset, check if there's an event at the current time
                int index_off = this->track_index_offsets[track_idx];
                index_off++;

                // if we have more events, don't stop yet
                if (events.size() - 1 > index_off)
                {
                    has_more_events = true;
                }

                // search forward in time
                for (uint64_t event_offset = index_off; event_offset < events.size(); event_offset++)
                {
                    Dictionary event = events[event_offset];
                    double event_delta = event.get("delta", 0);

                    // apply tempo
                    double microseconds_per_tick = static_cast<double>(this->midi->get_tempo()) / static_cast<double>(this->midi->get_division());
                    // delta time is stored as ticks, convert to microseconds
                    event_delta = event_delta * microseconds_per_tick;

                    // convert to seconds for ease of use
                    double event_delta_seconds = event_delta / 1000000.0;
                    double event_absolute_time = event_delta_seconds + static_cast<double>(this->prev_track_times[track_idx]);

                    if (this->current_time >= event_absolute_time)
                    {
                        this->track_index_offsets[track_idx] = event_offset;
                        String event_type = event.get("type", "undef");

                        if (event_type == "meta")
                        {
                            // ingest meta events such as tempo changes
                            // we need to do this now as opposed to when the midi file is loaded
                            // to allow for tempo changes during playback
                            int meta_type = event.get("subtype", 0);

                            if (meta_type == MidiParser::MidiEventMeta::MidiMetaEventType::SetTempo)
                            {
                                this->midi->set_tempo(static_cast<int>(event.get("data", DEFAULT_MIDI_TEMPO)));
                            }

                            // TODO: support time signature changes
                            // these should be handled in the same way as tempo changes
                            // to allow for changes during playback (even though it isn't usually necessary)

                            emit_signal("meta", event, track_idx);
                        }
                        else if (event_type == "note")
                        {
                            emit_signal("note", event, track_idx);
                            
                            // check if this is a note on event
                            if (static_cast<int>(event.get("subtype", 0)) == MIDI_MESSAGE_NOTE_ON)
                            {
                                // if a sound effect exists for this track, play it
                                if (this->get_track_sound_effects().has(track_idx))
                                {
                                    Ref<AudioStreamWAV> sound_effect = this->get_track_sound_effects()[track_idx];
                                    if (sound_effect.is_valid())
                                    {
                                        // if the sound effect hasn't been loaded yet, load it
                                        if (audio_sources.find(track_idx) == audio_sources.end())
                                        {
                                            cs_audio_source_t *source = cs_read_mem_wav(sound_effect->get_data().ptr(), sound_effect->get_data().size(), 0);
                                            audio_sources[track_idx] = source;
                                        }

                                        cs_sound_params_t params = cs_sound_params_default();
                                        cs_play_sound(audio_sources[track_idx], params);
                                    }
                                }
                            }
                        }
                        else if (event_type == "system")
                        {
                            emit_signal("system", event, track_idx);
                        }
                        else
                        {
                            UtilityFunctions::printerr("[GodotMidi] Invalid event type");
                        }

                        // store time to look for events after this one
                        this->prev_track_times[track_idx] = event_absolute_time;
                    }
                    else
                    {
                        // print
                        // UtilityFunctions::print("[GodotMidi] No more events on this track at this time");
                        // we've gone too far, break
                        break;
                    }
                }
            }

            if (has_more_events == false)
            {
                if (this->loop == false)
                {
                    this->stop();
                    UtilityFunctions::print("[GodotMidi] Finished, stopping");
                    return;
                }
                this->stop();
                this->play();
                UtilityFunctions::print("[GodotMidi] Finished, looping");
            }

            // increment time, current time will hold the
            // number of seconds since starting
            this->current_time += delta * speed_scale;
        }
    }

    cs_update(delta);
}