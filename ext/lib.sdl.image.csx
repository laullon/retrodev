/*---------------------------------------------------------------------------------------------------------


---------------------------------------------------------------------------------------------------------*/
#load "bld/ext/clang.csx"
#load "bld/ext/git.csx"

#r "../../mkb.dll"
using Kltv.Kombine.Api;
using Kltv.Kombine.Types;
using static Kltv.Kombine.Api.Statics;
using static Kltv.Kombine.Api.Tool;

// Name of the library for the output
KValue sdl3friendlyname = "SDL Image (v3.4.x)";
// Path to store the sdl3 sources
KValue sdl3path = "sdl.img/";
// Branch to use for the sdl3 sources
KValue sdl3branch = "release-3.4.x";
// Name of the library for the output
KValue sdl3name = "sdl.img";
// Repository to use for the sdl3 sources
KValue sdl3repo = "https://github.com/libsdl-org/SDL_image.git";

// SDL_image configuration
private KList Defines = new KList();
		Defines += "SDL_IMAGE_USE_COMMON_BACKEND";
		Defines += "USE_STBIMAGE";
		Defines += "LOAD_XCF";
		Defines += "LOAD_XPM";
		Defines += "LOAD_XV";
		Defines += "LOAD_BMP";
		Defines += "LOAD_GIF";
		Defines += "LOAD_JPG";
		Defines += "LOAD_LBM";
		Defines += "LOAD_PCX";
		Defines += "LOAD_PNG";
		Defines += "LOAD_PNM";
		Defines += "LOAD_QOI";
		Defines += "LOAD_SVG";
		Defines += "LOAD_TGA";
if (Host.IsWindows()) {
	Defines += "LOAD_AVIF";
	Defines += ArgEscape("LOAD_AVIF_DYNAMIC=\"libavif-16.dll\"");
	Defines += "LOAD_TIF";
	Defines += ArgEscape("LOAD_TIF_DYNAMIC=\"libtiff-6.dll\"");
	Defines += "LOAD_WEBP";
	Defines += ArgEscape("LOAD_WEBP_DYNAMIC=\"libwebp-7.dll\"");
	Defines += ArgEscape("LOAD_WEBPMUX_DYNAMIC=\"libwebpmux-3.dll\"");
	Defines += ArgEscape("LOAD_WEBPDEMUX_DYNAMIC=\"libwebpdemux-2.dll\"");
}


//
// Manage the SDL3 image library dependency
//
int dependencies(string[] args){
	if (Args.Get(0) == "clean") {
		Msg.Print($"Cleaning {sdl3friendlyname} sources");
		Folders.Delete(sdl3path,true);
		return 0;
	}
	if (Args.Get(0) == "update") {
		if (Folders.Exists(sdl3path)){
			Msg.Print($"Updating {sdl3friendlyname} sources");
			// Sdl3 image is not branched so update is just update the label
			//
			//Git.Pull(sdl3path);
			return 0;
		}
	}
	if ( (Args.Get(0) == "install") || (Args.Get(0) == "update")) {
		Msg.Print($"Cloning {sdl3friendlyname} sources");
		if (Folders.Exists(sdl3path)){
			Msg.BeginIndent();
			Msg.Print($"{sdl3friendlyname} folder already present. Skip");
			Msg.EndIndent();
			return 0;
		}
		Git.Clone(sdl3repo,sdl3path,sdl3branch);
		return 0;
	}
	Msg.Print("No valid parameter action supplied.");
	return -1;
}

//
// Build the SDL3 image library Action
//
int build(string[] args){
	Msg.Print($"Building {sdl3friendlyname} Image library");
	Msg.BeginIndent();
	// Check if the sdl3 sources folder is present
	if (!Folders.Exists(sdl3path)){
		Msg.PrintAndAbort($"{sdl3friendlyname} image sources not found. Please run 'mkb dependencies install' to get the sources.");
	}
	// Output paths
	KValue OutputBin = KValue.Import("OutputBin");
	KValue OutputLib = KValue.Import("OutputLib");
	KValue OutputTmp = KValue.Import("OutputTmp");
	// Include directories
	KList Includes = new KList();
	Includes += RealPath(sdl3path+"include/");
	Includes += RealPath(sdl3path+"visualc/external/include/");
	Includes += Share.Registry("sdl","incpath");
	// Add the artifact name into the output folders
	OutputBin += sdl3name + "/";
	OutputLib += sdl3name + "/";
	// Create an instance of the clang tool.
	Clang clang = new Clang();
	// Create the list of sources to be compiled
	// We pass the relative path from the current folder
	KList src = CreateSourceList(sdl3path);
	// Override completly the defines since some previous one could affect
	// sdl3 image has issues with "DEBUG" define
	clang.Options.Defines = Defines;
	clang.Options.IncludeDirs += Includes;
	// Generate the list of object files to be used as output
	KList objs = src.WithExtension(clang.Options.ObjectExtension).WithPrefix(OutputTmp);
	// And compile the sources
	clang.Compile(src, objs);
	// Use the librarian to generate a static library
	clang.Librarian(objs, OutputLib + sdl3name + clang.Options.LibExtension);
	// ------------------------------------------------------------------------
	Msg.PrintTask("Building static library: " + sdl3name + clang.Options.LibExtension);
	Msg.PrintTaskSuccess(" done");
	Msg.Print("---------------------------------------------------------------------------------------");
	Msg.EndIndent();
	register (args);
	return 0;
}

//
// Register the library
//
int register(string[] args) {
	Msg.Print($"Registering {sdl3friendlyname} library");
	Msg.BeginIndent();
	KValue OutputLib = KValue.Import("OutputLib");
	OutputLib += sdl3name + "/";
	// Create an instance of the clang tool.
	Clang clang = new Clang();
	string registeredLibName = sdl3name + clang.Options.LibExtension;
	if (Host.IsMacOS() || Host.IsLinux()) {
		registeredLibName = sdl3name;
	}
	// Register the output to make it available for everyone
	Msg.Print($"Registering {sdl3friendlyname} library under the name: "+sdl3name);
	Msg.Print("  libname: " + registeredLibName);
	Share.Register(sdl3name,"libname",registeredLibName);
	Msg.Print("  libpath: " + RealPath(OutputLib));
	Share.Register(sdl3name,"libpath",RealPath(OutputLib));
	Msg.Print("  incpath: " + RealPath(sdl3path+"include/"));
	Share.Register(sdl3name,"incpath",RealPath(sdl3path+"include/"));
	Msg.EndIndent();
	Msg.PrintTask("Registered static library: " + sdl3name);
	Msg.PrintTaskSuccess(" done");
	Msg.Print("---------------------------------------------------------------------------------------");
	return 0;
}

//
// Clean artifacts
//
int clean(string[] args){
	// Output paths
	Msg.Print("Cleaning: "+sdl3friendlyname);
	KValue OutputBin = KValue.Import("OutputBin");
	KValue OutputLib = KValue.Import("OutputLib");
	KValue OutputTmp = KValue.Import("OutputTmp");
	KList folders = new KList();
	folders += OutputBin+sdl3name+"/";
	folders += OutputLib+sdl3name+"/";
	folders += OutputTmp+sdl3name+"/";
	Msg.BeginIndent();
	foreach(KValue folder in folders){
		Msg.Print("Deleting: "+RealPath(folder));
	}
	Msg.EndIndent();
	// Clean the folders
	Folders.Delete(folders,true);
	return 0;
}

//
// Create the source code list to be compiled
//
private KList CreateSourceList(KValue sdl3path){
	KList src = new KList();
	// sdl3 common source code
	src += sdl3path+"src/IMG.c";
	src += sdl3path+"src/IMG_ani.c";
	src += sdl3path+"src/IMG_anim_decoder.c";
	src += sdl3path+"src/IMG_anim_encoder.c";
	src += sdl3path+"src/IMG_bmp.c";
	src += sdl3path+"src/IMG_gif.c";
	src += sdl3path+"src/IMG_jpg.c";
	src += sdl3path+"src/IMG_lbm.c";
	src += sdl3path+"src/IMG_pcx.c";
	src += sdl3path+"src/IMG_libpng.c";
	src += sdl3path+"src/IMG_png.c";
	src += sdl3path+"src/IMG_pnm.c";
	src += sdl3path+"src/IMG_qoi.c";
	src += sdl3path+"src/IMG_stb.c";
	src += sdl3path+"src/IMG_svg.c";
	src += sdl3path+"src/IMG_tga.c";
	src += sdl3path+"src/IMG_xcf.c";
	src += sdl3path+"src/IMG_xpm.c";
	src += sdl3path+"src/IMG_xv.c";
	src += sdl3path+"src/xmlman.c";
	if (Host.IsWindows()) {
		src += sdl3path+"src/IMG_avif.c";
		src += sdl3path+"src/IMG_tif.c";
		src += sdl3path+"src/IMG_webp.c";
		src += sdl3path+"src/IMG_WIC.c";
	}
	return src;
}
