// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Source asset -- macOS emulator process launch implementation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "../source.emulator.native.h"
#include <filesystem>
#include <cstdio>
#include <vector>
#include <string>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace RetrodevLib {

	void* SourceEmulatorNative::Spawn(const std::string& exePath, const std::string& rawCmdLine, const std::string& workDir) {
		//
		// Split rawCmdLine into argv tokens for posix_spawn.
		// Honour double-quoted spans so paths with spaces survive intact.
		//
		std::vector<std::string> tokens;
		size_t i = 0;
		while (i < rawCmdLine.size()) {
			while (i < rawCmdLine.size() && rawCmdLine[i] == ' ')
				i++;
			if (i >= rawCmdLine.size())
				break;
			std::string tok;
			if (rawCmdLine[i] == '"') {
				i++;
				while (i < rawCmdLine.size() && rawCmdLine[i] != '"')
					tok += rawCmdLine[i++];
				if (i < rawCmdLine.size())
					i++;
			} else {
				while (i < rawCmdLine.size() && rawCmdLine[i] != ' ')
					tok += rawCmdLine[i++];
			}
			if (!tok.empty())
				tokens.push_back(tok);
		}
		if (tokens.empty()) {
			Log::Error(LogChannel::Build, "[Emulator] Empty command line.");
			return nullptr;
		}
		std::vector<char*> argv;
		for (auto& t : tokens)
			argv.push_back(t.data());
		argv.push_back(nullptr);
		//
		// Set working directory via posix_spawn file actions
		//
		std::string absWorkDir = workDir.empty() ? std::filesystem::path(exePath).parent_path().string() : workDir;
		posix_spawn_file_actions_t fa;
		posix_spawn_file_actions_init(&fa);
		posix_spawn_file_actions_addchdir_np(&fa, absWorkDir.c_str());
		pid_t pid = -1;
		int rc = posix_spawn(&pid, argv[0], &fa, nullptr, argv.data(), environ);
		posix_spawn_file_actions_destroy(&fa);
		if (rc != 0) {
			char errMsg[512];
			std::snprintf(errMsg, sizeof(errMsg), "[Emulator] Failed to launch: %s: posix_spawn error %d", exePath.c_str(), rc);
			Log::Error(LogChannel::Build, "%s", errMsg);
			return nullptr;
		}
		return reinterpret_cast<void*>(static_cast<intptr_t>(pid));
	}

	bool SourceEmulatorNative::Poll(void* handle, int& outExitCode) {
		int pid = static_cast<int>(reinterpret_cast<intptr_t>(handle));
		int status = 0;
		if (waitpid(static_cast<pid_t>(pid), &status, WNOHANG) == 0)
			return false;
		outExitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
		return true;
	}

	void SourceEmulatorNative::Destroy(void* handle) {
		//
		// pid was stored directly in the pointer value -- nothing to free
		//
		(void)handle;
	}

}
