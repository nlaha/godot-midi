#ifndef MIDI_PLAYER_H
#define MIDI_PLAYER_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/audio_stream.hpp>

#include <thread>

#include "midi_resource.h"
#include "midi_parser.h"

using namespace godot;

enum PlayerState
{
    Playing,
    Paused,
    Stopped
};

/// @brief MidiPlayer class, responsible for playing back a MidiResource in real-time
class MidiPlayer : public Node
{
    GDCLASS(MidiPlayer, Node);

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_midi", "midi"), &MidiPlayer::set_midi);
        ClassDB::bind_method(D_METHOD("get_midi"), &MidiPlayer::get_midi);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "midi", PROPERTY_HINT_RESOURCE_TYPE, "MidiResource"), "set_midi", "get_midi");

        ClassDB::bind_method(D_METHOD("play"), &MidiPlayer::play);
        ClassDB::bind_method(D_METHOD("stop"), &MidiPlayer::stop);
        ClassDB::bind_method(D_METHOD("pause"), &MidiPlayer::pause);
        ClassDB::bind_method(D_METHOD("resume"), &MidiPlayer::resume);
        ClassDB::bind_method(D_METHOD("get_state"), &MidiPlayer::get_state);

        ClassDB::bind_method(D_METHOD("get_current_time"), &MidiPlayer::get_current_time);
        ClassDB::bind_method(D_METHOD("set_current_time", "current_time"), &MidiPlayer::set_current_time);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "current_time"), "set_current_time", "get_current_time");

        ClassDB::bind_method(D_METHOD("get_loop"), &MidiPlayer::get_loop);
        ClassDB::bind_method(D_METHOD("set_loop", "loop"), &MidiPlayer::set_loop);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop"), "set_loop", "get_loop");

        ClassDB::bind_method(D_METHOD("get_speed_scale"), &MidiPlayer::get_speed_scale);
        ClassDB::bind_method(D_METHOD("set_speed_scale", "speed_scale"), &MidiPlayer::set_speed_scale);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale"), "set_speed_scale", "get_speed_scale");

        ClassDB::bind_method(D_METHOD("link_audio_stream_player", "audio_stream_player"), &MidiPlayer::link_audio_stream_player);

        ClassDB::bind_method(D_METHOD("process_delta", "delta"), &MidiPlayer::process_delta);

        ClassDB::bind_method(D_METHOD("loop_internal"), &MidiPlayer::loop_internal);
        ClassDB::bind_method(D_METHOD("loop_or_stop_thread_safe"), &MidiPlayer::loop_or_stop_thread_safe);

        ADD_SIGNAL(MethodInfo("finished"));

        ADD_SIGNAL(MethodInfo("note"));
        ADD_SIGNAL(MethodInfo("meta"));
        ADD_SIGNAL(MethodInfo("system"));
    };

private:
    /// @brief The midi resource to play
    Ref<MidiResource> midi;

    /// @brief The current state of the player
    std::atomic<PlayerState> state;

    /// @brief The current time in seconds
    double current_time;

    /// @brief The current track index offsets
    Array track_index_offsets;

    /// @brief Whether to loop the midi playback
    bool loop;

    /// @brief The speed scale of the midi playback (1.0 = normal speed, 2.0 = double speed, 0.5 = half speed, etc.)
    double speed_scale;

    /// @brief Track previous time for delta calculation
    Array prev_track_times;
    
    /// @brief The linked AudioStreamPlayer (optional)
    std::vector<AudioStreamPlayer*> asps;
    AudioStreamPlayer* longest_asp;

    /// @brief The playback thread for playing back the midi on a separate thread
    std::thread playback_thread;
    
    /// @brief The audio output latency from the audio server
    double audio_output_latency;

    /// @brief Whether the audio stream player is linked
    bool has_asp ;

    /// @brief Whether to automatically stop the audio stream player when the midi player stops
    bool auto_stop = true;

    void threaded_playback();

    void loop_or_stop_thread_safe();

public:
    void process_delta(double delta);

    MidiPlayer();
    ~MidiPlayer();

    void play();
    void stop();
    void pause();
    void resume();
    void loop_internal();

    // process
    void _process(float delta);

    void link_audio_stream_player(Array asps);

    double get_speed_scale()
    {
        return this->speed_scale;
    };

    bool get_loop()
    {
        return this->loop;
    };

    int get_state()
    {
        return (int)this->state.load();
    };

    double get_current_time()
    {
        return this->current_time;
    };

    void set_speed_scale(double speed_scale)
    {
        this->speed_scale = speed_scale;
    };

    void set_loop(bool loop)
    {
        this->loop = loop;
    };

    /// @brief Sets the current time and updates the track index offsets
    /// @param current_time 
    void set_current_time(double current_time)
    {
        this->current_time = current_time;

        // update track_index_offsets
        for (uint64_t i = 0; i < this->midi->get_track_count(); i++)
        {
            // get events for this track
            Array events = this->midi->get_tracks()[i].get("events");

            // search forward in time
            for (uint64_t j = 0; j < events.size(); j++)
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

                // update the offset if the event time is less than the current time
                if (this->current_time >= event_absolute_time)
                {
                    this->track_index_offsets[i] = j;
                }
                else
                {
                    break;
                }

                // update the previous track time
                this->prev_track_times[i] = event_absolute_time;
            }
        }
    };

    void set_midi(const Ref<MidiResource> &midi)
    {
        this->midi = midi;
        if (this->midi != NULL)
        {
            // initialize track_index_offsets
            this->track_index_offsets.clear();
            this->track_index_offsets.resize(this->midi->get_track_count());
        }
    };

    Ref<MidiResource> get_midi()
    {
        return this->midi;
    };
};

#endif // MIDI_PLAYER_H