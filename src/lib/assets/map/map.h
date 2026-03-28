// ---------------------------------------------------------------------------
//
// Retrodev Lib
//
// Map asset utilities -- index translation between compact and absolute tile indices.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------

#pragma once

#include <project/project.h>
#include <vector>
#include <cstdint>

namespace RetrodevLib {

    //
    // Convert a compact extracted tile index to an absolute grid index.
    // Absolute indices include deleted tile slots; extracted indices skip them.
    // deletedSorted must be sorted in ascending order.
    //
    int MapExtractedToAbs(int extracted, const std::vector<int>& deletedSorted);

    //
    // Convert an absolute grid index to a compact extracted tile index.
    // Returns -1 if abs itself is a deleted tile slot.
    // deletedSorted must be sorted in ascending order.
    //
    int MapAbsToExtracted(int abs, const std::vector<int>& deletedSorted);

    //
    // Build a sorted DeletedTiles list for a given tileset slot's active variant.
    // Looks up the variant name in the provided buildTiles collection.
    // Returns an empty vector if the slot or variant cannot be resolved.
    //
    std::vector<int> MapGetSortedDeletedTiles(const TilesetSlot& slot, const std::vector<ProjectBuildTilesEntry>& buildTiles);

    //
    // Translate a single cell value's tile index using a mapping function.
    // The slot index bits are preserved; only the tile index bits change.
    // Returns 0 (empty) if the mapping function returns -1 (deleted tile).
    //
    uint16_t MapTranslateCell(uint16_t cell, const std::vector<int>& deletedSorted, bool toAbsolute);

    //
    // Apply an index translation pass to all map data in the project.
    // toAbsolute=true: compact->absolute (called after load).
    // toAbsolute=false: absolute->compact (called before save).
    //
    void MapFixupIndices(std::vector<ProjectBuildMapEntry>& buildMaps, const std::vector<ProjectBuildTilesEntry>& buildTiles, bool toAbsolute);

}
