// --------------------------------------------------------------------------------------------------------------
//
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <project/project.h>
#include <assets/palette/palette.params.h>
#include <glaze/glaze.hpp>
#include <string>
#include <vector>

//
// Glaze serialisation for PaletteParticipantRole
//
template <> struct glz::meta<RetrodevLib::PaletteParticipantRole> {
	static constexpr std::string_view name = "PaletteParticipantRole";
	using enum RetrodevLib::PaletteParticipantRole;
	static constexpr auto value = glz::enumerate("Always", Always, "Level", Level, "ScreenZone", ScreenZone);
};
//
// Glaze serialisation for PaletteParticipant
//
template <> struct glz::meta<RetrodevLib::PaletteParticipant> {
	using T = RetrodevLib::PaletteParticipant;
	static constexpr auto value = glz::object("buildItemName", &T::buildItemName, "buildItemType", &T::buildItemType, "role", &T::role, "tag", &T::tag);
};
//
// Glaze serialisation for PaletteZone
//
template <> struct glz::meta<RetrodevLib::PaletteZone> {
	using T = RetrodevLib::PaletteZone;
	static constexpr auto value = glz::object("name", &T::name, "lineStart", &T::lineStart, "lineEnd", &T::lineEnd, "targetMode", &T::targetMode, "participants", &T::participants);
};
//
// Glaze serialisation for PaletteParams
//
template <> struct glz::meta<RetrodevLib::PaletteParams> {
	using T = RetrodevLib::PaletteParams;
	static constexpr auto value = glz::object("targetSystem", &T::targetSystem, "targetPaletteType", &T::targetPaletteType, "zones", &T::zones);
};
