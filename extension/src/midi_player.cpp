#include "midi_player.h"

#define DEFAULT_MIDI_TEMPO 500000

// global mutex
std::mutex mtx;

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
    if (this->audio_stream_player != nullptr)
    {
        this->audio_stream_player->play();
    }
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
    this->state.store(PlayerState::Stopped);
    UtilityFunctions::print("[GodotMidi] Stopped");

    // clean up thread
    if (this->playback_thread.joinable())
    {
        this->playback_thread.join();
    }

    // if the audio stream player is set, stop playing the audio
    if (this->audio_stream_player != nullptr)
    {
        this->audio_stream_player->stop();
    }
}

/// @brief Called when the midi player is set to loop
/// and the midi has finished playing
void MidiPlayer::loop_internal()
{
    this->stop();
    this->play();
}

/// @brief Pause the midi playback
void MidiPlayer::pause()
{
    this->state.store(PlayerState::Paused);
    UtilityFunctions::print("[GodotMidi] Paused");

    // if the audio stream player is set, pause the audio
    if (this->audio_stream_player != nullptr)
    {
        this->audio_stream_player->set_stream_paused(true);
    }
}

/// @brief Resume the midi playback
void MidiPlayer::resume()
{
    this->state.store(PlayerState::Playing);
    UtilityFunctions::print("[GodotMidi] Resumed");

    // if the audio stream player is set, resume the audio
    if (this->audio_stream_player != nullptr)
    {
        this->audio_stream_player->set_stream_paused(false);
    }
}

/// @brief Links the audio stream player to the midi player
/// @param audio_stream_player
void MidiPlayer::link_audio_stream_player(AudioStreamPlayer *audio_stream_player)
{
    this->audio_stream_player = audio_stream_player;
}

/// @brief Function that is run in a separate thread to play back the midi
/// @param midi_player
void MidiPlayer::threaded_playback()
{
    // Lambda function to get the current time in microseconds
    const auto get_now = []() -> long long {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    };

    // print
    UtilityFunctions::print("[GodotMidi] Playback thread started");
    const long long start_time = get_now();
    while (state.load() == PlayerState::Playing)
    {
        // get the delta from the audio stream player if it's set
        double delta = 0;
        if (audio_stream_player != nullptr)
        {
            double time = audio_stream_player->get_playback_position() + AudioServer::get_singleton()->get_time_since_last_mix();
            time -= audio_output_latency;
            delta = time - current_time;
        }
        else
        {
            // if the audio stream player is not set, compute the delta manually
            long long time_now = get_now();
            delta = (static_cast<double>(time_now - start_time) / 1000000.0) - current_time;
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
    }
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

    // increment time, current time will hold the
    // number of seconds since starting
    this->current_time += delta * speed_scale;
}