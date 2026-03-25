// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Project metadata -- map item serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <project/project.h>
#include <glaze/glaze.hpp>
#include <glaze/json/custom.hpp>
#include <string>
#include <vector>

namespace RetrodevLib::MapSerial {
	//
	// Encode a uint16_t vector as a contiguous hex string for compact JSON storage.
	// Each value becomes 4 uppercase hex digits; the whole array is one JSON string.
	//
	inline std::string EncodeHex(const std::vector<uint16_t>& v) {
		static constexpr char kHex[] = "0123456789ABCDEF";
		std::string result;
		result.reserve(v.size() * 4);
		for (uint16_t c : v) {
			result += kHex[(c >> 12) & 0xF];
			result += kHex[(c >> 8) & 0xF];
			result += kHex[(c >> 4) & 0xF];
			result += kHex[c & 0xF];
		}
		return result;
	}
	//
	// Decode a hex string produced by EncodeHex back into a uint16_t vector.
	//
	inline void DecodeHex(const std::string& hex, std::vector<uint16_t>& v) {
		v.clear();
		v.reserve(hex.size() / 4);
		for (std::size_t i = 0; i + 3 < hex.size(); i += 4) {
			auto nibble = [](char c) -> uint16_t {
				if (c >= '0' && c <= '9')
					return static_cast<uint16_t>(c - '0');
				if (c >= 'A' && c <= 'F')
					return static_cast<uint16_t>(c - 'A' + 10);
				if (c >= 'a' && c <= 'f')
					return static_cast<uint16_t>(c - 'a' + 10);
				return 0;
			};
			uint16_t val = static_cast<uint16_t>((nibble(hex[i]) << 12) | (nibble(hex[i + 1]) << 8) | (nibble(hex[i + 2]) << 4) | nibble(hex[i + 3]));
			v.push_back(val);
		}
	}
}
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::TilesetSlot> {
	using T = RetrodevLib::TilesetSlot;
	static constexpr auto value = glz::object("variants", &T::variants, "activeVariant", &T::activeVariant);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::TileGroup> {
	using T = RetrodevLib::TileGroup;
	static constexpr auto readTiles = [](T& s, const std::string& hex) { RetrodevLib::MapSerial::DecodeHex(hex, s.tiles); };
	static constexpr auto writeTiles = [](T& s) { return RetrodevLib::MapSerial::EncodeHex(s.tiles); };
	static constexpr auto value = glz::object("name", &T::name, "width", &T::width, "height", &T::height, "tiles", glz::custom<readTiles, writeTiles>);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::MapLayer> {
	using T = RetrodevLib::MapLayer;
	static constexpr auto readData = [](T& s, const std::string& hex) { RetrodevLib::MapSerial::DecodeHex(hex, s.data); };
	static constexpr auto writeData = [](T& s) { return RetrodevLib::MapSerial::EncodeHex(s.data); };
	static constexpr auto value = glz::object("name", &T::name, "width", &T::width, "height", &T::height, "mapSpeed", &T::mapSpeed, "offsetX", &T::offsetX, "offsetY", &T::offsetY,
											  "visible", &T::visible, "data", glz::custom<readData, writeData>);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::MapParams> {
	using T = RetrodevLib::MapParams;
	static constexpr auto value = glz::object("viewWidth", &T::viewWidth, "viewHeight", &T::viewHeight, "tilesets", &T::tilesets, "groups", &T::groups, "layers", &T::layers);
};
