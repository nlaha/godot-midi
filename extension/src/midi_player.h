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

        ADD_SIGNAL(MethodInfo("finished"));

        ADD_SIGNAL(MethodInfo("note"));
        ADD_SIGNAL(MethodInfo("meta"));
        ADD_SIGNAL(MethodInfo("system"));
    };

private:
    Ref<MidiResource> midi;

    PlayerState state;
    double current_time;
    Array track_index_offsets;
    bool loop;

public:
    virtual void _process(double delta) override;

    MidiPlayer();
    ~MidiPlayer();

    void play();
    void stop();
    void pause();

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

    void set_loop(bool loop)
    {
        this->loop = loop;
    };

    void set_current_time(double current_time)
    {
        this->current_time = current_time;
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