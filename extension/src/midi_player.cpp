#include "midi_player.h"

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

    this->audio_output_latency = AudioServer::get_singleton()->get_output_latency();

    this->playback_thread = std::thread();
    this->has_asp = false;

    this->longest_asp = nullptr;
    this->asps = std::vector<AudioStreamPlayer *>();

    // disable process in the editor
    if (Engine::get_singleton()->is_editor_hint())
    {
        set_process_mode(ProcessMode::PROCESS_MODE_DISABLED);
    }
    else
    {
        set_process_mode(ProcessMode::PROCESS_MODE_ALWAYS);
    }
}

MidiPlayer::~MidiPlayer()
{
    this->state.store(PlayerState::Stopped);
    // clean up thread
    if (this->playback_thread.joinable())
    {
        this->playback_thread.join();
    }
}

/// @brief Set the midi resource to play and start the playback thread
void MidiPlayer::play()
{
    if (this->midi == nullptr)
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

    this->state.store(PlayerState::Playing);
    UtilityFunctions::print("[GodotMidi] Playing");

    // start the playback thread without the audio stream player
    this->playback_thread = std::thread(&MidiPlayer::threaded_playback, this);

    // if the audio stream player is set, start playing the audio
    if (this->has_asp)
    {
        for (auto asp : this->asps)
        {
            asp->play();
        }
    }
}

/// @brief Stop the midi playback and reset the clock
void MidiPlayer::stop()
{
    stop_internal(this->auto_stop);
}

/// @brief Internal function for stopping the midi playback
void MidiPlayer::stop_internal(bool stop_asp = true)
{
    // reset time to zero
    this->current_time = 0;
    this->prev_track_times.clear();
    this->prev_track_times.resize(this->midi->get_track_count());
    this->track_index_offsets.clear();
    this->track_index_offsets.resize(this->midi->get_track_count());
    this->state.store(PlayerState::Stopped);
    UtilityFunctions::print("[GodotMidi] Stopped");

    // clean up thread
    if (this->playback_thread.joinable())
    {
        this->playback_thread.join();
    }

    // if the audio stream player is set, stop playing the audio
    if (this->has_asp && this->auto_stop)
    {
        for (auto asp : this->asps)
        {
            asp->stop();
        }
    }
}

/// @brief Called when the midi player is set to loop
/// and the midi has finished playing
void MidiPlayer::loop_internal()
{
    // always auto stop when looping
    this->stop_internal(true);
    this->play();
}

/// @brief Pause the midi playback
void MidiPlayer::pause()
{
    this->state.store(PlayerState::Paused);
    UtilityFunctions::print("[GodotMidi] Paused");

    // if the audio stream player is set, pause the audio
    if (this->has_asp)
    {
        for (auto asp : this->asps)
        {
            asp->set_stream_paused(true);
        }
    }
}

/// @brief Resume the midi playback
void MidiPlayer::resume()
{
    this->state.store(PlayerState::Playing);
    UtilityFunctions::print("[GodotMidi] Resumed");

    // if the audio stream player is set, resume the audio
    if (this->has_asp)
    {
        for (auto asp : this->asps)
        {
            asp->set_stream_paused(false);
        }
    }
}

/// @brief Links the audio stream players to the midi player
/// @param asps
void MidiPlayer::link_audio_stream_player(Array asps)
{
    // extract audio stream players from the array
    this->asps.resize(asps.size());
    double longest_time = 0;
    for (int i = 0; i < asps.size(); i++)
    {
        AudioStreamPlayer *asp = Object::cast_to<AudioStreamPlayer>(asps[i]);
        if (asp != nullptr)
        {
            // get the longest audio stream player
            double time = asp->get_stream()->get_length();
            if (time > longest_time)
            {
                longest_time = time;
                this->longest_asp = asp;
            }

            this->asps[i] = asp;
            this->has_asp = true;
        }
    }

    longest_asp->connect("finished", Callable(this, "loop_or_stop_thread_safe"));
}

/// @brief Process function that is called every frame
/// @param delta
void MidiPlayer::_process(float delta)
{
    // check to make sure the scene tree isn't paused
    if (this->get_tree()->is_paused())
    {
        if (this->state.load() == PlayerState::Playing)
        {
            // if it is, pause the player
            this->pause();
        }
    }
    else
    {
        // if the scene tree isn't paused, check if the player is paused
        if (this->state.load() == PlayerState::Paused)
        {
            // and unpause if it is
            this->resume();
        }
    }
}

/// @brief Function that is run in a separate thread to play back the midi
/// @param midi_player
void MidiPlayer::threaded_playback()
{
    // Lambda function to get the current time in microseconds
    const auto get_now = []() -> long long
    {
        return Time::get_singleton()->get_ticks_usec();
    };

    // print
    UtilityFunctions::print("[GodotMidi] Playback thread started");
    double delta = 0;
    while (state.load() == PlayerState::Playing || state.load() == PlayerState::Paused)
    {
        const long long start_time = get_now();

        if (state.load() == PlayerState::Paused)
        {
            // sleep for 1ms if we're paused
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // get the delta from the audio stream player if it's set
        if (this->has_asp)
        {
            double time = longest_asp->get_playback_position() + AudioServer::get_singleton()->get_time_since_last_mix();
            time -= audio_output_latency;
            delta = time - current_time;
        }

        // delta should never be negative
        delta = delta > 0 ? delta : 0;

        // process the midi player
        process_delta(delta);

        if (delta < 0.001)
        {
            // sleep for 1ms if we're going too fast
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (!this->has_asp)
        {
            // if the audio stream player is not set, compute the delta manually
            long long time_now = get_now();
            delta = static_cast<double>(time_now - start_time) / 1000000.0;
        }
    }
}

/// @brief Loop the midi player or stop it if looping is disabled
void MidiPlayer::loop_or_stop_thread_safe()
{
    this->emit_signal("finished");

    if (this->loop == false)
    {
        this->call_thread_safe("stop");
        UtilityFunctions::print("[GodotMidi] Finished, stopping");
        return;
    }
    // set state to stopped, this prevents issues while waiting for
    // the below function to sync with the main thread
    this->state.store(PlayerState::Stopped);
    this->call_thread_safe("loop_internal");
    UtilityFunctions::print("[GodotMidi] Finished, looping");
}

/// @brief Process one delta of time for the midi player
/// @param delta the time in seconds to process
void MidiPlayer::process_delta(double delta)
{
    if (this->midi == nullptr)
    {
        UtilityFunctions::printerr("[GodotMidi] No midi resource set");
        // stop the player if there's no midi resource
        this->state.store(PlayerState::Stopped);
        this->call_thread_safe("stop");
        return;
    }

    // process each track
    bool has_more_events = false;
    for (uint64_t i = 0; i < this->midi->get_track_count(); i++)
    {
        // get events for this track
        Array events = this->midi->get_tracks()[i].get("events");

        // starting at index offset, check if there's an event at the current time
        int index_off = this->track_index_offsets[i];

        // if we have more events, don't stop yet
        if (events.size() - 1 > index_off)
        {
            has_more_events = true;
        }

        // search forward in time
        for (uint64_t j = index_off; j < events.size(); j++)
        {
            Dictionary event = events[j];
            double event_delta = event.get("delta", 0);

            // apply tempo
            double microseconds_per_tick = static_cast<double>(this->midi->get_tempo()) / static_cast<double>(this->midi->get_division());
            // delta time is stored as ticks, convert to microseconds
            event_delta = event_delta * microseconds_per_tick;

            // convert to seconds for ease of use
            double event_delta_seconds = event_delta / 1000000.0;
            event_delta_seconds /= speed_scale;
            double event_absolute_time = event_delta_seconds + static_cast<double>(this->prev_track_times[i]);

            if (this->current_time >= event_absolute_time)
            {
                // start at next available event (index offset + 1, since index offset is the last event we processed)
                this->track_index_offsets[i] = j + 1;
                String event_type = event.get("type", "undef");

                if (event_type == "meta")
                {
                    // print note index offset, time, j and absolute time, track, subtype and delta
                    // UtilityFunctions::print("Note index offset: " + String::num_int64(index_off) + " j: " + String::num_int64(j) + " Time: " + String::num(this->current_time) + " Absolute time: " + String::num(event_absolute_time) + " Track: " + String::num_int64(i) + " Subtype: " + String::num(event.get("subtype", 0)) + " Delta: " + String::num(event_delta_seconds));

                    // ingest meta events such as tempo changes
                    // we need to do this now as opposed to when the midi file is loaded
                    // to allow for tempo changes during playback
                    int meta_type = event.get("subtype", 0);

                    if (meta_type == MidiParser::MidiEventMeta::MidiMetaEventType::SetTempo)
                    {
                        this->midi->set_tempo(static_cast<int>(event.get("data", DEFAULT_MIDI_TEMPO)));

                        // print tempo
                        // UtilityFunctions::print("[GodotMidi] Tempo: " + String::num(this->midi->get_tempo()));
                    }

                    // TODO: support time signature changes
                    // these should be handled in the same way as tempo changes
                    // to allow for changes during playback (even though it isn't usually necessary)

                    call_thread_safe("emit_signal", "meta", event, i);
                }
                else if (event_type == "note")
                {
                    call_thread_safe("emit_signal", "note", event, i);
                    // print note index offset, time, j, absolute time, track, subtype and delta
                    // UtilityFunctions::print("Note index offset: " + String::num_int64(index_off) + " j: " + String::num_int64(j) + " Time: " + String::num(this->current_time) + " Absolute time: " + String::num(event_absolute_time) + " Track: " + String::num_int64(i) + " Subtype: " + String::num(event.get("subtype", 0)) + " Delta: " + String::num(event_delta_seconds));
                }
                else if (event_type == "system")
                {
                    call_thread_safe("emit_signal", "system", event, i);
                }
                else
                {
                    UtilityFunctions::printerr("[GodotMidi] Invalid event type");
                }

                // store time to look for events after this one
                this->prev_track_times[i] = event_absolute_time;
            }
            else
            {
                // print
                // UtilityFunctions::print("[GodotMidi] No more events on this track at this time");
                // we've gone too far, break and move to the next track
                break;
            }
        }
    }

    if (has_more_events == false)
    {
        loop_or_stop_thread_safe();
    }

    // increment time, current time will hold the
    // number of seconds since starting
    this->current_time += delta;
}