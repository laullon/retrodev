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
KValue libfriendlyname = "RetroDev (Lib)";
// Path to find the sources
KValue libpath = "lib/";
// Name of the library for the output
KValue libname = "retrodev";

//
// No dependencies 
//
int dependencies(string[] args) {
	return 0;
}

//
// Build the library Action
//
int build(string[] args) {
	Msg.Print($"Building {libfriendlyname} library");
	Msg.BeginIndent();

	// Generate resources
	// ------------------------------------------------------------------------
	Msg.Print("Generating resources");

	Msg.PrintTask("Generating resources: "); Msg.PrintTaskSuccess(" done");
	
	// Build the library
	// ------------------------------------------------------------------------
	// Output paths
	KValue OutputBin = KValue.Import("OutputBin");
	KValue OutputLib = KValue.Import("OutputLib");
	KValue OutputTmp = KValue.Import("OutputTmp");
	// Add the artifact name into the output folders
	OutputBin += libname + "/";
	OutputLib += libname + "/";
	OutputTmp += libname + "/";
	// The list of defines to use
	KList Defines = new KList();
	// Include directories
	KList Includes = new KList();
	// RetroDev library includes
	Includes += RealPath(libpath);
	// Third party dependencies includes
	Includes += Share.Registry("sdl", "incpath");
	Includes += Share.Registry("sdl.img", "incpath");
	Includes += Share.Registry("freetype", "incpath");
	Includes += Share.Registry("glaze", "incpath");
	Includes += Share.Registry("ascript", "incpath");
	Includes += Share.Registry("rasm", "incpath");
	Includes += Share.Registry("rasm", "extpath");
	// Library directories
	KList LibraryDirs = new KList();
	// Libraries
	KList Libraries = new KList();
	// Create an instance of the clang tool.
	Clang clang = new Clang();
	// Create the list of sources to be compiled
	// We pass the relative path from the current folder
	KList src = CreateSourceList(libpath);
	//
	// Add angelscript addons
	//
	addAngelScriptAddon("scriptbuilder", ref src, ref Includes);
	addAngelScriptAddon("scriptstdstring", ref src, ref Includes);
	addAngelScriptAddon("scriptarray", ref src, ref Includes);
	addAngelScriptAddon("scriptgrid", ref src, ref Includes);
	addAngelScriptAddon("scriptdictionary", ref src, ref Includes);
	addAngelScriptAddon("scriptmath", ref src, ref Includes);
	addAngelScriptAddon("scriptfile", ref src, ref Includes);

	//
	// Add rasm API
	//
	/*
	string rasmext = Share.Registry("rasm", "extpath");
	if (rasmext == string.Empty) {
		Msg.PrintAndAbort("Rasm API path is not registered.");
		return -1; 
	}
	rasmext = Path.GetRelativePath(CurrentScriptFolder, rasmext);
	src += Glob(rasmext + "*.c");
	*/


	//
	//
	KList switches = new KList() { 
		"-Wall",
		"-Wextra",
		"-Wno-missing-braces" // Glaze...
	};
	//
	clang.Options.SwitchesCC += switches;
	clang.Options.SwitchesCXX += switches;
	clang.Options.Defines += Defines;
	clang.Options.IncludeDirs += Includes;
	//
	// Generate the list of object files to be used as output
	KList objs = src.WithExtension(clang.Options.ObjectExtension).WithPrefix(OutputTmp);
	// We set a delegate to modify parameters on each file build
	clang.ProcessFile += CustomParameters;
	// And compile the sources
	clang.Compile(src, objs);
	// Use the librarian to generate a static library
	clang.Librarian(objs, OutputLib + libname + clang.Options.LibExtension);
	// ------------------------------------------------------------------------
	Msg.PrintTask("Building static library: " + libname + clang.Options.LibExtension);
	Msg.PrintTaskSuccess(" done");
	Msg.Print("---------------------------------------------------------------------------------------");
	Msg.EndIndent();
	register(args);
	return 0;
}

//
// Add an angel script addon cpp and include path based on the name
//
private void addAngelScriptAddon(string name,ref KList src,ref KList Includes) {
	string path = Share.Registry("ascript", "addonpath");
	if (path == string.Empty) {
		Msg.PrintAndAbort("Angelscript addons path is not registered.");
		return;
	}
	Includes += path + name + "/";
	path = Path.GetRelativePath(CurrentScriptFolder, path);
	// We use a glob because angelscript breaks the rule of one folder one item 
	// with the file and filesystem addons (both under the scriptfile folder)
	src += Glob(path + name + "/*.cpp");
}

//
// Register the library
//
int register(string[] args) {
	// Output paths
	KValue OutputLib = KValue.Import("OutputLib");
	OutputLib += libname + "/";
	// Create an instance of the clang tool.
	Clang clang = new Clang();
	// Register the output to make it available for everyone
	Msg.Print($"Registering {libfriendlyname} library under the name: " + libname);
	Msg.BeginIndent();
	Msg.Print("  libname: " + libname + clang.Options.LibExtension);
	Share.Register(libname, "libname", libname + clang.Options.LibExtension);
	Msg.Print("  libpath: " + RealPath(OutputLib));
	Share.Register(libname, "libpath", RealPath(OutputLib));
	Msg.Print("  incpath: " + RealPath(CurrentScriptFolder + "/lib/"));
	Share.Register(libname, "incpath", RealPath(CurrentScriptFolder + "/lib/"));
	Msg.PrintTask("Registered static library: " + libname);
	Msg.PrintTaskSuccess(" done");
	Msg.Print("---------------------------------------------------------------------------------------");
	Msg.EndIndent();
	return 0;
}

//
// Clean artifacts
//
int clean(string[] args) {
	// Output paths
	Msg.Print($"Cleaning: {libfriendlyname}");
	KValue OutputBin = KValue.Import("OutputBin");
	KValue OutputLib = KValue.Import("OutputLib");
	KValue OutputTmp = KValue.Import("OutputTmp");
	KList folders = new KList();
	folders += OutputBin + libname + "/";
	folders += OutputLib + libname + "/";
	folders += OutputTmp + libname + "/";
	Msg.BeginIndent();
	foreach (KValue folder in folders) {
		Msg.Print("Deleting: " + RealPath(folder));
	}
	Msg.EndIndent();
	// Clean the folders
	Folders.Delete(folders, true);
	return 0;
}


//
// Create the source code list to be compiled
//
private KList CreateSourceList(KValue libpath) {
	KList src = new KList();
	//
	// Include all the sources in the library
	// Including the generated resources
	//
	src += Glob(libpath + "**/*.cpp");
	//
	// Is easy to just add all the sources in the glob and later
	// remove all OSX and LNX files (for windows) and so on
	// but we must keep uniformity, all the files that belongs to an specific platform
	// must be inside a folder following the same pattern
	//
	KList srcFiltered = new KList();
	foreach (KValue file in src) {
		string filex = file;
		// For Windows builds we skip everything on osx and lnx folders
		if (Host.IsWindows()) {
			if (filex.Contains("/osx/") || filex.Contains("/lnx/")) {
				continue;
			}
		}
		if (Host.IsLinux()) {
			if (filex.Contains("/win/") || filex.Contains("/osx/")) {
				continue;
			}
		}
		if (Host.IsMacOS()) {
			if (filex.Contains("/win/") || filex.Contains("/lnx/")) {
				continue;
			}
		}
		srcFiltered += file;
	}
	return srcFiltered;
}

//
//
public string CustomParameters(string file) {
	string addArgs = "";
	// Silence warings for some files
	//
	if (file.StartsWith(@"../ext/ascript/sdk/add_on/")) {
		addArgs = " -Wno-unused-function";
		addArgs += " -Wno-invalid-utf8 ";
		addArgs += " -Wno-unused-parameter ";
		addArgs += " -Wno-unused-but-set-variable";
		addArgs += " -Wno-language-extension-token ";
		Msg.Print("Applying: Custom warning removal for angelscript files");
	}
	if (file.StartsWith(@"lib/export")) {
		addArgs += " -Wno-unused-parameter ";
		addArgs += " -Wno-language-extension-token ";
		Msg.Print("Applying: Custom warning removal for angelscript usage");
	}
	if (file == @"lib/assets/source/source.rasm.cpp") {
		addArgs += " -Wno-reserved-user-defined-literal";
		addArgs += " -Wno-deprecated-declarations";
	}
	return addArgs;
}

