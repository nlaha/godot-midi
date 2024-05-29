#ifndef MIDI_RESOURCE_H
#define MIDI_RESOURCE_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/file_access.hpp>

#include "midi_resource.h"

using namespace godot;

/// @brief MidiResource class, responsible for loading and parsing a midi file, then storing it in a format that can be played back
class MidiResource : public Resource
{
    GDCLASS(MidiResource, Resource);

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_format", "format"), &MidiResource::set_format);
        ClassDB::bind_method(D_METHOD("get_format"), &MidiResource::get_format);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "format"), "set_format", "get_format");

        ClassDB::bind_method(D_METHOD("set_track_count", "track_count"), &MidiResource::set_track_count);
        ClassDB::bind_method(D_METHOD("get_track_count"), &MidiResource::get_track_count);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "track_count"), "set_track_count", "get_track_count");

        ClassDB::bind_method(D_METHOD("set_division", "division"), &MidiResource::set_division);
        ClassDB::bind_method(D_METHOD("get_division"), &MidiResource::get_division);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "division"), "set_division", "get_division");

        ClassDB::bind_method(D_METHOD("set_tempo", "tempo"), &MidiResource::set_tempo);
        ClassDB::bind_method(D_METHOD("get_tempo"), &MidiResource::get_tempo);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "tempo"), "set_tempo", "get_tempo");

        ClassDB::bind_method(D_METHOD("set_tracks", "tracks"), &MidiResource::set_tracks);
        ClassDB::bind_method(D_METHOD("get_tracks"), &MidiResource::get_tracks);
        ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tracks"), "set_tracks", "get_tracks");

        // save and load methods
        ClassDB::bind_method(D_METHOD("load_file", "path"), &MidiResource::load_file);
        ClassDB::bind_method(D_METHOD("save_file", "path", "resource"), &MidiResource::save_file);
    }

private:
    int format;
    int track_count;
    int division;
    int tempo;
    Array tracks;

public:
    Error load_file(const String &p_path);
    Error save_file(const String &p_path, const Ref<Resource> &p_resource);

    // getters and setters

    /// @brief Sets the format of the midi file, see MidiParser::MidiHeaderChunk::MidiFileFormat
    /// @param p_format
    inline void set_format(int p_format) { format = p_format; }

    /// @brief Gets the format of the midi file, see MidiParser::MidiHeaderChunk::MidiFileFormat
    /// @return
    inline int get_format() const { return format; }

    /// @brief Sets the number of tracks in the midi file
    /// @param p_track_count
    inline void set_track_count(int p_track_count) { track_count = p_track_count; }

    /// @brief Gets the number of tracks in the midi file
    /// @return
    inline int get_track_count() const { return track_count; }

    /// @brief Sets the division of the midi file in ticks per quarter note
    /// @param p_division
    inline void set_division(int p_division) { division = p_division; }

    /// @brief Gets the division of the midi file in ticks per quarter note
    /// @return
    inline int get_division() const { return division; }

    /// @brief Sets the tempo in microseconds per quarter note
    /// @param p_tempo
    inline void set_tempo(int p_tempo) { tempo = p_tempo; }

    /// @brief Gets the tempo in microseconds per quarter note
    /// @return
    inline int get_tempo() const { return tempo; }

    /// @brief Sets the tracks of the midi file
    /// @param p_tracks
    inline void set_tracks(Array p_tracks) { tracks = p_tracks; }

    /// @brief Gets the tracks of the midi file
    /// @return
    inline Array get_tracks() const { return tracks; }
};

#endif // MIDI_RESOURCE_H