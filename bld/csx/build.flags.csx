/*---------------------------------------------------------------------------------------------------------

	Retro Dev 

	Build Flags

	(c) TLOTB 2026

---------------------------------------------------------------------------------------------------------*/

//
// Just for intellisense, has no impact when building
//
#r "../bin/mkb.dll"
using Kltv.Kombine.Api;
using Kltv.Kombine.Types;
using static Kltv.Kombine.Api.Statics;
using static Kltv.Kombine.Api.Tool;


public class BuildFlags {
	public class BuildFlagsStruct {
		public BuildFlagsStruct() {
			BuildMode = "debug";
			Verbose = false;
			Production = false;
			Deps = false;
		}
		// Build mode: debug or release
		public string BuildMode {get;set;}
		// Verbose mode (it will show clang invocation)
		public bool Verbose {get;set;}
		// Production mode (it will remove debug information / developer settings)
		public bool Production { get; set; }
		// If we should include the dependencies in the operation
		public bool Deps { get; set; }
	}

	private static BuildFlagsStruct? buildFlags = null;
	public static BuildFlagsStruct Flags {
		get {
			if (buildFlags == null) {
				object? obj = Share.Get("RetroDevBuildFlags");
				if (obj != null) {
					//BuildFlagsStruct? opt = CastObject(obj);
					BuildFlagsStruct? opt = Cast<BuildFlagsStruct>(obj);
					if (opt != null) {
						buildFlags = opt;
						return buildFlags;
					}
				}
				if (buildFlags == null){
					Msg.PrintAndAbort("Error: Build flags not initialized. Please run BuildFlagsInit() first.");
					buildFlags = new BuildFlagsStruct();
					// It is just to remove the warning, we will never reach this point
				}
			}
			return buildFlags;
		}
	}

	//
	// Cast function in Kombine works only with properties and not members
	// We add ours here instead to work on members (just as an example if required for other tooling)
	//
	public static BuildFlagsStruct? CastObject(object? obj) {
		if (obj == null) {
			return null;
		}
		BuildFlagsStruct flags = new BuildFlagsStruct();
		Type objType = obj.GetType();
		Type flagsType = typeof(BuildFlagsStruct);
		foreach (var objMember in objType.GetMembers()) {
			if (objMember.MemberType == System.Reflection.MemberTypes.Field) {
				var field = (System.Reflection.FieldInfo)objMember;
				var flagsField = flagsType.GetField(field.Name);
				if (flagsField != null && flagsField.FieldType == field.FieldType) {
					flagsField.SetValue(flags, field.GetValue(obj));
				}
			} else if (objMember.MemberType == System.Reflection.MemberTypes.Property) {
				var prop = (System.Reflection.PropertyInfo)objMember;
				var flagsProp = flagsType.GetProperty(prop.Name);
				if (flagsProp != null && flagsProp.PropertyType == prop.PropertyType && prop.CanRead && flagsProp.CanWrite) {
					flagsProp.SetValue(flags, prop.GetValue(obj));
				}
			}
		}
		return flags;
	}


	//
	// Print the build flags
	//
	public static void Print(){
		Msg.Print("Build flags:");
		if (buildFlags == null) {
			Msg.Print("  Not initialized");
			return;
		}
		Msg.Print("  Mode: " + buildFlags.BuildMode);
		Msg.Print("  Verbose: " + buildFlags.Verbose);
		Msg.Print("  Production: " + buildFlags.Production);
		Msg.Print("  Include Deps: " + buildFlags.Deps);
	}

	//
	// Initialize the build flags from the command line arguments
	//
	public static void Init() {
		// Create a new object to store the build flags
		buildFlags = new BuildFlagsStruct();
		// Parse parameters to fetch build config options
		if (Args.Contains("release")) {
			buildFlags.BuildMode = "release";
		} else {
			buildFlags.BuildMode = "debug";
		}
		if (Args.Contains("verbose")) {
			buildFlags.Verbose = true;
		}
		if (Args.Contains("production")) {
			buildFlags.Production = true;
		}
		if (Args.Contains("deps")) {
			buildFlags.Deps = true;
		}
		// And share the object for the rest of the projects
		Share.Set("RetroDevBuildFlags", buildFlags);
	}
}