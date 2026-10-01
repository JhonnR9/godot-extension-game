# World generation pipeline

Chunk generation now runs as an ordered list of immutable `ChunkGenerationPass` objects. `VoxelAPI::_setup_generation_pipeline()` assembles the default list, and `ChunkModelGenerator` captures a shared snapshot of that list in each worker job.

The default order is:

1. `BiomeSelectionPass` chooses per-column biome settings.
2. `TerrainSurfacePass` selects the surface height and fills the terrain palette.
3. `CaveCarvingPass` removes blocks from the generated terrain.
4. `WaterFillPass` fills exposed columns up to their water level.
5. `TreeGenerationPass` places deterministic trees and their leaves, skipping disallowed biomes and planting columns whose base is at or below that column's water level.

Each pass receives a `ChunkGenerationContext` with the chunk coordinates, world seed, noise resources, writable chunk, and per-column `ColumnGenerationData`. Passes can be inserted before or after existing stages with `ChunkGenerationPipeline::add_pass()`.

## Desert biome

`BiomeSelectionPass` currently selects desert columns when the biome noise reaches `0.28`. Deserts use sand for the surface and shallow subsurface, sandstone below, less height variation with a separate ridged dune signal, a lower water level, and no trees. The sand and sandstone textures are included in the block texture array.

## Adding another biome

Implement a `ChunkGenerationPass` that runs before `TerrainSurfacePass`. For each `(x, z)` column, select a biome and write its `biome_id`, `height_scale`, `height_offset`, `water_level`, and block palette (`surface_block`, `subsurface_block`, `stone_block`, and `deep_stone_block`) into `context.column(x, z)`. The terrain, cave, and water stages then consume those values without needing biome-specific branches.

Additional passes can add ores, structures, vegetation, or post-processing rules. Keep pass instances immutable after assembling the pipeline: generation runs on worker threads, and the same pass objects can be called concurrently for different chunks. Store per-chunk scratch data in `ChunkGenerationContext`, not in a pass member.

When changing pass order, preserve the dependencies: terrain must precede carving and fluids; features that need a surface should run after terrain; structures that should be carved through caves need to run before `CaveCarvingPass`.
