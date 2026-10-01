# World generation pipeline

Chunk generation now runs as an ordered list of immutable `ChunkGenerationPass` objects. `VoxelAPI::_setup_generation_pipeline()` assembles the default list, and `ChunkModelGenerator` captures a shared snapshot of that list in each worker job.

The default order is:

1. `BiomeSelectionPass` blends per-column terrain, water and biome palettes across a broad transition band.
2. `TerrainSurfacePass` selects the surface height and fills the terrain palette.
3. `CaveCarvingPass` removes blocks from the generated terrain.
4. `WaterFillPass` fills exposed columns up to their water level.
5. `VegetationGenerationPass` places deterministic cacti in the desert and flowers or tall grass in the plains.
6. `TreeGenerationPass` places deterministic trees and their leaves, skipping disallowed biomes and planting columns whose base is at or below that column's water level.

Each pass receives a `ChunkGenerationContext` with the chunk coordinates, world seed, noise resources, writable chunk, and per-column `ColumnGenerationData`. Passes can be inserted before or after existing stages with `ChunkGenerationPipeline::add_pass()`.

## Desert biome

`BiomeSelectionPass` uses a smooth biome-noise weight, and the low biome-noise frequency creates larger regions. The transition belt blends terrain height and water level continuously, with a scrubby dirt palette between grassland and full desert. Deserts use sand at the surface, sandstone below, dune relief, fewer trees, and cacti. The plains use stronger broad relief and deterministic flowers and tall grass. Plant blocks use cutout textures and do not collide.

## Adding another biome

Implement a `ChunkGenerationPass` that runs before `TerrainSurfacePass`. For each `(x, z)` column, select a biome and write its `biome_id`, `height_scale`, `height_offset`, `water_level`, and block palette (`surface_block`, `subsurface_block`, `stone_block`, and `deep_stone_block`) into `context.column(x, z)`. The terrain, cave, and water stages then consume those values without needing biome-specific branches.

Additional passes can add ores, structures, vegetation, or post-processing rules. Keep pass instances immutable after assembling the pipeline: generation runs on worker threads, and the same pass objects can be called concurrently for different chunks. Store per-chunk scratch data in `ChunkGenerationContext`, not in a pass member.

When changing pass order, preserve the dependencies: terrain must precede carving and fluids; features that need a surface should run after terrain; structures that should be carved through caves need to run before `CaveCarvingPass`.
