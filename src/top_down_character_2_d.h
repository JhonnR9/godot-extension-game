//
// Created by jhone on 22/04/2026.
//

#ifndef TOP_DOWN_CHARACTER_2_D_H
#define TOP_DOWN_CHARACTER_2_D_H

#include <godot_cpp/classes/character_body2d.hpp>

namespace godot {
class TopDownCharacter2D : public CharacterBody2D {
	GDCLASS(TopDownCharacter2D, CharacterBody2D)

protected:
	static void _bind_methods();

public:
	TopDownCharacter2D();
	void add_input(Vector2 p_input);
	void add_force(Vector2 p_force);
	void _ready() override;
	void _physics_process(float delta);

private:
	Vector2 input_dir;
	Vector2 external_force;
	float max_velocity;
	float acceleration;
	float friction;

public:
	// getters and setters
	void set_max_velocity(float p_velocity) {
		max_velocity = p_velocity;
	}
	float get_max_velocity() const {
		return max_velocity;
	}

	void set_friction(float p_friction) {
		friction = p_friction;
	}
	float get_friction() const {
		return friction;
	}

	void set_acceleration(float p_acceleration) {
		acceleration = p_acceleration;
	}

	float get_acceleration() const {
		return acceleration;
	}


};

} //namespace godot

#endif //TOP_DOWN_CHARACTER_2_D_H
