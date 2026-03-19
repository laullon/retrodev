/*---------------------------------------------------------------------------------------------------------



---------------------------------------------------------------------------------------------------------*/

#load "bld/ext/git.csx"

#r "../bld/bin/mkb.dll"
using Kltv.Kombine.Api;
using Kltv.Kombine.Types;
using static Kltv.Kombine.Api.Statics;
using static Kltv.Kombine.Api.Tool;

KValue libfriendname = "Glaze (v2.9.5)";
KValue libname = "glaze";
// Path to store the library sources
KValue libpath = "glaze/";
// URL to download the library sources
KValue liburl = "https://github.com/stephenberry/glaze";
// Friendly name of the library version
KValue version = "v2.9.5";

// Build the library
// Glaze is a header only library, so we don't need to build anything, just register the include path
//
int build(string[] args){
	Msg.Print($"Building {libfriendname} library");
	Msg.BeginIndent();
	if (!Folders.Exists(libpath)) {
		Msg.PrintAndAbort($"{libfriendname} sources not found. Please run 'mkb dependencies install' to get the sources.");
	}
	// ------------------------------------------------------------------------
	Msg.PrintTask("Preparing header library: " + libname);
	Msg.PrintTaskSuccess(" done");
	Msg.Print("---------------------------------------------------------------------------------------");
	Msg.EndIndent();
	// Build register the library as well.
	register(args);
	return 0;
}

// Register the library output
//
int register(string[] args) {
	Msg.Print($"Registering {libfriendname} library");
	Msg.BeginIndent();
	// Register the output to make it available for everyone
	Msg.Print($"Registering {libfriendname} library under the name: "+libname);
	Msg.Print("  incpath: " + RealPath(libpath+"include/"));
	Share.Register(libname,"incpath",RealPath(libpath+"include/"));
	Msg.PrintTask("Register header library: " + libname );
	Msg.PrintTaskSuccess(" done");
	Msg.Print("---------------------------------------------------------------------------------------");
	Msg.EndIndent();
	return 0;
}
//
// Clean artifacts
//
int clean(string[] args){
	// Nothing to clean since Glaze is a header only library and doesn't produce any artifacts
	Msg.Print($"Cleaning: {libfriendname} Nothing to do.");
	return 0;
}

//
// Manage the library dependency
//
int dependencies(string[] args) {
	if (Args.Get(0) == "clean") {
		Msg.Print($"Cleaning {libfriendname} sources");
		Folders.Delete(libpath,true);
		return 0;
	}
	if (Args.Get(0) == "update") {
		Msg.Print($"Updating {libfriendname} sources");
		if (Folders.Exists(libpath)){
			//
			// Glaze is not branched so update is just update the label
			//
			//Git.Pull(libpath);
			return 0;
		}
	}
	if ( (Args.Get(0) == "install") || (Args.Get(0) == "update")) {
		return install(args);
	}
	Msg.Print("No valid parameter action supplied.");
	return -1;
}

// Private method to fetch the library sources
// Not exposed as an action
private int install(string[] args) {
	Msg.Print($"Cloning {libfriendname} "+version);
	if (Folders.Exists(libpath)){
		Msg.BeginIndent();
		Msg.Print($"{libfriendname} folder already present. Skip");
		Msg.EndIndent();
		return 0;
	}
	Git.Clone(liburl,libpath,version);
	return 0;
}