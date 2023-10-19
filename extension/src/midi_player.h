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
#include <atomic>

#include "midi_resource.h"

using namespace godot;

enum PlayerState
{
    Playing,
    Paused,
    Stopped
};

class MidiPlayer : public Node
{
    GDCLASS(MidiPlayer, Node);

protected:

    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_midi", "midi"), &MidiPlayer::set_midi);
        ClassDB::bind_method(D_METHOD("get_midi"), &MidiPlayer::get_midi);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "midi"), "set_midi", "get_midi");

        ClassDB::bind_method(D_METHOD("play"), &MidiPlayer::play);
        ClassDB::bind_method(D_METHOD("stop"), &MidiPlayer::stop);
        ClassDB::bind_method(D_METHOD("pause"), &MidiPlayer::pause);
        ClassDB::bind_method(D_METHOD("get_state"), &MidiPlayer::get_state);

        ClassDB::bind_method(D_METHOD("get_current_time"), &MidiPlayer::get_current_time);
        ClassDB::bind_method(D_METHOD("set_current_time", "current_time"), &MidiPlayer::set_current_time);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "current_time"), "set_current_time", "get_current_time");

        ClassDB::bind_method(D_METHOD("get_meta_callback"), &MidiPlayer::get_meta_callback);
        ClassDB::bind_method(D_METHOD("set_meta_callback", "meta_callback"), &MidiPlayer::set_meta_callback);
        ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "meta_callback"), "set_meta_callback", "get_meta_callback");

        ClassDB::bind_method(D_METHOD("get_note_callback"), &MidiPlayer::get_note_callback);
        ClassDB::bind_method(D_METHOD("set_note_callback", "meta_callback"), &MidiPlayer::set_note_callback);
        ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "meta_callback"), "set_note_callback", "get_note_callback");

        ClassDB::bind_method(D_METHOD("get_system_callback"), &MidiPlayer::get_system_callback);
        ClassDB::bind_method(D_METHOD("set_system_callback", "meta_callback"), &MidiPlayer::set_system_callback);
        ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "meta_callback"), "set_system_callback", "get_system_callback");
        
    };

private:
    Ref<MidiResource> midi;

    std::atomic<PlayerState> state;
    double current_time;
    Array track_index_offsets;

    Callable meta_callback;
    Callable note_callback;
    Callable system_callback;

public:
    virtual void _process(double delta) override;

    MidiPlayer();
    ~MidiPlayer();

    void play();
    void stop();
    void pause();

    int get_state()
    {
        return (int)this->state;
    };

    double get_current_time()
    {
        return this->current_time;
    };

    void set_current_time(double current_time)
    {
        this->current_time = current_time;
    };

    void set_midi(Ref<MidiResource> midi)
    {
        this->midi = midi;
    };

    Ref<MidiResource> get_midi()
    {
        return this->midi;
    };

    // callable getters and setters
    Callable get_meta_callback()
    {
        return this->meta_callback;
    }

    void set_meta_callback(Callable cb)
    {
        this->meta_callback = cb;
    }

    Callable get_note_callback()
    {
        return this->note_callback;
    }
    
    void set_note_callback(Callable cb)
    {
        this->note_callback = cb;
    }

    Callable get_system_callback()
    {
        return this->system_callback;
    }

    void set_system_callback(Callable cb)
    {
        this->system_callback = cb;
    }
};

#endif // MIDI_PLAYER_H