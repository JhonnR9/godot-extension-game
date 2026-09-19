#ifndef TREE_DECORATOR_H
#define TREE_DECORATOR_H

#include "chunk_model.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <cstdint>
#include <functional>
#include <memory>

namespace godot {

struct TreeSettings {
	int64_t seed = 0;
	int max_trees_per_chunk = 4; // por coluna de chunk (32x32)
	int min_trunk_height = 4;
	int max_trunk_height = 6;

	// Altura (Y em blocos, mundo) do ultimo bloco solido da coluna (wx, wz).
	// DEVE ser a mesma formula usada pelo gerador de terreno.
	std::function<int32_t(int32_t wx, int32_t wz)> surface_height;
};

// Servico 100% deterministico e sem estado:
// as arvores de uma coluna de chunk (cx, cz) dependem so de (seed, cx, cz) + altura do terreno.
// Ao decorar o chunk C, calculamos as arvores das 9 colunas vizinhas (3x3) e
// escrevemos apenas os blocos que caem dentro de C. Assim arvores na borda
// ficam completas e reaparecem identicas se o chunk for descarregado/recarregado.
class TreeDecorator : public RefCounted {
	GDCLASS(TreeDecorator, RefCounted)

protected:
	static void _bind_methods() {}

public:
	void set_settings(const TreeSettings &p_settings) { _settings = p_settings; }
	void set_seed(int64_t p_seed) { _settings.seed = p_seed; }

	// Thread-safe (nao altera estado interno). Chamar ANTES de o chunk entrar no repositorio.
	void decorate_chunk(const Vector3i &p_chunk_pos, const std::shared_ptr<Chunk> &p_chunk) const;

private:
	TreeSettings _settings;
};

} // namespace godot

#endif // TREE_DECORATOR_H