/*---------------------------------------------------------------------------------------------------------



---------------------------------------------------------------------------------------------------------*/

#load "bld/ext/clang.csx"
#load "bld/ext/git.csx"
#load "bld/csx/build.flags.csx"

#r "../bld/bin/mkb.dll"
using Kltv.Kombine.Api;
using Kltv.Kombine.Types;
using static Kltv.Kombine.Api.Statics;
using static Kltv.Kombine.Api.Tool;

// Name of the library for the output
KValue libfriendlyname = "Angelscript (v2.38.0)";
// Path to store the lib sources
KValue libpath = "ascript/";
// Branch to use for the lib sources
KValue libbranch = "v2.38.0";
// Name of the library for the output
KValue libname = "ascript";
// Repository to use for the lib sources
KValue librepo = "https://github.com/anjo76/angelscript.git";

//
// Manage the library dependency
//
int dependencies(string[] args){
	if (Args.Get(0) == "clean") {
		Msg.Print($"Cleaning {libfriendlyname} sources");
		Folders.Delete(libpath,true);
		return 0;
	}
	if (Args.Get(0) == "update") {
		if (Folders.Exists(libpath)){
			Msg.Print($"Updating {libname} sources");
			Git.Pull(libpath);
			return 0;
		}
	}
	if ( (Args.Get(0) == "install") || (Args.Get(0) == "update")) {
		Msg.Print($"Cloning {libfriendlyname} sources");
		if (Folders.Exists(libpath)){
			Msg.BeginIndent();
			Msg.Print($"{libfriendlyname} folder already present. Skip");
			Msg.EndIndent();
			return 0;
		}
		Git.Clone(librepo,libpath,libbranch);
		return 0;
	}
	Msg.Print("No valid parameter action supplied.");
	return -1;
}


// Build the library Action
int build(string[] args){
	Msg.Print($"Building {libfriendlyname} library");
	Msg.BeginIndent();
	// Check if the sources folder is present
	if (!Folders.Exists(libpath)){
		Msg.PrintAndAbort($"{libfriendlyname} sources not found. Please run 'mkb dependencies install' to get the sources.");
	}
	// Output paths
	KValue OutputBin = KValue.Import("OutputBin");
	KValue OutputLib = KValue.Import("OutputLib");
	KValue OutputTmp = KValue.Import("OutputTmp");
	// Compilation flags
	KList Flags = new KList { "-Wno-language-extension-token", "-Wno-unused-parameter" };
	Flags += "-Wno-deprecated-declarations";
	// The list of defines to use
	// "ANGELSCRIPT_EXPORT" <-- To be declared for dll / so exportation
	KList Defines = new KList { "AS_MAX_PORTABILITY" };
	// ;AS_DEBUG;AS_MAX_PORTABILITY
	// Include directories
	KList Includes = new KList();
	Includes += libpath + "sdk/angelscript/include/";
	Includes += libpath + "sdk/angelscript/sources/";
	// Add the artifact name into the output folders
	OutputBin += libname + "/";
	OutputLib += libname + "/";
	// Create an instance of the clang tool.
	Clang clang = new Clang();
	// Create the list of sources to be compiled
	// We pass the relative path from the current folder
	KList src = CreateSourceList(libpath);
	clang.Options.SwitchesCC += Flags;
	clang.Options.SwitchesCC -= "-fno-exceptions";
	clang.Options.SwitchesCXX += Flags;
	clang.Options.SwitchesCXX -= "-fno-exceptions";
	clang.Options.Defines += Defines;
	clang.Options.IncludeDirs += Includes;
	// Generate the list of object files to be used as output
	KList objs = src.WithExtension(clang.Options.ObjectExtension).WithPrefix(OutputTmp);
	// And compile the sources
	clang.Compile(src, objs);
	// Use the librarian to generate a static library
	clang.Librarian(objs, OutputLib + libname + clang.Options.LibExtension);
	// ------------------------------------------------------------------------
	Msg.PrintTask("Building static library: " + libname + clang.Options.LibExtension);
	Msg.PrintTaskSuccess(" done");
	Msg.Print("---------------------------------------------------------------------------------------");
	Msg.EndIndent();
	// Libraries are registered in the stack when built
	register(args);
	return 0;
}

//
// Register the library in the stack to be used
//
int register(string[] args) {
	Msg.Print($"Registering {libfriendlyname} library");
	Msg.BeginIndent();
	KValue OutputLib = KValue.Import("OutputLib");
	OutputLib += libname + "/";
	// Create an instance of the clang tool.
	Clang clang = new Clang();
	// Register the output to make it available for everyone
	Msg.Print($"Registering {libfriendlyname} library under the name: "+libname);
	Msg.Print("  libname: " + libname + clang.Options.LibExtension);
	Share.Register(libname,"libname",libname + clang.Options.LibExtension);
	Msg.Print("  libpath: " + RealPath(OutputLib));
	Share.Register(libname,"libpath",RealPath(OutputLib));
	Msg.Print("  incpath: " + RealPath(libpath+"sdk/angelscript/include/"));
	Share.Register(libname,"incpath",RealPath(libpath+ "sdk/angelscript/include/"));
	Msg.Print("  addonpath: " + RealPath(libpath + "sdk/add_on/"));
	Share.Register(libname, "addonpath", RealPath(libpath + "sdk/add_on/"));
	Msg.PrintTask("Registered static library: " + libname);
	Msg.PrintTaskSuccess(" done");
	Msg.Print("---------------------------------------------------------------------------------------");
	Msg.EndIndent();
	return 0;
}
//
// Clean artifacts
//
int clean(string[] args){
	// Import Output paths
	Msg.Print("Cleaning: "+libname);
	KValue OutputBin = KValue.Import("OutputBin");
	KValue OutputLib = KValue.Import("OutputLib");
	KValue OutputTmp = KValue.Import("OutputTmp");
	KList folders = new KList();
	folders += OutputBin+libname+"/";
	folders += OutputLib+libname+"/";
	folders += OutputTmp+libname+"/";
	Msg.BeginIndent();
	foreach(KValue folder in folders){
		Msg.Print("Deleting: "+RealPath(folder));
	}
	Msg.EndIndent();
	// Clean the folders
	Folders.Delete(folders,true);
	return 0;
}


// Create the source code list to be compiled
private KList CreateSourceList(KValue libpath){
	KList src = new KList();
	// common source code
	src = Glob("ascript/sdk/angelscript/source/**/*.cpp");

	if (Host.IsWindows()){
		// Not required
	}
	if (Host.IsMacOS()){
		// Not yet.
	}
	if (Host.IsLinux()){
		// Not yet.

	}
	return src;
}