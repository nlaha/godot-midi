#include "midi_player.h"

MidiPlayer::MidiPlayer()
{
    // initialize variables
    this->current_time = 0;

    // initialize track_index_offsets
    this->track_index_offsets.resize(this->midi->get_track_count());
    for (size_t i = 0; i < this->track_index_offsets.size(); i++)
    {
        this->track_index_offsets[i] = 0;
    }

    this->track_index_offsets.clear();
}

MidiPlayer::~MidiPlayer()
{
}

/// @brief Set the midi resource to play and start the playback thread
void MidiPlayer::play()
{
    this->state = Playing;
}

/// @brief Stop the midi playback and reset the clock
void MidiPlayer::stop()
{
    // reset time to zero
    this->current_time = 0;
    this->state = Stopped;
}

/// @brief Pause the midi playback
void MidiPlayer::pause()
{
    this->state = Paused;
}

/// @brief Internal playback function
void MidiPlayer::_process(double delta)
{
    if(this->state == PlayerState::Playing)
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

            for (size_t j = index_off; j < events.size(); j++)
            {
                Dictionary event = events[j];
                double event_time = event.get("time", 0);
                
                if (event_time == this->current_time)
                {
                    Array args;
                    args.append(event);
                    if (event.get("type", "undef") == "meta")
                    {
                        this->meta_callback.callv(args);

                    } else if (event.get("type", "undef") == "note")
                    {
                        this->note_callback.callv(args);

                    } else if (event.get("type", "undef") == "system")
                    {
                        this->note_callback.callv(args);
                    } else {
                        UtilityFunctions::printerr("[GodotMidi] Invalid event type");
                    }
                }
            }
        }

        if (has_more_events == false)
        {
            // loop by default
            // TODO: add loop property
            this->current_time = 0;
            this->track_index_offsets.clear();
        }
        
        // increment time, current time will hold the
        // number of seconds since starting
        this->current_time += delta;   
    }
}