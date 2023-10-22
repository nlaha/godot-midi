shader_type spatial;

uniform vec3 color: source_color;

void fragment() {
	
	ALBEDO = color;
	EMISSION = color * 5.0;
}
