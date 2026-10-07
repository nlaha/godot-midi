extends GdUnitTestSuite

const MIDI_FILES := [
	"res://invent1.mid",
	"res://rhythm_game_track.mid",
	"res://test_low_tempo.mid",
]

const NOTE_TIMEOUT_MSEC := 2000


class NoteCounter:
	extends RefCounted

	var count := 0

	func on_note(_event: Dictionary, _track_index: int) -> void:
		count += 1


func test_midi_files_load_with_tracks_and_notes() -> void:
	for midi_path in MIDI_FILES:
		var midi := MidiResource.new()
		var error := midi.load_file(midi_path)

		assert_int(error).is_equal(OK)
		if error != OK:
			continue

		assert_int(midi.get_track_count()).is_greater(0)
		assert_int(midi.get_tracks().size()).is_equal(midi.get_track_count())

		var note_count := 0
		for track in midi.get_tracks():
			for event in track.get("events", []):
				if event.get("type", "") == "note":
					note_count += 1
		assert_int(note_count).is_greater(0)


func test_midi_files_play_note_events() -> void:
	for midi_path in MIDI_FILES:
		var midi := MidiResource.new()
		var error := midi.load_file(midi_path)
		assert_int(error).is_equal(OK)
		if error != OK:
			continue

		var player := MidiPlayer.new()
		add_child(player)
		player.midi = midi

		var note_counter := NoteCounter.new()
		player.note.connect(note_counter.on_note)
		player.play()

		assert_int(player.get_state()).is_equal(0) # Playing

		var start_time_msec := Time.get_ticks_msec()
		while note_counter.count == 0 and Time.get_ticks_msec() - start_time_msec < NOTE_TIMEOUT_MSEC:
			await get_tree().process_frame

		assert_int(note_counter.count).is_greater(0)
		assert_float(player.current_time).is_greater(0.0)

		player.stop()
		player.queue_free()
