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

    // safely get audio output latency
    AudioServer *audio_server = AudioServer::get_singleton();
    if (audio_server != nullptr)
    {
        this->audio_output_latency = audio_server->get_output_latency();
    }
    else
    {
        this->audio_output_latency = 0.0;
        UtilityFunctions::printerr("[GodotMidi] AudioServer singleton is null, using default latency of 0.0");
    }
    this->note_offset = 0;
    this->note_cache = Array();

    this->playback_thread = std::thread();
    this->has_asp = false;

    this->longest_asp = nullptr;
    this->asp_midi = nullptr;
    this->asps = std::vector<AudioStreamPlayer *>();

    // disable process in the editor
    Engine *engine = Engine::get_singleton();
    if (engine != nullptr)
    {
        if (engine->is_editor_hint())
        {
            set_process_mode(ProcessMode::PROCESS_MODE_DISABLED);
        }
        else
        {
            set_process_mode(ProcessMode::PROCESS_MODE_ALWAYS);
        }
    }
    else
    {
        UtilityFunctions::printerr("[GodotMidi] Engine singleton is null, using default process mode");
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
            if (asp != nullptr)
            {
                asp->play();
            }
            else
            {
                UtilityFunctions::printerr("[GodotMidi] Null AudioStreamPlayer found in play(), skipping");
            }
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
    if (this->midi == nullptr)
    {
        UtilityFunctions::printerr("[GodotMidi] No midi resource set");
        return;
    }

    // reset time to zero
    this->current_time = 0;
    this->prev_track_times.clear();
    this->track_index_offsets.clear();

    // only resize if midi is valid
    if (this->midi != nullptr)
    {
        this->prev_track_times.resize(this->midi->get_track_count());
        this->track_index_offsets.resize(this->midi->get_track_count());
    }

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
            if (asp != nullptr)
            {
                asp->stop();
            }
            else
            {
                UtilityFunctions::printerr("[GodotMidi] Null AudioStreamPlayer found in stop_internal(), skipping");
            }
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
            if (asp != nullptr)
            {
                asp->set_stream_paused(true);
            }
            else
            {
                UtilityFunctions::printerr("[GodotMidi] Null AudioStreamPlayer found in pause(), skipping");
            }
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
            if (asp != nullptr)
            {
                asp->set_stream_paused(false);
            }
            else
            {
                UtilityFunctions::printerr("[GodotMidi] Null AudioStreamPlayer found in resume(), skipping");
            }
        }
    }
}

/// @brief Links the audio stream players to the midi player
/// @param asps
void MidiPlayer::link_audio_stream_player(Array asps)
{
    // extract audio stream players from the array
    double longest_time = -1000.0;
    for (int i = 0; i < asps.size(); i++)
    {
        Variant item = asps[i];

        // determine if we're dealing with an ASP or a ASP Midi
        AudioStreamPlayer *asp = nullptr;
        if (item.has_method("note_on"))
        {
            asp = Object::cast_to<AudioStreamPlayerMidi>(item);
            UtilityFunctions::print("[GodotMidi] Linked AudioStreamPlayerMidi");
        }
        else
        {
            asp = Object::cast_to<AudioStreamPlayer>(item);
            UtilityFunctions::print("[GodotMidi] Linked AudioStreamPlayer");
        }

        if (asp != nullptr && asp->get_stream() != nullptr)
        {
            Ref<AudioStream> asp_stream = asp->get_stream();
            if (asp_stream.is_null() || asp_stream->get_length() == 0 || asp_stream.is_valid() == false)
            {
                UtilityFunctions::printerr("[GodotMidi] Invalid AudioStream in link_audio_stream_player at index " + String::num_int64(i));
                continue;
            }

            // get the longest audio stream player
            double time = asp_stream->get_length();
            if (time > longest_time)
            {
                longest_time = time;
                this->longest_asp = asp;
            }

            this->asps.push_back(asp);
            this->has_asp = true;
        }
        else
        {
            UtilityFunctions::printerr("[GodotMidi] Invalid audio stream player found, skipping");
            continue;
        }
    }

    if (longest_asp == nullptr)
    {
        UtilityFunctions::printerr("[GodotMidi] No valid AudioStreamPlayers linked in link_audio_stream_player");
        this->has_asp = false;
        return;
    }

    longest_asp->connect("finished", Callable(this, "loop_or_stop_thread_safe"));
}

/// @brief Process function that is called every frame
/// @param delta
void MidiPlayer::_process(float delta)
{
    // check to make sure the scene tree isn't paused
    SceneTree *tree = this->get_tree();
    if (tree == nullptr)
    {
        UtilityFunctions::printerr("[GodotMidi] Scene tree is null in _process()");
        return;
    }

    if (tree->is_paused())
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
        Time *time_singleton = Time::get_singleton();
        if (time_singleton == nullptr)
        {
            UtilityFunctions::printerr("[GodotMidi] Time singleton is null, returning 0");
            return 0;
        }
        return time_singleton->get_ticks_usec();
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
        if (this->has_asp && this->longest_asp != nullptr)
        {
            AudioServer *audio_server = AudioServer::get_singleton();
            if (audio_server != nullptr)
            {
                double time = longest_asp->get_playback_position() + audio_server->get_time_since_last_mix();
                time -= audio_output_latency;
                delta = time - current_time;
            }
            else
            {
                UtilityFunctions::printerr("[GodotMidi] AudioServer singleton is null in threaded_playback()");
                delta = 0;
            }
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
    call_thread_safe("emit_signal", "finished");

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
        // bounds check for track index offsets and prev track times
        if (i >= this->track_index_offsets.size() || i >= this->prev_track_times.size())
        {
            UtilityFunctions::printerr("[GodotMidi] Track index out of bounds, skipping track " + String::num_int64(i));
            continue;
        }

        // get events for this track
        Array tracks = this->midi->get_tracks();
        if (i >= tracks.size())
        {
            UtilityFunctions::printerr("[GodotMidi] Track index exceeds tracks array size, skipping track " + String::num_int64(i));
            continue;
        }

        Dictionary track_dict = tracks[i];
        if (!track_dict.has("events"))
        {
            UtilityFunctions::printerr("[GodotMidi] Track " + String::num_int64(i) + " has no events, skipping");
            continue;
        }

        Array events = track_dict["events"];

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
            if (j >= events.size())
            {
                UtilityFunctions::printerr("[GodotMidi] Event index out of bounds, breaking from event loop");
                break;
            }

            Dictionary event = events[j];
            double event_delta = event.get("delta", 0);

            // apply tempo (or fixed SMPTE rate)
            double microseconds_per_tick = this->get_microseconds_per_tick();
            // delta time is stored as ticks, convert to microseconds
            event_delta = event_delta * microseconds_per_tick;

            // convert to seconds for ease of use
            double event_delta_seconds = event_delta / 1000000.0;
            // check for division by zero in speed scale
            if (speed_scale == 0)
            {
                UtilityFunctions::printerr("[GodotMidi] Speed scale is zero, using default scale of 1.0");
                event_delta_seconds /= 1.0;
            }
            else
            {
                event_delta_seconds /= speed_scale;
            }
            double event_absolute_time = event_delta_seconds + static_cast<double>(this->prev_track_times[i]);

            if (this->current_time + this->note_offset >= event_absolute_time)
            {
                // start at next available event (index offset + 1, since index offset is the last event we processed)
                this->track_index_offsets[i] = j + 1;
                String event_type = event.get("type", "undef");
                int event_subtype = event.get("subtype", 0);
                int event_channel = event.get("channel", 0);

                if (event_type == "meta")
                {
                    // ingest meta events such as tempo changes
                    // we need to do this now as opposed to when the midi file is loaded
                    // to allow for tempo changes during playback
                    if (event_subtype == MidiParser::MidiEventMeta::MidiMetaEventType::SetTempo)
                    {
                        this->midi->set_tempo(static_cast<int>(event.get("data", DEFAULT_MIDI_TEMPO)));
                    }

                    // TODO: support time signature changes
                    // these should be handled in the same way as tempo changes
                    // to allow for changes during playback (even though it isn't usually necessary)

                    call_thread_safe("emit_signal", "meta", event, i);
                }
                else if (event_type == "note")
                {
                    if (this->asp_midi != nullptr)
                    {
                        // // send events over to the audio stream player midi
                        // switch (event_subtype)
                        // {
                        // case MidiParser::MidiEventNote::NoteType::NoteOn:
                        //     // note on event
                        //     this->asp_midi->note_on(event.get("note", 0), event.get("data", 0), event_channel);
                        //     break;
                        // case MidiParser::MidiEventNote::NoteType::NoteOff:
                        //     // note off event
                        //     this->asp_midi->note_off(event.get("note", 0), event_channel);
                        //     break;
                        // case MidiParser::MidiEventNote::NoteType::ProgramChange:
                        //     // program change event
                        //     this->asp_midi->program_change(event_channel, event.get("data", 0));
                        //     break;
                        // case MidiParser::MidiEventNote::NoteType::PitchBend:
                        // {
                        //     // pitch bend event
                        //     // it's a 14-bit value, the first byte is the LSB and the second byte is the MSB
                        //     // the first byte is in "note" and the second byte is in "data"
                        //     int pitch_bend = (static_cast<int>(event.get("note", 0)) << 7) | static_cast<int>(event.get("data", 0));
                        //     this->asp_midi->pitch_bend(event_channel, pitch_bend);
                        //     break;
                        // }
                        // case MidiParser::MidiEventNote::NoteType::Controller:
                        //     // controller event
                        //     this->asp_midi->control_change(event_channel, event.get("note", 0), event.get("data", 0));
                        //     break;
                        // default:
                        //     break;
                        // }
                    }

                    // emit signal for note events
                    call_thread_safe("emit_signal", "note", event, i);
                }
                else if (event_type == "system")
                {
                    // emit signal for system events
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

/// @brief Rebuilds the note timing cache used by get_notes_in_range()/get_notes_around().
/// Merges every track's events into a single tick-ordered timeline (since
/// tempo changes apply globally, not per-track) and walks it once to compute
/// each note on/off event's absolute time in seconds. Safe to call from the
/// main thread whenever the midi resource changes; does not touch any state
/// used by the playback thread
void MidiPlayer::build_note_cache()
{
    this->note_cache.clear();

    if (this->midi == nullptr)
    {
        return;
    }

    // merge every track's events into a single array tagged with their
    // absolute tick position, so they can be sorted into one global timeline
    Array all_events;

    int64_t track_count = this->midi->get_track_count();
    for (int64_t t = 0; t < track_count; t++)
    {
        Array events = this->midi->get_tracks()[t].get("events");
        double tick_accum = 0.0;

        for (int64_t i = 0; i < events.size(); i++)
        {
            Dictionary event = events[i];
            double delta = event.get("delta", 0);
            tick_accum += delta;

            String type = event.get("type", "");
            int subtype = event.get("subtype", -1);
            bool is_tempo = type == "meta" && subtype == (int)MidiParser::MidiEventMeta::MidiMetaEventType::SetTempo;

            Dictionary timed_event = event.duplicate();
            timed_event["tick"] = tick_accum;
            timed_event["is_tempo"] = is_tempo;
            all_events.push_back(timed_event);
        }
    }

    // sort by absolute tick position; tempo-change events are ordered before
    // other events at the exact same tick so they take effect in time for
    // anything happening simultaneously (see _compare_tick_events)
    all_events.sort_custom(Callable(this, "_compare_tick_events"));

    double time_seconds = 0.0;
    double last_tick = 0.0;
    int32_t current_tempo = DEFAULT_MIDI_TEMPO;

    for (int64_t i = 0; i < all_events.size(); i++)
    {
        Dictionary event = all_events[i];
        double tick = event.get("tick", 0.0);
        bool is_tempo = event.get("is_tempo", false);

        double delta_ticks = tick - last_tick;
        double microseconds_per_tick = this->get_microseconds_per_tick(current_tempo);
        time_seconds += (delta_ticks * microseconds_per_tick) / 1000000.0;
        last_tick = tick;

        if (is_tempo)
        {
            current_tempo = (int32_t)event.get("data", DEFAULT_MIDI_TEMPO);
        }

        String type = event.get("type", "");
        if (type != "note")
        {
            continue;
        }

        int subtype = event.get("subtype", -1);
        if (subtype != (int)MidiParser::MidiEventNote::NoteType::NoteOn &&
            subtype != (int)MidiParser::MidiEventNote::NoteType::NoteOff)
        {
            // only note on/off events are cached; controller, pitch bend,
            // aftertouch, etc. aren't "notes" for rhythm-game purposes
            continue;
        }

        Dictionary note_event = event.duplicate();
        note_event.erase("tick");
        note_event.erase("is_tempo");

        // per MIDI convention, a NoteOn with velocity 0 is really a NoteOff;
        // "active" tells the caller whether this is an actual note trigger
        int velocity = note_event.get("data", 0);
        bool active = subtype == (int)MidiParser::MidiEventNote::NoteType::NoteOn && velocity > 0;
        note_event["active"] = active;
        note_event["time"] = time_seconds;

        this->note_cache.push_back(note_event);
    }
}

/// @brief Returns all cached note events whose absolute time (shifted by
/// note_offset) falls within [start_time, end_time]
/// @param start_time start of the query window, in seconds
/// @param end_time end of the query window, in seconds
Array MidiPlayer::get_notes_in_range(double start_time, double end_time)
{
    Array result;

    if (start_time > end_time)
    {
        double tmp = start_time;
        start_time = end_time;
        end_time = tmp;
    }

    // shift the query window by -note_offset instead of adjusting every
    // cached entry, since note_cache is stored in raw (unshifted) time
    double query_start = start_time - this->note_offset;
    double query_end = end_time - this->note_offset;

    int64_t count = this->note_cache.size();

    // binary search for the first entry whose time is >= query_start;
    // note_cache is kept sorted ascending by time by build_note_cache()
    int64_t low = 0;
    int64_t high = count;
    while (low < high)
    {
        int64_t mid = low + (high - low) / 2;
        Dictionary entry = this->note_cache[mid];
        double entry_time = entry.get("time", 0.0);
        if (entry_time < query_start)
        {
            low = mid + 1;
        }
        else
        {
            high = mid;
        }
    }

    for (int64_t i = low; i < count; i++)
    {
        Dictionary note_event = this->note_cache[i];
        double time = note_event.get("time", 0.0);

        if (time > query_end)
        {
            // sorted ascending, nothing further can match
            break;
        }

        Dictionary result_event = note_event.duplicate();
        result_event["time"] = time + this->note_offset;
        result.push_back(result_event);
    }

    return result;
}

/// @brief Convenience wrapper for querying a window around a specific
/// timestamp, e.g. the current playback time
/// @param time the center timestamp, in seconds
/// @param window_before how far before `time` to include, in seconds
/// @param window_after how far after `time` to include, in seconds
Array MidiPlayer::get_notes_around(double time, double window_before, double window_after)
{
    return this->get_notes_in_range(time - window_before, time + window_after);
}