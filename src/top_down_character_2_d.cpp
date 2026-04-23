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
	external_force += p_force;
}
void TopDownCharacter2D::_ready() {
	CharacterBody2D::_ready();
}

void TopDownCharacter2D::_physics_process(float delta) {
	Vector2 new_velocity = get_velocity();
	Vector2 total_input = input_dir + external_force;

	if (input_dir.length_squared() >= 0.01) {
		new_velocity += total_input * acceleration * delta;
	} else {
		new_velocity = new_velocity.move_toward(Vector2(), friction * delta);
	}

	if(new_velocity.length_squared() > max_velocity * max_velocity) {
		new_velocity = new_velocity.normalized() * max_velocity;
	}

	set_velocity(new_velocity);
	move_and_slide();

	input_dir = Vector2();
	external_force = Vector2();
}

void TopDownCharacter2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_input", "input"), &TopDownCharacter2D::add_input);
	ClassDB::bind_method(D_METHOD("add_force", "force"), &TopDownCharacter2D::add_force);

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
TopDownCharacter2D::TopDownCharacter2D() : max_velocity(300), acceleration(20), friction(20), external_force(Vector2()) {
}
