# World generation pipeline

Chunk generation now runs as an ordered list of immutable `ChunkGenerationPass` objects. `VoxelAPI::_setup_generation_pipeline()` assembles the default list, and `ChunkModelGenerator` captures a shared snapshot of that list in each worker job.

The default order is:

1. `BiomeSelectionPass` samples `TerrainSampler` once per column using the immutable biome registry.
2. `TerrainSurfacePass` fills the sampled palette and configured strata.
3. `CaveCarvingPass` carves scattered rounded tunnels.
4. `OreGenerationPass` places compact iron, coal and diamond deposits plus dirt pockets inside remaining stone and deepslate.
5. `WaterFillPass` fills exposed columns up to their water level.
6. `TreeGenerationPass` places deterministic trees with each biome's materials, height and crown profile.
7. `VegetationGenerationPass` places the biome's weighted plant/flower profile on dry surfaces, without replacing trees.

Each pass receives a `ChunkGenerationContext` with the chunk coordinates, world seed, noise resources, writable chunk, and per-column `ColumnGenerationData`. Passes can be inserted before or after existing stages with `ChunkGenerationPipeline::add_pass()`.

## Desert biome

`BiomeSelectionPass` uses a smooth biome-noise weight, and the low biome-noise frequency creates larger regions. The transition belt blends terrain height continuously around a fixed sea level, with a scrubby dirt palette between grassland and full desert. Deserts use sand at the surface, sandstone below, dune relief, fewer trees, and cacti. The plains use stronger broad relief and deterministic flowers and tall grass. Plant blocks use cutout textures and do not collide.

## Adding another biome

Edit `project/data/biome_registry.json`: add a unique ID/name, partition the land climate intervals, choose a relief anchor, and configure materials, soil depth, strata, trees and vegetation. The full schema and expansion instructions are in [biome_registry.md](biome_registry.md).

`TerrainSampler` computes the complete base column for generation, neighbour queries and initial spawn. This replaces duplicated height calculations that previously used different dune amplitudes, height scales and rounding outside a chunk. `VoxelAPI.sample_terrain_column()` exposes the same sampler for debugging. The registry is loaded/validated once on the main thread; generation workers share a const snapshot and perform no disk/configuration reads.

Additional passes can add ores, structures, vegetation, or post-processing rules. Keep pass instances immutable after assembling the pipeline: generation runs on worker threads, and the same pass objects can be called concurrently for different chunks. Store per-chunk scratch data in `ChunkGenerationContext`, not in a pass member.

When changing pass order, preserve the dependencies: terrain must precede carving and fluids; features that need a surface should run after terrain; structures that should be carved through caves need to run before `CaveCarvingPass`.

## Underground tunnels

`CaveCarvingPass` now carves rounded, continuous paths instead of intersecting
3D noise thresholds. A deterministic generator uses the world seed and regions
of 96 × 64 × 96 blocks. Most regions get one 144-block winding path; 40% of
those paths also get a 60-block branch. Some paths open into a wider chamber.
Other regions remain solid, keeping tunnel systems scattered rather than
turning the underground into a dense lattice.

Main tunnels have a radius of 3.6–5 blocks, with a vertical scale of 0.85
(roughly 7–10 blocks wide and 6–8 high). Branches use a radius of 3.4, and chambers
use a radius of 6.5–8. Consecutive path segments share endpoints; carving the
union of their rounded volumes avoids isolated blocks inside the passages.
The outer seven blocks below each terrain column and the bedrock remain intact.
This stage does not add surface entrances or change the surface biome relief.

Each chunk queries nearby regional paths, clips their bounding boxes, and only
checks voxel distances inside those boxes. Workers have no shared mutable cave
state. Identical world coordinates produce the same cavities regardless of
chunk generation order, including across negative coordinates and boundaries.
The old cave noise resources and thresholds are no longer used.

Terrain is regenerated with this algorithm when loaded, with saved edits
applied afterward. Existing edits can therefore affect the resulting passages;
use a new world to inspect the new generation without old mining edits.

Run algorithm checks with:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -pedantic tests/cave_tunnels_test.cpp -o /tmp/cave_tunnels_test
/tmp/cave_tunnels_test
```

The checks cover tunnel cross-sections, deterministic seeds, scattered cavity
volume, negative coordinates, and agreement between whole-region and chunk
queries at shared boundaries. Visual navigation and frame-time measurements
still need to be checked in the running game.

## Meadow vegetation

Seeded 12 × 12 patches vary coverage from 8.5% to 15.9% of eligible grass
columns. Most plants are low grass tufts, with fewer tall grasses and ferns.
Flowers are sparse (roughly 1.2–2%): the original flower, white daisies, blue
cornflowers and red poppies. Species preferences vary between patches.
Plants use crossed alpha-cutout quads, no collision and no additional nodes.
Only dry grass surfaces are eligible; desert cactus rules are unchanged.

## Underground deposits

`OreGenerationPass` now uses seeded ellipsoidal deposits in 16³ regions rather
than rare high noise thresholds. Candidate regions choose iron (35%), coal
(29%), deep diamond (4%), dirt (12%) or no deposit (20%). These are candidate
probabilities, not percentages of terrain: deposits have finite radii, can be
excluded by depth, and only replace remaining stone/deepslate at least 12
blocks below the surface. They never fill tunnels or replace bedrock, water,
plants or previous deposits. Diamond centers are below Y = -80; dirt centers
are mostly in the upper underground, Y = -96 through 48. Candidate ordering
and a seven-block halo keep generation consistent across chunk boundaries.
