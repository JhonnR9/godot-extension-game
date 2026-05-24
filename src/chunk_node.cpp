#include "chunk_node.h"
#include "world.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/texture2d_array.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/shader.hpp>

namespace godot {

void ChunkNode::_setup() {

    if (_shape.is_null()) {
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


    Ref<Shader> shader = ResourceLoader::get_singleton()->load(
        "res://shaders/chunk.gdshader"
    );

    if (!shader.is_valid()) {
        ERR_PRINT("Erro: shader principal não encontrado");
        return;
    }

    Ref<ShaderMaterial> mat;
    mat.instantiate();

    mat->set_shader(shader);


    Ref<Texture2DArray> tex_array = ResourceLoader::get_singleton()->load(
        "res://textures/block_array.tres"
    );

    if (tex_array.is_valid()) {
        mat->set_shader_parameter("albedo_array", tex_array);
    } else {
        ERR_PRINT("Erro: atlas array não encontrado em res://sprites/atlas_array.tres");
    }


   /* Ref<Shader> outline_shader = ResourceLoader::get_singleton()->load(
        "res://shaders/outline.gdshader"
    );

    if (outline_shader.is_valid()) {

        Ref<ShaderMaterial> outline_mat;
        outline_mat.instantiate();

        outline_mat->set_shader(outline_shader);

        outline_mat->set_shader_parameter(
            "outline_size",
            0.01f
        );

        outline_mat->set_shader_parameter(
            "outline_color",
            Vector3(1.0, 1.0, 1.0)
        );

      //  mat->set_next_pass(outline_mat);
    }
    else {
        ERR_PRINT("Erro: outline shader não encontrado");
    }*/


    set_material_override(mat);
}

void ChunkNode::set_collision_faces(
    const PackedVector3Array &collision_faces
) {
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

} // namespace godot