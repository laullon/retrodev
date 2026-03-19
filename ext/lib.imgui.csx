/*---------------------------------------------------------------------------------------------------------


---------------------------------------------------------------------------------------------------------*/
#load "bld/ext/git.csx"

#r "../bld/bin/mkb.dll"
using Kltv.Kombine.Api;
using Kltv.Kombine.Types;
using static Kltv.Kombine.Api.Statics;
using static Kltv.Kombine.Api.Tool;

// Name of the library for the output
KValue libfriendlyname = "ImGui (v1.92.6)";
// Name of the library for the register
KValue libname = "imgui";
// Repository to use for the lib sources
KValue librepo = "https://github.com/ocornut/imgui.git";
// Path to store the imgui sources
KValue libpath = "imgui/";
// Name of the branch/tag/commit to fetch
KValue libbranch = "v1.92.6";

//
// Manage the ImGUI dependency
//
int dependencies(string[] args){
	if (Args.Get(0) == "clean") {
		Msg.Print($"Cleaning {libfriendlyname} sources");
		Folders.Delete(libpath,true);
		return 0;
	}
	if (Args.Get(0) == "update") {
		Msg.Print($"Updating {libfriendlyname} sources");
		if (Folders.Exists(libpath)){
			//
			// Imgui is not branched so update is just update the label
			//
			//Git.Pull(libpath);
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
		Git.Clone(librepo, libpath,libbranch);
		return 0;
	}
	Msg.Print("No valid parameter action supplied.");
	return -1;
}

//
// ImGUI library is only checked on build
// It is not build directly but from source code on the library project
//
int build(string[] args){
	Msg.Print($"Building {libfriendlyname} library");
	Msg.BeginIndent();
	// Check if the sdl2 sources folder is present
	if (!Folders.Exists(libpath)){
		Msg.PrintAndAbort($"{libfriendlyname} sources not found. Please run 'mkb dependencies install' to get the sources.");
	}
	Msg.Print($"{libfriendlyname} sources found. Nothing else to do.");
	Msg.Print("---------------------------------------------------------------------------------------");
	Msg.EndIndent();
	register(args);
	return 0;
}

//
// Register the library
//
int register(string[] args) {
	Msg.Print($"Registering {libfriendlyname} library");
	Msg.BeginIndent();
	// Register the output to make it available for everyone
	Msg.Print($"Registering {libfriendlyname} library under the name: "+libname);
	// ImGui includes
	// 
	Msg.Print("  incpath: " + RealPath(libpath));
	Share.Register(libname,"incpath",RealPath(libpath));
	// ImGui backends includes
	//
	Msg.Print("  incbackendspath: " + RealPath(libpath+"backends/"));
	Share.Register(libname,"incbackendspath",RealPath(libpath+"backends/"));
	// ImGui extensions includes
	//
	Msg.Print("  incextensionspath: " + RealPath("imgui.ext/"));
	Share.Register(libname,"incextensionspath",RealPath("imgui.ext/"));
	// ImGui extensions includes
	//
	Msg.Print("  incconfigpath: " + RealPath("imgui.cfg/"));
	Share.Register(libname, "incconfigpath", RealPath("imgui.cfg/"));
	// ImGui sources
	//
	Msg.Print("  srcpath: " + RealPath(libpath));
	Share.Register(libname,"srcpath",RealPath(libpath));
	Msg.PrintTask("Register source library: " + libname );
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
	folders += OutputTmp+"ext/"+libname+"/";
	Msg.BeginIndent();
	foreach(KValue folder in folders){
		Msg.Print("Deleting: "+RealPath(folder));
	}
	Msg.EndIndent();
	// Clean the folders
	Folders.Delete(folders,true);
	return 0;
}

