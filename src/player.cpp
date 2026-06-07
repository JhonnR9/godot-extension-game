#include "player.h"

#include "world.h"

#include "crosshair.h"
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace godot {

void Player::_ready() {
    if (!Engine::get_singleton()->is_editor_hint()) {
        Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
    }

	if ((_head = get_node<Node3D>("Head"))) {
		if((_camera = get_node<Camera3D>("Head/Camera3D"))) {
			_camera->set_current(true);
		}
	}


	if ((_world = get_node<World>("../World"))) {
		_world->set_focus_node(this);
	}
}

void Player::_physics_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    Input *input = Input::get_singleton();
    Vector3 current_velocity = get_velocity();

    if (_double_tap_timer > 0) {
        _double_tap_timer -= delta;
    }

    if (input->is_action_just_pressed("jump")) {
        if (_double_tap_timer > 0) {
            _current_mode = (_current_mode == WALK) ? FLY : WALK;
            _double_tap_timer = 0;
        } else {
            _double_tap_timer = DOUBLE_TAP_TIME;
        }
    }

    if (input->is_action_just_pressed("toggle_mode")) {
        _current_mode = (_current_mode == NOCLIP) ? WALK : NOCLIP;
    }

    get_node<CollisionShape3D>("CollisionShape3D")
        ->set_disabled(_current_mode == NOCLIP);

    Vector3 direction;

    Basis basis = get_transform().basis;

    Vector3 forward = -basis.get_column(2);
    Vector3 right = basis.get_column(0);

    forward.y = 0;
    right.y = 0;

    forward = forward.normalized();
    right = right.normalized();

    if (input->is_action_pressed("move_forward")) {
        direction += forward;
    }

    if (input->is_action_pressed("move_backward")) {
        direction -= forward;
    }

    if (input->is_action_pressed("move_left")) {
        direction -= right;
    }

    if (input->is_action_pressed("move_right")) {
        direction += right;
    }

    if (_current_mode == WALK) {

        if (!is_on_floor()) {
            float gravity =
                ProjectSettings::get_singleton()
                    ->get_setting("physics/3d/default_gravity");

            current_velocity.y -= gravity * delta;
        }

        if (input->is_action_just_pressed("jump") && is_on_floor()) {
            current_velocity.y = _jump_force;
        }

        if (direction.length() > 0) {
            direction = direction.normalized();

            current_velocity.x = direction.x * _speed;
            current_velocity.z = direction.z * _speed;
        } else {
            current_velocity.x =
                Math::lerp(current_velocity.x, 0.0f, 0.15f);

            current_velocity.z =
                Math::lerp(current_velocity.z, 0.0f, 0.15f);
        }

    } else {

        if (input->is_action_pressed("move_up")) {
            direction.y += 1;
        }

        if (input->is_action_pressed("move_down")) {
            direction.y -= 1;
        }

        float speed_to_use =
            (_current_mode == FLY)
                ? _fly_speed
                : _noclip_speed;

        if (direction.length() > 0) {
            current_velocity =
                direction.normalized() * speed_to_use;
        } else {
            current_velocity =
                current_velocity.lerp(Vector3(0, 0, 0), 0.1f);
        }
    }

	set_velocity(current_velocity);

	move_and_slide();

	if (is_on_ceiling()) {
		current_velocity.y = 0;
	}

	apply_floor_snap();
}

void Player::_unhandled_input(const Ref<InputEvent> &event) {

    Ref<InputEventMouseMotion> motion = event;

    if (motion.is_valid()) {

        Vector2 relative = motion->get_relative();

        _yaw -= relative.x * _mouse_sensitivity;
        _pitch -= relative.y * _mouse_sensitivity;

        constexpr float HALF_PI =
            static_cast<float>(Math_PI) * 0.5f;

        _pitch = Math::clamp(_pitch, -HALF_PI, HALF_PI);

        set_rotation(Vector3(0, _yaw, 0));

    	_head->set_rotation(Vector3(_pitch, 0, 0));
    }

    Ref<InputEventKey> key_event = event;

    if (key_event.is_valid() &&
        key_event->is_pressed() &&
        !key_event->is_echo()) {

        Key keycode = key_event->get_keycode();

        if (keycode >= Key::KEY_0 &&
            keycode <= Key::KEY_9) {

            _selected_block_id =
                keycode - Key::KEY_0;
        }
    }

    Ref<InputEventMouseButton> mouse_button = event;

    if (mouse_button.is_valid() &&
        mouse_button->is_pressed()) {

        float fov = _camera->get_fov();

        if (mouse_button->get_button_index() ==
            MouseButton::MOUSE_BUTTON_WHEEL_UP) {

            fov -= 2.0f;
        }

        if (mouse_button->get_button_index() ==
            MouseButton::MOUSE_BUTTON_WHEEL_DOWN) {

            fov += 2.0f;
        }

        fov = Math::clamp(fov, 20.0f, 100.0f);

        _camera->set_fov(fov);


        if (mouse_button->get_button_index() ==
            MouseButton::MOUSE_BUTTON_LEFT) {

            Dictionary hit = raycast_block(8.0f);

            if (!hit.is_empty()) {

                Vector3 position = hit["position"];
                Vector3 normal = hit["normal"];

                position -= normal * 0.01f;

                if (_world) {
                    _world->break_block(position);
                }
            }
        }

        if (mouse_button->get_button_index() ==
            MouseButton::MOUSE_BUTTON_RIGHT) {

            Dictionary hit = raycast_block(8.0f);

            if (!hit.is_empty()) {

                Vector3 position = hit["position"];
                Vector3 normal = hit["normal"];

                position += normal * 0.01f;

                if (_world) {

                    auto selected_type =static_cast<voxel::BlockType>(_selected_block_id);

                    voxel::Block block_to_place =voxel::make_block(selected_type);

                    _world->set_block(position,block_to_place);
                }
            }
        }
    }
}

Dictionary Player::raycast_block(float distance) {

    Vector3 from =
        _camera->get_global_position();

    Vector3 to =
        from +
        (-_camera->get_global_transform()
              .basis
              .get_column(2)) *
            distance;

    Ref<PhysicsRayQueryParameters3D> query =
        PhysicsRayQueryParameters3D::create(
            from,
            to
        );

    query->set_exclude(Array::make(get_rid()));

    Ref<World3D> world = get_world_3d();

    return world->get_direct_space_state()
        ->intersect_ray(query);
}

void Player::_bind_methods() {
}

} // namespace godot