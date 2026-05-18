#include "ChunkNode.h"
#include "world.h"
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace godot {


void ChunkNode::_setup() {
	if ((_shape.is_null())) {
		_shape.instantiate();
	}
	if (!_collision_shape) {
		_collision_shape = memnew(CollisionShape3D);
		_collision_shape->set_name("CollisionShape3D");
	}
	if (!_static_body) {
		_static_body = memnew(StaticBody3D);
		_static_body->set_name("StaticBody3D");
		_static_body->add_child(_collision_shape);

		add_child(_static_body);
	}

	if (!_material.is_valid()) {
		_material.instantiate();

		_material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);
		_material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, false);

		Ref<Resource> tex_res = ResourceLoader::get_singleton()->load("res://sprites/atlas.png");
		if (tex_res.is_valid()) {
			_material->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, tex_res);
		} else {
			ERR_PRINT("Erro: Não foi possível carregar a textura res://sprites/atlas.png");
		}

		_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_PER_PIXEL);
	}
}

void ChunkNode::set_collision_faces(const PackedVector3Array &collision_faces) {
	_setup();
	_shape->set_faces(collision_faces);
	if (_collision_shape->get_shape() != _shape) {
		_collision_shape->set_shape(_shape);
	}
}
void ChunkNode::disable() {
	set_mesh(Ref<Mesh>());
	if (_shape.is_valid()) {
		_shape->set_faces(PackedVector3Array());
	}
	set_visible(false);
	set_process(false);
	set_global_position(Vector3());
}
void ChunkNode::enable() {
	set_visible(true);
	set_process(true);
}

void ChunkNode::_enter_tree() {
	MeshInstance3D::_enter_tree();
	_setup();
}
void ChunkNode::_bind_methods() {
}

} //namespace godot