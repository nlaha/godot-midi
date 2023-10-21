shader_type canvas_item;

uniform vec2 scale;

uniform vec4 color1: source_color;
uniform vec4 color2: source_color;

void fragment() {
	vec2 uv = floor(scale * UV.xy);
	vec4 alpha = vec4(vec3(mod(uv.x + uv.y, 2.)), 1);
	COLOR = mix(color1, color2, alpha);
}
