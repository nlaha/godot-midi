#define TSF_IMPLEMENTATION
#include "tsf.h"

#include "audio_stream_player_midi.h"

AudioStreamPlayerMidi::AudioStreamPlayerMidi()
{
    this->sample_rate = 44100;

    // create an audio stream generator
    this->stream = Ref<AudioStream>(memnew(AudioStreamGenerator()));
}

AudioStreamPlayerMidi::~AudioStreamPlayerMidi()
{
}

/// @brief Called when the node is ready
void AudioStreamPlayerMidi::_ready()
{
    if (this->get_stream() == nullptr)
    {
        this->set_stream(this->stream);
    }

    // don't run in editor
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    this->play();

    if (this->has_stream_playback())
    {
        this->playback = (Ref<AudioStreamGeneratorPlayback>)this->get_stream_playback();

        // log the sample rate
        UtilityFunctions::print("[GodotMidi] Sample Rate: " + itos(this->sample_rate));
    }

    if (this->soundfont == nullptr)
    {
        return;
    }

    // print size of file buffer
    UtilityFunctions::print("[GodotMidi] File Buffer Size: " + itos(this->soundfont->get_file_buffer().size()));

    this->sf2_handle = tsf_load_memory(this->soundfont->get_file_buffer().ptr(), this->soundfont->get_file_buffer().size());
    if (this->sf2_handle == nullptr)
    {
        UtilityFunctions::print("[GodotMidi] Failed to parse soundfont");
        return;
    }

    // set up soundfont
    tsf_set_output(this->sf2_handle, TSF_STEREO_INTERLEAVED, this->sample_rate, 0);

    // log the size of the sf2_handle
    UtilityFunctions::print("[GodotMidi] Num SF2 Presets: " + itos(this->sf2_handle->presetNum));

    // log the soundfont
    UtilityFunctions::print("[GodotMidi] Soundfont: " + this->soundfont->get_path());
}

/// @brief Process the audio stream player
/// @param delta the delta time
void AudioStreamPlayerMidi::_process(float delta)
{
    // don't run in editor
    if (Engine::get_singleton()->is_editor_hint())
    {
        return;
    }

    if (this->sf2_handle == nullptr || this->playback == nullptr || this->is_playing() == false)
    {
        return;
    }

    int frames_available = this->playback->get_frames_available();

    if (frames_available == 0)
    {
        return;
    }

    PackedFloat32Array buffer = PackedFloat32Array();
    buffer.resize(frames_available * 2); 

    tsf_render_float(this->sf2_handle, buffer.ptrw(), frames_available, 0);

    // iterate through the buffer and push the frames to the playback 
    // even indices are the left channel, odd indices are the right channel
    for (int i = 0; i < buffer.size(); i += 2)
    {
        float left = buffer[i];
        float right = buffer[i + 1];
        this->playback->push_frame(Vector2(left, right));
    }
}

/// @brief Called for every note event in the linked midi player
/// @param note the note number
/// @param velocity the velocity of the note
/// @param preset the preset number

/// @param preset the preset number
void AudioStreamPlayerMidi::note_on(int note, float velocity, int preset)
{
    if (this->sf2_handle == nullptr)
    {

        return;
    }

    tsf_note_on(this->sf2_handle, preset, note, velocity);
}

/// @brief Called for every note off event in the linked midi player
/// @param note
/// @param preset
void AudioStreamPlayerMidi::note_off(int note, int preset)
{
    if (this->sf2_handle == nullptr)
    {
        return;
    }

    tsf_note_off(this->sf2_handle, preset, note);
}
