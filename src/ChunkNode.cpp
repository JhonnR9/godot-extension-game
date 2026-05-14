#include "ChunkNode.h"
#include "world.h"

namespace godot {

ChunkNode::~ChunkNode() {
	MeshInstance3D::~MeshInstance3D();
	_static_body = nullptr;
	_collision_shape = nullptr;
}

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
		_material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_PER_VERTEX);
	}
}
void ChunkNode::_setup_material() {
	if (!_material.is_valid()) {
		_material.instantiate();
		_material->set_texture_filter(BaseMaterial3D::TEXTURE_FILTER_NEAREST);
		_material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_PER_VERTEX);
	}
}
void ChunkNode::set_collision_faces(const PackedVector3Array &collision_faces) {
	_setup();
	_shape->set_faces(collision_faces);
}

void ChunkNode::_enter_tree() {
	MeshInstance3D::_enter_tree();
	_setup();
}
void ChunkNode::_bind_methods() {
}

} //namespace godot