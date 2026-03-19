//----------------------------------------------------------------------------------------------------
//
//
//
//
//
//----------------------------------------------------------------------------------------------------

#include "../imgui.text.editor.h"

namespace ImGui {
	namespace TextEditorLangs {
		namespace Python {
			//
			// Python reserved keywords
			//
			const char* const kPythonKeywords[] = {"False",	  "await",	"else",	  "import", "pass",		"None", "break",  "except", "in", "raise", "True", "class",
												   "finally", "is",		"return", "and",	"continue", "for",	"lambda", "try",	"as", "def",   "from", "nonlocal",
												   "while",	  "assert", "del",	  "global", "not",		"with", "async",  "elif",	"if", "or",	   "yield"};
			//
			// Python built-in functions and types
			//
			const char* const kPythonIdentifiers[] = {
				"abs",		  "aiter",		 "all",			 "any",		"anext",   "ascii",	   "bin",	"bool",	  "breakpoint", "bytearray", "bytes",	  "callable",
				"chr",		  "classmethod", "compile",		 "complex", "delattr", "dict",	   "dir",	"divmod", "enumerate",	"eval",		 "exec",	  "filter",
				"float",	  "format",		 "frozenset",	 "getattr", "globals", "hasattr",  "hash",	"help",	  "hex",		"id",		 "input",	  "int",
				"isinstance", "issubclass",	 "iter",		 "len",		"list",	   "locals",   "map",	"max",	  "memoryview", "min",		 "next",	  "object",
				"oct",		  "open",		 "ord",			 "pow",		"print",   "property", "range", "repr",	  "reversed",	"round",	 "set",		  "setattr",
				"slice",	  "sorted",		 "staticmethod", "str",		"sum",	   "super",	   "tuple", "type",	  "vars",		"zip",		 "__import__"};
		} // namespace Python
	} // namespace TextEditorLangs

	const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::Python() {
		static bool inited = false;
		static LanguageDefinition langDef;
		if (!inited) {
			// Populate keywords
			for (auto& k : TextEditorLangs::Python::kPythonKeywords)
				langDef.mKeywords.insert(k);
			// Populate known identifiers
			for (auto& k : TextEditorLangs::Python::kPythonIdentifiers) {
				Identifier id;
				id.mDeclaration = "Built-in function";
				langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
			}
			langDef.mCommentStart = "\"\"\"";
			langDef.mCommentEnd = "\"\"\"";
			langDef.mSingleLineComment = "#";
			langDef.mCaseSensitive = true;
			langDef.mName = "Python";
			inited = true;
		}
		return langDef;
	}

} // namespace ImGui
