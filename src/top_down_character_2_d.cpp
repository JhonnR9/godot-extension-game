//
// Created by jhone on 22/04/2026.
//

#include "top_down_character_2_d.h"

#include <algorithm>

using namespace godot;

void TopDownCharacter2D::add_input(const Vector2 p_input) {
	input_dir += p_input;

	if (input_dir.length_squared() > 1.0f) {
		input_dir = input_dir.normalized();
	}
}
void TopDownCharacter2D::add_force(Vector2 p_force) {
	external_forces += p_force;
}
void TopDownCharacter2D::add_impulse(Vector2 p_impulse) {
	pending_impulses += p_impulse;
}
void TopDownCharacter2D::_ready() {
	CharacterBody2D::_ready();
}

void TopDownCharacter2D::_physics_process(float delta) {
	Vector2 current_velocity = get_velocity();

	if (input_dir.length_squared() > 1.0f) {
		input_dir = input_dir.normalized();
	}
	Vector2 target_input_velocity = input_dir * max_velocity;

	if (input_dir.length_squared() > 0.01f) {
		current_velocity = current_velocity.move_toward(target_input_velocity, acceleration * delta);
	} else {
		current_velocity = current_velocity.move_toward(Vector2(), friction * delta);
	}

	current_velocity += external_forces * delta;
	current_velocity += pending_impulses;

	if (current_velocity.length() > max_velocity) {
		float drag_factor = 2.0f;
		current_velocity = current_velocity.move_toward(current_velocity.normalized() * max_velocity, friction * drag_factor * delta);
	}

	set_velocity(current_velocity);
	move_and_slide();

	input_dir = Vector2();
	external_forces = Vector2();
	pending_impulses = Vector2();
}

void TopDownCharacter2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_input", "input"), &TopDownCharacter2D::add_input);
	ClassDB::bind_method(D_METHOD("add_force", "force"), &TopDownCharacter2D::add_force);
	ClassDB::bind_method(D_METHOD("add_impulse", "impulse"), &TopDownCharacter2D::add_impulse);

	ClassDB::bind_method(D_METHOD("set_max_velocity", "max_velocity"), &TopDownCharacter2D::set_max_velocity);
	ClassDB::bind_method(D_METHOD("get_max_velocity"), &TopDownCharacter2D::get_max_velocity);

	ClassDB::bind_method(D_METHOD("set_acceleration", "acceleration"), &TopDownCharacter2D::set_acceleration);
	ClassDB::bind_method(D_METHOD("get_acceleration"), &TopDownCharacter2D::get_acceleration);

	ClassDB::bind_method(D_METHOD("set_friction", "friction"), &TopDownCharacter2D::set_friction);
	ClassDB::bind_method(D_METHOD("get_friction"), &TopDownCharacter2D::get_friction);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_velocity"), "set_max_velocity", "get_max_velocity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "acceleration"), "set_acceleration", "get_acceleration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "friction"), "set_friction", "get_friction");
}
TopDownCharacter2D::TopDownCharacter2D() : max_velocity(300), acceleration(20), friction(20), external_forces(Vector2()), pending_impulses(Vector2()) {
}
