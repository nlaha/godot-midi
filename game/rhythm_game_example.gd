extends Node2D

@export var look_ahead := 2.0 ## seconds of upcoming notes to show
@export var look_behind := 0.5 ## seconds behind current time to still display
@export var track_speed := 450.0 ## pixels per second
@export var note_radius := 28.0
@export var hit_x := 180.0 ## x position of the hit zone
@export var hit_window := 0.2 ## seconds for a valid hit

var midi_player: MidiPlayer
var asp: AudioStreamPlayer

var score := 0
var feedback := ""
var feedback_tmr := 0.0

var countdown_timer := 3.0
var game_started := false
var current_display_time := 0.0

# keys we've already scored so we don't double-count
var hit_keys := {}
# keys that have passed without being hit so we don't score them late
var miss_keys := {}

# note events visible this frame (already filtered)
var visible_notes: Array = []

# shockwave rings: Array of {pos, age, lifetime, color, start_r}
var shockwaves: Array = []
var hit_flash_tmr := 0.0
var hit_flash_color := Color.WHITE


func _note_key(e: Dictionary) -> String:
	return str(e.get("time", 0.0)) + ":" + str(e.get("note", 0))


func _ready() -> void:
	midi_player = $MidiPlayer
	asp = $AudioStreamPlayer
	midi_player.link_audio_stream_player([asp])


func _process(delta: float) -> void:
	feedback_tmr = max(0.0, feedback_tmr - delta)
	hit_flash_tmr = max(0.0, hit_flash_tmr - delta)
	if feedback_tmr <= 0.0:
		feedback = ""

	# age shockwaves, remove expired ones
	for i in range(shockwaves.size() - 1, -1, -1):
		shockwaves[i].age += delta
		if shockwaves[i].age >= shockwaves[i].lifetime:
			shockwaves.remove_at(i)

	if not game_started:
		countdown_timer -= delta
		# display_time = -countdown_timer: at ct=look_ahead the first note
		# enters the right edge; at ct=0 it reaches the hit zone as audio starts
		current_display_time = -countdown_timer
		if countdown_timer <= 0.0:
			game_started = true
			midi_player.play()
	else:
		current_display_time = midi_player.current_time

	var t := current_display_time
	visible_notes = []

	for e in midi_player.get_notes_around(t, look_behind, look_ahead):
		# only note-on events matter for gameplay
		if not e.get("active", false):
			continue
		var key := _note_key(e)
		if hit_keys.has(key):
			continue
		# note has passed the hit window without being hit - mark missed
		if game_started and float(e.get("time", 0.0)) < t - hit_window:
			miss_keys[key] = true
			continue
		visible_notes.append(e)

	$CanvasLayer/ScoreLabel.text = "Score: %d" % score
	$CanvasLayer/FeedbackLabel.text = feedback
	queue_redraw()


func _draw() -> void:
	var vp := get_viewport_rect().size
	var track_y := vp.y * 0.5
	var t := current_display_time

	# track rail
	draw_line(Vector2(0.0, track_y), Vector2(vp.x, track_y), Color(0.2, 0.2, 0.2), 6.0)

	# hit zone: filled ring + optional flash
	var zone_pos := Vector2(hit_x, track_y)
	if hit_flash_tmr > 0.0:
		var fa := hit_flash_tmr / 0.35
		draw_circle(zone_pos, (note_radius + 12.0) * (1.0 + 0.45 * fa), Color(hit_flash_color.r, hit_flash_color.g, hit_flash_color.b, fa * 0.45))
	draw_circle(zone_pos, note_radius + 12.0, Color(0.7, 0.7, 0.7, 0.15))
	draw_arc(zone_pos, note_radius + 12.0, 0.0, TAU, 48, Color(1.0, 1.0, 1.0, 0.7), 3.0)

	# shockwave rings
	for wave in shockwaves:
		var tw: float = wave.age / wave.lifetime
		var radius: float = lerpf(wave.start_r, wave.start_r * 3.2, tw)
		var alpha: float = 1.0 - tw
		var width: float = lerpf(5.0, 1.0, tw)
		draw_arc(wave.pos, radius, 0.0, TAU, 48, Color(wave.color.r, wave.color.g, wave.color.b, alpha), width)

	# notes
	for e in visible_notes:
		var note_time := float(e.get("time", 0.0))
		var dt := note_time - t
		var x := hit_x + dt * track_speed
		if x < -note_radius * 2.0 or x > vp.x + note_radius:
			continue
		var is_don := int(e.get("note", 0)) % 2 == 0
		var fill := Color(0.9, 0.2, 0.2) if is_don else Color(0.2, 0.45, 0.95)
		var border := Color(1.0, 0.6, 0.6) if is_don else Color(0.6, 0.8, 1.0)
		draw_circle(Vector2(x, track_y), note_radius, fill)
		draw_arc(Vector2(x, track_y), note_radius, 0.0, TAU, 48, border, 3.0)

	# countdown overlay
	if not game_started:
		var font := ThemeDB.fallback_font
		var font_size := 120
		var txt := str(ceili(countdown_timer))
		var txt_w := font.get_string_size(txt, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x
		draw_string(font, Vector2((vp.x - txt_w) * 0.5, vp.y * 0.5 - 40.0), txt,
				HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(1.0, 1.0, 1.0, 0.9))


func _spawn_hit_effect(pos: Vector2, is_hit: bool, is_don: bool) -> void:
	# basic burst - shown on every press
	var p := CPUParticles2D.new()
	add_child(p)
	p.position = pos
	p.one_shot = true
	p.explosiveness = 1.0
	p.lifetime = 0.35
	p.amount = 10
	p.spread = 180.0
	p.initial_velocity_min = 80.0
	p.initial_velocity_max = 160.0
	p.gravity = Vector2(0.0, 220.0)
	p.scale_amount_min = 2.0
	p.scale_amount_max = 4.0
	p.color = Color(0.85, 0.85, 0.85)
	p.emitting = true
	get_tree().create_timer(p.lifetime + 0.1).timeout.connect(p.queue_free)

	if not is_hit:
		return

	# fancy burst - only on a valid hit
	var note_color := Color(0.9, 0.2, 0.2) if is_don else Color(0.2, 0.45, 0.95)

	var p2 := CPUParticles2D.new()
	add_child(p2)
	p2.position = pos
	p2.one_shot = true
	p2.explosiveness = 1.0
	p2.lifetime = 0.7
	p2.amount = 28
	p2.spread = 180.0
	p2.initial_velocity_min = 180.0
	p2.initial_velocity_max = 420.0
	p2.gravity = Vector2(0.0, 260.0)
	p2.scale_amount_min = 4.0
	p2.scale_amount_max = 9.0
	p2.color = note_color
	p2.emitting = true
	get_tree().create_timer(p2.lifetime + 0.1).timeout.connect(p2.queue_free)

	# expanding shockwave ring
	shockwaves.append({
		"pos": pos,
		"age": 0.0,
		"lifetime": 0.45,
		"color": note_color,
		"start_r": note_radius + 12.0,
	})

	# flash the hit zone
	hit_flash_tmr = 0.35
	hit_flash_color = note_color


func _input(event: InputEvent) -> void:
	if not game_started:
		return
	if event is InputEventKey and event.pressed and not event.echo:
		var t := midi_player.current_time
		if event.keycode == KEY_D:
			_try_hit(t, true) # Don - red
		elif event.keycode == KEY_F:
			_try_hit(t, false) # Ka  - blue


func _try_hit(t: float, is_don: bool) -> void:
	var best_dt := INF
	var best_note := {}

	for e in visible_notes:
		if (int(e.get("note", 0)) % 2 == 0) != is_don:
			continue
		var dt := absf(float(e.get("time", 0.0)) - t)
		if dt < best_dt:
			best_dt = dt
			best_note = e

	var effect_pos := Vector2(hit_x, get_viewport_rect().size.y * 0.5)

	if not best_note.is_empty() and best_dt <= hit_window:
		var key := _note_key(best_note)
		if not hit_keys.has(key) and not miss_keys.has(key):
			hit_keys[key] = true
			score += 1
			var signed_ms := roundi((float(best_note.get("time", 0.0)) - t) * 1000.0)
			var ms_str := ("+" if signed_ms >= 0 else "") + str(signed_ms) + "ms"
			if best_dt <= 0.02:
				feedback = "Awesome! " + ms_str
			elif best_dt <= 0.1:
				feedback = "OK " + ms_str
			else:
				feedback = "Terrible! " + ms_str
			feedback_tmr = 0.5
			_spawn_hit_effect(effect_pos, true, is_don)
			return

	# miss - still show a press effect
	_spawn_hit_effect(effect_pos, false, is_don)
	if not best_note.is_empty():
		var signed_ms := roundi((float(best_note.get("time", 0.0)) - t) * 1000.0)
		var ms_str := ("+" if signed_ms >= 0 else "") + str(signed_ms) + "ms"
		feedback = "Miss! " + ms_str
	else:
		feedback = "Miss!"
	feedback_tmr = 0.5
