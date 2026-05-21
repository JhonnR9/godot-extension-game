#include "crosshair.h"

using namespace godot;

void Crosshair::_draw() {
	Vector2 center = get_size() * 0.5f;

	float size = 6.0f;

	draw_line(
		center + Vector2(-size, 0),
		center + Vector2(size, 0),
		Color(1,1,1),
		2.0f
	);

	draw_line(
		center + Vector2(0, -size),
		center + Vector2(0, size),
		Color(1,1,1),
		2.0f
	);
}

void Crosshair::_bind_methods() {
}