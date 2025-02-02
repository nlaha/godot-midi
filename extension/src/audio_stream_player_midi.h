#ifndef AUDIO_STREAM_PLAYER_MIDI_CLASS_H
#define AUDIO_STREAM_PLAYER_MIDI_CLASS_H

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <memory>
#include "utility.h"
#include "sf2_resource.h"

using namespace godot;

/// @brief A class that plays midi files as audio via soundfonts
class AudioStreamPlayerMidi : public AudioStreamPlayer
{
    GDCLASS(AudioStreamPlayerMidi, AudioStreamPlayer);

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("note_on", "note", "velocity", "preset"), &AudioStreamPlayerMidi::note_on);
        ClassDB::bind_method(D_METHOD("note_off", "note", "preset"), &AudioStreamPlayerMidi::note_off);

        ClassDB::bind_method(D_METHOD("get_sample_rate"), &AudioStreamPlayerMidi::get_sample_rate);
        ClassDB::bind_method(D_METHOD("set_sample_rate", "sample_rate"), &AudioStreamPlayerMidi::set_sample_rate);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "sample_rate"), "set_sample_rate", "get_sample_rate");

        ClassDB::bind_method(D_METHOD("get_soundfont"), &AudioStreamPlayerMidi::get_soundfont);
        ClassDB::bind_method(D_METHOD("set_soundfont", "soundfont"), &AudioStreamPlayerMidi::set_soundfont);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "soundfont", PROPERTY_HINT_RESOURCE_TYPE, "Sf2Resource"), "set_soundfont", "get_soundfont");
    }

    /// @brief The playback reference of the audio stream player
    Ref<AudioStreamGeneratorPlayback> playback;

    /// @brief The sample rate of the audio stream player
    int sample_rate;

    /// @brief The soundfont resource
    Ref<Sf2Resource> soundfont;

private:
    Ref<AudioStream> stream;

    tsf *sf2_handle;

public:
    AudioStreamPlayerMidi();

    ~AudioStreamPlayerMidi();

    void _ready() override;
    void _process(float delta);

    void note_on(int note, float velocity, int preset);
    void note_off(int note, int preset);

    int get_sample_rate() const { return this->sample_rate; }
    void set_sample_rate(int sample_rate) { this->sample_rate = sample_rate; }

    Ref<Sf2Resource> get_soundfont() const { return this->soundfont; }
    void set_soundfont(const Ref<Sf2Resource> &soundfont) { this->soundfont = soundfont; }
};

#endif // AUDIO_STREAM_PLAYER_MIDI_CLASS_H