// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <project/project.h>
#include <glaze/glaze.hpp>
#include <string>
#include <vector>
#include <map>

//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::SourceParams::EmulatorParams::CommonFields> {
	using T = RetrodevLib::SourceParams::EmulatorParams::CommonFields;
	static constexpr auto value =
		glz::object("mediaFile", &T::mediaFile, "snapshot", &T::snapshot, "symbolFile", &T::symbolFile, "command", &T::command, "sendCPM", &T::sendCPM, "machine", &T::machine);
};

template <> struct glz::meta<RetrodevLib::SourceParams::EmulatorParams::WinApeParams> {
	using T = RetrodevLib::SourceParams::EmulatorParams::WinApeParams;
	static constexpr auto value = glz::object();
};

template <> struct glz::meta<RetrodevLib::SourceParams::EmulatorParams::RvmParams> {
	using T = RetrodevLib::SourceParams::EmulatorParams::RvmParams;
	static constexpr auto value = glz::object();
};

template <> struct glz::meta<RetrodevLib::SourceParams::EmulatorParams::AceDlParams> {
	using T = RetrodevLib::SourceParams::EmulatorParams::AceDlParams;
	static constexpr auto value =
		glz::object("crtc", &T::crtc, "ram", &T::ram, "firmware", &T::firmware, "speed", &T::speed, "alone", &T::alone, "skipConfigFile", &T::skipConfigFile);
};

template <> struct glz::meta<RetrodevLib::SourceParams::EmulatorParams> {
	using T = RetrodevLib::SourceParams::EmulatorParams;
	static constexpr auto value = glz::object("emulator", &T::emulator, "common", &T::common, "acedl", &T::acedl);
};

template <> struct glz::meta<RetrodevLib::SourceParams> {
	using T = RetrodevLib::SourceParams;
	static constexpr auto value = glz::object("tool", &T::tool, "sources", &T::sources, "includeDirs", &T::includeDirs, "defines", &T::defines, "dependencies", &T::dependencies,
											  "toolOptions", &T::toolOptions, "emulatorParams", &T::emulatorParams);
};
