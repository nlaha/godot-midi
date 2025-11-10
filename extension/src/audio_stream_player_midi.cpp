#define TSF_IMPLEMENTATION
#include "tsf.h"

#include "audio_stream_player_midi.h"

std::mutex tsf_mutex;

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
        Object::cast_to<AudioStreamGenerator>(this->stream.ptr())->set_mix_rate(this->sample_rate);

        this->playback = (Ref<AudioStreamGeneratorPlayback>)this->get_stream_playback();

        // log the sample rate
        UtilityFunctions::print("[GodotMidi ASPM] Sample Rate: " + itos(this->sample_rate));
    }

    if (this->soundfont == nullptr)
    {
        return;
    }

    tsf_mutex.lock();

    // print size of file buffer
    UtilityFunctions::print("[GodotMidi ASPM] File Buffer Size: " + itos(this->soundfont->get_file_buffer().size()));

    this->sf2_handle = tsf_load_memory(this->soundfont->get_file_buffer().ptr(), this->soundfont->get_file_buffer().size());
    if (this->sf2_handle == nullptr)
    {
        UtilityFunctions::print("[GodotMidi ASPM] Failed to parse soundfont");
        return;
    }

    // set up soundfont
    tsf_set_output(this->sf2_handle, TSF_STEREO_INTERLEAVED, this->sample_rate, 0);

    // Initialize preset on special 10th MIDI channel to use percussion sound bank (128) if available
    tsf_channel_set_bank_preset(this->sf2_handle, 9, 128, 0);

    // log the size of the sf2_handle
    UtilityFunctions::print("[GodotMidi ASPM] Num SF2 Presets: " + itos(this->sf2_handle->presetNum));

    // log the soundfont
    UtilityFunctions::print("[GodotMidi ASPM] Soundfont: " + this->soundfont->get_path());

    tsf_mutex.unlock();

    this->play();
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

    fill_buffer();
}

/// @brief Fill the audio buffer with samples from the soundfont
/// This function is called every frame to fill the audio buffer with samples from the soundfont
/// It uses the tsf library to render the audio samples and push them to the playback buffer
void AudioStreamPlayerMidi::fill_buffer()
{
    if (this->sf2_handle == nullptr || this->playback == nullptr || this->is_playing() == false)
    {
        return;
    }

    int frames_available = this->playback->get_frames_available();

    if (frames_available == 0)
    {
        return;
    }
    else
    {
        // log the number of frames available
        UtilityFunctions::print("[GodotMidi ASPM] Frames Available: " + itos(frames_available));
    }

    PackedFloat32Array buffer = PackedFloat32Array();
    buffer.resize(frames_available * 2);

    tsf_mutex.lock();
    tsf_render_float(this->sf2_handle, buffer.ptrw(), frames_available, 0);
    tsf_mutex.unlock();

    // iterate through the buffer and push the frames to the playback
    // even indices are the left channel, odd indices are the right channel
    for (int i = 0; i < buffer.size(); i += 2)
    {
        float left = buffer[i];
        float right = buffer[i + 1];
        UtilityFunctions::print("[GodotMidi ASPM] Pushing Frame: " + itos(i / 2) + " Left: " + rtos(left) + " Right: " + rtos(right));
        this->playback->push_frame(Vector2(left, right));
    }
}

/// @brief Called for every note event in the linked midi player
/// @param note the note number
/// @param velocity the velocity of the note
/// @param channel the channel number
void AudioStreamPlayerMidi::note_on(int note, float velocity, int channel)
{
    if (this->sf2_handle == nullptr)
    {
        return;
    }

    tsf_mutex.lock();
    tsf_channel_note_on(this->sf2_handle, channel, note, velocity);
    tsf_mutex.unlock();
}

/// @brief Called for every note off event in the linked midi player
/// @param note the note number
/// @param channel the channel number
void AudioStreamPlayerMidi::note_off(int note, int channel)
{
    if (this->sf2_handle == nullptr)
    {
        return;
    }

    tsf_mutex.lock();
    tsf_channel_note_off(this->sf2_handle, channel, note);
    tsf_mutex.unlock();
}

/// @brief Called for every program change event in the linked midi player
/// @param channel the channel number
/// @param preset the preset number
void AudioStreamPlayerMidi::program_change(int channel, int preset)
{
    if (this->sf2_handle == nullptr)
    {
        return;
    }

    // log the program change
    UtilityFunctions::print("[GodotMidi ASPM] Program Change: Channel " + itos(channel) + ", Preset " + itos(preset));

    tsf_mutex.lock();
    tsf_channel_set_presetnumber(this->sf2_handle, channel, preset, (channel == 9) ? 1 : 0);
    tsf_mutex.unlock();
}

/// @brief Called for every pitch bend event in the linked midi player
/// @param channel the channel number
/// @param bend the pitch bend value
void AudioStreamPlayerMidi::pitch_bend(int channel, int bend)
{
    if (this->sf2_handle == nullptr)
    {
        return;
    }

    tsf_mutex.lock();
    tsf_channel_set_pitchwheel(this->sf2_handle, channel, bend);
    tsf_mutex.unlock();
}

/// @brief Called for every control change event in the linked midi player
/// @param channel the channel number
/// @param control the control number
/// @param value the value of the control
void AudioStreamPlayerMidi::control_change(int channel, int control, int value)
{
    if (this->sf2_handle == nullptr)
    {
        return;
    }

    tsf_mutex.lock();
    tsf_channel_midi_control(this->sf2_handle, channel, control, value);
    tsf_mutex.unlock();
}