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

#include "midi_resource.h"

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
        ClassDB::bind_method(D_METHOD("get_state"), &MidiPlayer::get_state);

        ClassDB::bind_method(D_METHOD("get_current_time"), &MidiPlayer::get_current_time);
        ClassDB::bind_method(D_METHOD("set_current_time", "current_time"), &MidiPlayer::set_current_time);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "current_time"), "set_current_time", "get_current_time");

        ClassDB::bind_method(D_METHOD("get_loop"), &MidiPlayer::get_loop);
        ClassDB::bind_method(D_METHOD("set_loop", "loop"), &MidiPlayer::set_loop);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop"), "set_loop", "get_loop");

        ClassDB::bind_method(D_METHOD("get_manual_process"), &MidiPlayer::get_manual_process);
        ClassDB::bind_method(D_METHOD("set_manual_process", "manual_process"), &MidiPlayer::set_manual_process);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "manual_process"), "set_manual_process", "get_manual_process");

        ClassDB::bind_method(D_METHOD("get_speed_scale"), &MidiPlayer::get_speed_scale);
        ClassDB::bind_method(D_METHOD("set_speed_scale", "speed_scale"), &MidiPlayer::set_speed_scale);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale"), "set_speed_scale", "get_speed_scale");

        ClassDB::bind_method(D_METHOD("process_delta", "delta"), &MidiPlayer::process_delta);

        ADD_SIGNAL(MethodInfo("finished"));

        ADD_SIGNAL(MethodInfo("note"));
        ADD_SIGNAL(MethodInfo("meta"));
        ADD_SIGNAL(MethodInfo("system"));
    };

private:
    /// @brief The midi resource to play
    Ref<MidiResource> midi;

    /// @brief The current state of the player
    PlayerState state;

    /// @brief The current time in seconds
    double current_time;

    /// @brief The current track index offsets
    Array track_index_offsets;

    /// @brief Whether to loop the midi playback
    bool loop;

    /// @brief Whether to manually process the midi playback
    bool manual_process;

    /// @brief The speed scale of the midi playback (1.0 = normal speed, 2.0 = double speed, 0.5 = half speed, etc.)
    double speed_scale;

public:
    virtual void _process(double delta) override;
    void process_delta(double delta);

    MidiPlayer();
    ~MidiPlayer();

    void play();
    void stop();
    void pause();

    double get_speed_scale()
    {
        return this->speed_scale;
    };

    bool get_manual_process()
    {
        return this->manual_process;
    };

    bool get_loop()
    {
        return this->loop;
    };

    int get_state()
    {
        return (int)this->state;
    };

    double get_current_time()
    {
        return this->current_time;
    };

    void set_speed_scale(double speed_scale)
    {
        this->speed_scale = speed_scale;
    };

    void set_manual_process(bool manual_process)
    {
        this->manual_process = manual_process;
    };

    void set_loop(bool loop)
    {
        this->loop = loop;
    };

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
                double event_time = event.get("time", 0);

                // update the offset if the event time is less than the current time
                if (this->current_time >= event_time)
                {
                    this->track_index_offsets[i] = j;
                }
                else
                {
                    break;
                }
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