# Block Registry

The editor dock named **Block Registry** edits the source file `res://data/block_registry.json`. Each record stores a stable numeric ID, code name, display name, category, texture keys for each face, flags, and tint.

Use **Add** to create a record, edit it in the form, then choose **Save and Generate**. Texture keys refer to files in `res://textures/blocks/` without their file extension. All referenced images must have the same dimensions.

Generation writes:

- `res://textures/block_array.tres` and `res://textures/block_mapping.json`
- `res://data/block_registry.generated.json` with IDs, flags, categories, tint, and texture layer IDs
- `res://generated/block_registry.generated.h` with the C++ IDs, default flags, and name lookup

In C++, use `voxel::block_ids::stone` or `voxel::block_id_from_name("stone")` with `voxel::make_block(...)`. The generated JSON is also the source for the chunk mesher's face textures and tints.

IDs are saved in world chunks, so keep an existing block's ID stable when changing its name or metadata. ID `0` and the name `air` are reserved.
