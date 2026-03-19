// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "../source.emulator.native.h"
#include <filesystem>
#include <cstdio>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace RetrodevLib {

	void* SourceEmulatorNative::Spawn(const std::string& exePath, const std::string& rawCmdLine, const std::string& workDir) {
		//
		// Convert UTF-8 strings to wide for the Win32 API
		//
		std::wstring wCmd(rawCmdLine.begin(), rawCmdLine.end());
		std::string absWorkDir = workDir.empty() ? std::filesystem::path(exePath).parent_path().string() : workDir;
		std::wstring wWorkDir(absWorkDir.begin(), absWorkDir.end());
		//
		// Launch with a clean STARTUPINFOW — no STARTF_USESTDHANDLES — so the
		// child process inherits a normal GUI stdio context, not our redirected pipes
		//
		STARTUPINFOW si = {};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi = {};
		if (!CreateProcessW(nullptr, wCmd.data(), nullptr, nullptr, FALSE, 0, nullptr, wWorkDir.c_str(), &si, &pi)) {
			char errMsg[512];
			std::snprintf(errMsg, sizeof(errMsg), "[Emulator] Failed to launch: %s: Win32 error %lu", exePath.c_str(), GetLastError());
			Log::Error(LogChannel::Build, "%s", errMsg);
			return nullptr;
		}
		//
		// Thread handle is not needed — close it immediately
		//
		CloseHandle(pi.hThread);
		//
		// HANDLE is a void* on Windows — return it directly as the opaque handle
		//
		return pi.hProcess;
	}

	bool SourceEmulatorNative::Poll(void* handle, int& outExitCode) {
		HANDLE hProcess = static_cast<HANDLE>(handle);
		if (WaitForSingleObject(hProcess, 0) != WAIT_OBJECT_0)
			return false;
		DWORD exitCode = 0;
		GetExitCodeProcess(hProcess, &exitCode);
		outExitCode = static_cast<int>(exitCode);
		return true;
	}

	void SourceEmulatorNative::Destroy(void* handle) {
		CloseHandle(static_cast<HANDLE>(handle));
	}

} // namespace RetrodevLib
