// --------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include <windows.h>
#include <vector>
#include <string>
#include <retro.main.h>

//
// On debug we use console applicatoin
//
#ifdef RELEASE

// Gets a vector of strings from the command line
// To be used in the main function. We use the unicode version of the command line
// and parameters with non latin characters to be converted into utf8
// std::string is not utf8 but it will fail only with lenghts.
//
std::vector<std::string> ConvertCommandLineToArgs(LPCSTR lpCmdLine) {
	std::vector<std::string> args;
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv != nullptr) {
		for (int i = 0; i < argc; i++) {
			int argLen = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
			std::string arg(argLen, '\0');
			WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, &arg[0], argLen, nullptr, nullptr);
			args.push_back(arg);
		}
		LocalFree(argv);
	}
	return args;
}

//
// Windows entry point
//
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	// Convert lpCmdLine to argc/argv
	std::vector<std::string> args = ConvertCommandLineToArgs(lpCmdLine);
	int argc = static_cast<int>(args.size());
	std::vector<char*> argv;
	for (const auto& arg : args) {
		argv.push_back(const_cast<char*>(arg.c_str()));
	}
	// Call the regular main function
	return retromain(argc, argv.data());
}
#else

//
// Entry point (Console)
//
int main(int argc, char** argv) {
	return retromain(argc, argv);
}

#endif