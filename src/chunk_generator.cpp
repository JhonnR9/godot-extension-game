#include "chunk_generator.h"
#include "cave_tunnels.h"
#include "surface_vegetation.h"
#include "terrain_sampler.h"
#include "underground_deposits.h"
#include <array>

#include <godot_cpp/core/math.hpp>

namespace godot {

void BiomeSelectionPass::apply(ChunkGenerationContext &context) const {
    for(int z=0;z<Chunk::SIZE_Z;++z) for(int x=0;x<Chunk::SIZE_X;++x)
        context.column(x,z)=TerrainSampler::sample(context.settings,context.world_x(x),context.world_z(z));
    context.columns_ready=true;
}

void VegetationGenerationPass::apply(ChunkGenerationContext &context) const {
    for(int z=0;z<Chunk::SIZE_Z;++z) for(int x=0;x<Chunk::SIZE_X;++x) {
        const auto &c=context.column(x,z);
        if(!c.definition || c.surface_height+1<=c.water_level) continue;
        const auto &p=c.definition->vegetation;
        if(p.coverage_max==0 || c.climate_weight<p.climate_min || (c.climate_weight>=p.climate_max && p.climate_max<1)) continue;
        const auto plant=voxel::sample_surface_plant(p,_seed,context.world_x(x),context.world_z(z));
        if(plant.block==voxel::block_ids::air) continue;
        const int ground=c.surface_height-context.world_y(0);
        if(ground>=0 && ground<Chunk::SIZE_Y && context.chunk.get_block(x,ground,z)!=c.surface_block) continue;
        for(int dy=1;dy<=plant.height;++dy) {
            const int y=ground+dy;
            if(y>=0 && y<Chunk::SIZE_Y && voxel::is_air(context.chunk.get_block(x,y,z)))
                context.write_block(x,y,z,voxel::make_block(plant.block),GenerationLayer::VEGETATION);
        }
    }
}

void TerrainSurfacePass::apply(ChunkGenerationContext &context) const {
    for(int z=0;z<Chunk::SIZE_Z;++z) for(int x=0;x<Chunk::SIZE_X;++x) for(int y=0;y<Chunk::SIZE_Y;++y) {
        const auto block=TerrainSampler::block_at(context.column(x,z),context.world_y(y));
        if(!voxel::is_air(block)) context.write_block(x,y,z,block,GenerationLayer::TERRAIN);
    }
}

void CaveCarvingPass::apply(ChunkGenerationContext &context) const {
    const voxel::CavePoint origin{static_cast<double>(context.world_x(0)),
            static_cast<double>(context.world_y(0)), static_cast<double>(context.world_z(0))};
    const auto tunnels = voxel::CaveTunnels::for_bounds(context.settings.world_seed, origin,
            {origin.x + Chunk::SIZE_X, origin.y + Chunk::SIZE_Y, origin.z + Chunk::SIZE_Z});
    std::array<bool, Chunk::VOLUME> carved{};
    for (const auto &tunnel : tunnels) {
        const double r = tunnel.radius;
        const int min_x = std::max(0, static_cast<int>(std::floor(std::min(tunnel.a.x, tunnel.b.x)-r-origin.x)));
        const int max_x = std::min(Chunk::MAX_X, static_cast<int>(std::ceil(std::max(tunnel.a.x, tunnel.b.x)+r-origin.x)));
        const int min_y = std::max(0, static_cast<int>(std::floor(std::min(tunnel.a.y, tunnel.b.y)-r-origin.y)));
        const int max_y = std::min(Chunk::MAX_Y, static_cast<int>(std::ceil(std::max(tunnel.a.y, tunnel.b.y)+r-origin.y)));
        const int min_z = std::max(0, static_cast<int>(std::floor(std::min(tunnel.a.z, tunnel.b.z)-r-origin.z)));
        const int max_z = std::min(Chunk::MAX_Z, static_cast<int>(std::ceil(std::max(tunnel.a.z, tunnel.b.z)+r-origin.z)));
        for (int z=min_z; z<=max_z; ++z) {
            for (int x=min_x; x<=max_x; ++x) {
                const int surface = context.column(x,z).surface_height;
                for (int y=min_y; y<=max_y; ++y) {
                    const int wy=context.world_y(y);
                    // Keep the terrain roof and bedrock intact; do not carve water.
                    if (wy <= WORLD_BEDROCK_Y || surface-wy < 7) continue;
                    const int index=x+Chunk::SIZE_X*(y+Chunk::SIZE_Y*z);
                    if (carved[index]) continue;
                    const auto block=context.chunk.get_block(x,y,z);
                    if (!voxel::is_collidable(block)) continue;
                    if (voxel::CaveTunnels::distance_squared(tunnel,
                            {origin.x+x+0.5, origin.y+y+0.5, origin.z+z+0.5}) <= r*r) {
                        context.write_block(x,y,z,0,GenerationLayer::CARVING);
                        carved[index]=true;
                    }
                }
            }
        }
    }
}

void OreGenerationPass::apply(ChunkGenerationContext &context) const {
    const int ox=context.world_x(0), oy=context.world_y(0), oz=context.world_z(0);
    const auto deposits=voxel::underground_deposits(context.settings.world_seed,
            ox,oy,oz,ox+Chunk::SIZE_X,oy+Chunk::SIZE_Y,oz+Chunk::SIZE_Z);
    for (const auto &deposit : deposits) {
        const uint16_t type=deposit.kind==voxel::DepositKind::IRON ? voxel::block_ids::iron_ore :
                deposit.kind==voxel::DepositKind::COAL ? voxel::block_ids::coal_ore :
                deposit.kind==voxel::DepositKind::DIAMOND ? voxel::block_ids::diamond_ore : voxel::block_ids::dirt;
        const int min_x=std::max(0,static_cast<int>(std::floor(deposit.x-deposit.rx-ox)));
        const int max_x=std::min(Chunk::MAX_X,static_cast<int>(std::ceil(deposit.x+deposit.rx-ox)));
        const int min_y=std::max(0,static_cast<int>(std::floor(deposit.y-deposit.ry-oy)));
        const int max_y=std::min(Chunk::MAX_Y,static_cast<int>(std::ceil(deposit.y+deposit.ry-oy)));
        const int min_z=std::max(0,static_cast<int>(std::floor(deposit.z-deposit.rz-oz)));
        const int max_z=std::min(Chunk::MAX_Z,static_cast<int>(std::ceil(deposit.z+deposit.rz-oz)));
        for (int z=min_z;z<=max_z;++z) for (int x=min_x;x<=max_x;++x) {
            for (int y=min_y;y<=max_y;++y) {
                const int wy=oy+y;
                if (wy<=WORLD_BEDROCK_Y || context.column(x,z).surface_height-wy < 12) continue;
                const uint16_t host=voxel::type(context.chunk.get_block(x,y,z));
                // Replace only rock: never fill the tunnel, cover bedrock,
                // replace existing ore, or change a surface/vegetation block.
                if (host!=voxel::block_ids::stone && host!=voxel::block_ids::deepslate) continue;
                if (deposit.contains(ox+x+0.5,wy+0.5,oz+z+0.5))
                    context.write_block(x,y,z,voxel::make_block(type),GenerationLayer::ORE);
            }
        }
    }
}

void WaterFillPass::apply(ChunkGenerationContext &context) const {
	for (int z = 0; z < Chunk::SIZE_Z; ++z) {
		for (int x = 0; x < Chunk::SIZE_X; ++x) {
			const ColumnGenerationData &column = context.column(x, z);
			// Water bodies belong to lowland/plains terrain only. Leave the
			// desert dry and ignore shallow one-block dips in the ground.
			if (!column.surface_water ||
					column.surface_height >= column.water_level - 1) {
				continue;
			}
			for (int y = 0; y < Chunk::SIZE_Y; ++y) {
				const int32_t wy = context.world_y(y);
				// Water occupies blocks below sea level so its top mesh lands
				// exactly on the sea-level plane instead of one block above it.
				if (wy > column.surface_height && wy < column.water_level &&
						voxel::is_air(context.chunk.get_block(x, y, z))) {
					const voxel::Block water_flags = voxel::BLOCK_FLAG_TRANSPARENT |
							(column.definition && column.definition->kind == BiomeKind::OCEAN ? voxel::BLOCK_FLAG_OCEAN : 0);
					context.write_block(x, y, z,
							voxel::make_block(voxel::block_ids::water, water_flags), GenerationLayer::WATER);
				}
			}
		}
	}
}

Chunk ChunkGenerator::generate(const Vector3i &chunk_pos, const TerrainSettings &settings,
		const ChunkGenerationPipeline &pipeline) {
	return pipeline.generate(chunk_pos, settings);
}

} // namespace godot
