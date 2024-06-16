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

#include <godot_cpp/classes/audio_stream_wav.hpp>

// if we're on windows we need to include windows.h for the midi api
#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include "midi_resource.h"
#include "midi_parser.h"

using namespace godot;

// forward declare cs_audio_source_t
struct cs_audio_source_t;

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

        ClassDB::bind_method(D_METHOD("set_track_sound_effects", "track_sound_effects"), &MidiPlayer::set_track_sound_effects);
        ClassDB::bind_method(D_METHOD("get_track_sound_effects"), &MidiPlayer::get_track_sound_effects);
        ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "track_sound_effects"), "set_track_sound_effects", "get_track_sound_effects");

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

    /// @brief Track previous time for delta calculation
    Array prev_track_times;

    /// @brief Track sound effects, played when a note is played on a track (key = track index, value = AudioStream)
    Dictionary track_sound_effects;

    std::unordered_map<int, cs_audio_source_t*> audio_sources;

public:
    virtual void _process(double delta) override;
    void process_delta(double delta);

    MidiPlayer();
    ~MidiPlayer();

    void play();
    void stop();
    void pause();

    /// @brief Gets the speed scale of the midi playback
    /// @return the speed scale
    double get_speed_scale()
    {
        return this->speed_scale;
    };

    /// @brief Gets whether the midi playback is set to manually process
    /// @return true if the midi playback is set to manually process, false otherwise
    bool get_manual_process()
    {
        return this->manual_process;
    };

    /// @brief Gets whether the midi playback is set to loop
    /// @return true if the midi playback is set to loop, false otherwise
    bool get_loop()
    {
        return this->loop;
    };

    /// @brief Gets the current state of the player
    /// @return the current state of the player
    int get_state()
    {
        return (int)this->state;
    };

    /// @brief Gets the current time in seconds
    /// @return the current time in seconds
    double get_current_time()
    {
        return this->current_time;
    };

    /// @brief Sets the speed scale of the midi playback
    /// @param speed_scale the speed scale
    void set_speed_scale(double speed_scale)
    {
        this->speed_scale = speed_scale;
    };

    /// @brief Sets whether to manually process the midi playback
    /// @param manual_process true for manual processing, false for automatic processing
    void set_manual_process(bool manual_process)
    {
        this->manual_process = manual_process;
    };

    /// @brief Sets whether to loop the midi playback
    /// @param loop true for looping, false for not looping
    void set_loop(bool loop)
    {
        this->loop = loop;
    };

    /// @brief Sets the sound effects of the midi file
    /// @param p_track_sound_effects
    inline void set_track_sound_effects(Dictionary p_track_sound_effects) { track_sound_effects = p_track_sound_effects; }

    /// @brief Gets the sound effects of the midi file
    /// @return
    inline Dictionary get_track_sound_effects() const { return track_sound_effects; }

    /// @brief Sets the current time in seconds
    /// @param current_time the current time in seconds
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

    /// @brief Sets the midi resource to play
    /// @param midi the midi resource to play
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

    /// @brief Gets the midi resource to play
    /// @return the midi resource to play
    Ref<MidiResource> get_midi()
    {
        return this->midi;
    };
};

#endif // MIDI_PLAYER_H