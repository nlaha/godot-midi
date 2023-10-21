#include "midi_player.h"

MidiPlayer::MidiPlayer()
{
    // initialize variables
    this->current_time = 0;

    set_process_thread_group(ProcessThreadGroup::PROCESS_THREAD_GROUP_SUB_THREAD);
}

MidiPlayer::~MidiPlayer()
{
}

/// @brief Set the midi resource to play and start the playback thread
void MidiPlayer::play()
{
    this->state = PlayerState::Playing;
    UtilityFunctions::print("[GodotMidi] Playing");
}

/// @brief Stop the midi playback and reset the clock
void MidiPlayer::stop()
{
    // reset time to zero
    this->current_time = 0;
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
    if (!Engine::get_singleton()->is_editor_hint())
    {
        if (this->state == PlayerState::Playing)
        {
            // process each track
            bool has_more_events = false;
            for (size_t i = 0; i < this->midi->get_track_count(); i++)
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
                for (size_t j = index_off; j < events.size(); j++)
                {
                    Dictionary event = events[j];
                    double event_time = event.get("time", 0);

                    if (this->current_time >= event_time)
                    {
                        this->track_index_offsets[i] = j;
                        String event_type = event.get("type", "undef");

                        if (event_type == "meta")
                        {
                            emit_signal("meta", event, i);
                        }
                        else if (event_type == "note")
                        {
                            emit_signal("note", event, i);
                        }
                        else if (event_type == "system")
                        {
                            emit_signal("system", event, i);
                        }
                        else
                        {
                            UtilityFunctions::printerr("[GodotMidi] Invalid event type");
                        }
                    }
                    else
                    {
                        // we've gone too far, break
                        break;
                    }
                }
            }

            if (has_more_events == false)
            {
                // loop by default
                // TODO: add loop property
                this->current_time = 0;
                this->track_index_offsets.clear();
                this->track_index_offsets.resize(this->midi->get_track_count());
                UtilityFunctions::print("[GodotMidi] Finished, looping");
            }

            // increment time, current time will hold the
            // number of seconds since starting
            this->current_time += delta;
        }
    }
}