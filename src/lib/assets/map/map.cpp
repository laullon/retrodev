// ---------------------------------------------------------------------------
//
// Retrodev Lib
//
// Map asset utilities -- index translation between compact and absolute tile indices.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------

#include "map.h"
#include <algorithm>

namespace RetrodevLib {

    int MapExtractedToAbs(int extracted, const std::vector<int>& deletedSorted) {
        int abs = 0;
        int remaining = extracted;
        while (true) {
            bool isDeleted = std::binary_search(deletedSorted.begin(), deletedSorted.end(), abs);
            if (!isDeleted) {
                if (remaining == 0)
                    return abs;
                remaining--;
            }
            abs++;
        }
    }

    int MapAbsToExtracted(int abs, const std::vector<int>& deletedSorted) {
        if (std::binary_search(deletedSorted.begin(), deletedSorted.end(), abs))
            return -1;
        int extracted = 0;
        for (int i = 0; i < abs; i++) {
            if (!std::binary_search(deletedSorted.begin(), deletedSorted.end(), i))
                extracted++;
        }
        return extracted;
    }

    std::vector<int> MapGetSortedDeletedTiles(const TilesetSlot& slot, const std::vector<ProjectBuildTilesEntry>& buildTiles) {
        if (slot.variants.empty())
            return {};
        int vi = std::max(0, std::min(slot.activeVariant, (int)slot.variants.size() - 1));
        const std::string& variantName = slot.variants[vi];
        const TileExtractionParams* tileParams = nullptr;
        for (const auto& entry : buildTiles) {
            if (entry.name == variantName) {
                tileParams = &entry.tileParams;
                break;
            }
        }
        if (!tileParams || tileParams->DeletedTiles.empty())
            return {};
        std::vector<int> sorted = tileParams->DeletedTiles;
        std::sort(sorted.begin(), sorted.end());
        return sorted;
    }

    uint16_t MapTranslateCell(uint16_t cell, const std::vector<int>& deletedSorted, bool toAbsolute) {
        if (cell == 0)
            return 0;
        int slotBits = (int)(cell >> 12);
        int tileIdx = (int)(cell & 0x0FFF);
        int newIdx = toAbsolute ? MapExtractedToAbs(tileIdx, deletedSorted) : MapAbsToExtracted(tileIdx, deletedSorted);
        if (newIdx < 0)
            return 0;
        return (uint16_t)((slotBits << 12) | (newIdx & 0x0FFF));
    }

    void MapFixupIndices(std::vector<ProjectBuildMapEntry>& buildMaps, const std::vector<ProjectBuildTilesEntry>& buildTiles, bool toAbsolute) {
        for (auto& mapEntry : buildMaps) {
            MapParams& mp = mapEntry.mapParams;
            //
            // Build a sorted DeletedTiles vector per slot index (slot 0..N-1)
            //
            std::vector<std::vector<int>> slotDeleted;
            slotDeleted.reserve(mp.tilesets.size());
            for (const auto& slot : mp.tilesets)
                slotDeleted.push_back(MapGetSortedDeletedTiles(slot, buildTiles));
            //
            // Translate all layer cell values
            //
            for (auto& layer : mp.layers) {
                for (auto& cell : layer.data) {
                    if (cell == 0)
                        continue;
                    int slotIdx = (int)(cell >> 12) - 1;
                    if (slotIdx < 0 || slotIdx >= (int)slotDeleted.size())
                        continue;
                    const std::vector<int>& deleted = slotDeleted[slotIdx];
                    if (deleted.empty())
                        continue;
                    cell = MapTranslateCell(cell, deleted, toAbsolute);
                }
            }
            //
            // Translate all group tile values (same cell encoding)
            //
            for (auto& group : mp.groups) {
                for (auto& cell : group.tiles) {
                    if (cell == 0)
                        continue;
                    int slotIdx = (int)(cell >> 12) - 1;
                    if (slotIdx < 0 || slotIdx >= (int)slotDeleted.size())
                        continue;
                    const std::vector<int>& deleted = slotDeleted[slotIdx];
                    if (deleted.empty())
                        continue;
                    cell = MapTranslateCell(cell, deleted, toAbsolute);
                }
            }
        }
    }

}
