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
		namespace Lua {
			//
			// Lua reserved keywords
			//
			const char* const kLuaKeywords[] = {"and", "break", "do",  "else", "elseif", "end",	   "false",	 "for",	 "function", "goto",  "if",
												"in",  "local", "nil", "not",  "or",	 "repeat", "return", "then", "true",	 "until", "while"};
			//
			// Lua standard library identifiers and built-in functions
			//
			const char* const kLuaIdentifiers[] = {"assert",	 "collectgarbage", "dofile",	   "error",		  "getmetatable", "ipairs",		  "loadfile",	  "load",
												   "loadstring", "next",		   "pairs",		   "pcall",		  "print",		  "rawequal",	  "rawlen",		  "rawget",
												   "rawset",	 "select",		   "setmetatable", "tonumber",	  "tostring",	  "type",		  "xpcall",		  "_G",
												   "_VERSION",	 "arshift",		   "band",		   "bnot",		  "bor",		  "bxor",		  "btest",		  "extract",
												   "lrotate",	 "lshift",		   "replace",	   "rrotate",	  "rshift",		  "create",		  "resume",		  "running",
												   "status",	 "wrap",		   "yield",		   "isyieldable", "debug",		  "getuservalue", "gethook",	  "getinfo",
												   "getlocal",	 "getregistry",	   "getmetatable", "getupvalue",  "upvaluejoin",  "upvalueid",	  "setuservalue", "sethook",
												   "setlocal",	 "setmetatable",   "setupvalue",   "traceback",	  "close",		  "flush",		  "input",		  "lines",
												   "open",		 "output",		   "popen",		   "read",		  "tmpfile",	  "type",		  "write",		  "close",
												   "flush",		 "lines",		   "read",		   "seek",		  "setvbuf",	  "write",		  "__gc",		  "__tostring",
												   "abs",		 "acos",		   "asin",		   "atan",		  "ceil",		  "cos",		  "deg",		  "exp",
												   "tointeger",	 "floor",		   "fmod",		   "ult",		  "log",		  "max",		  "min",		  "modf",
												   "rad",		 "random",		   "randomseed",   "sin",		  "sqrt",		  "string",		  "tan",		  "type",
												   "atan2",		 "cosh",		   "sinh",		   "tanh",		  "pow",		  "frexp",		  "ldexp",		  "log10",
												   "pi",		 "huge",		   "maxinteger",   "mininteger",  "loadlib",	  "searchpath",	  "seeall",		  "preload",
												   "cpath",		 "path",		   "searchers",	   "loaded",	  "module",		  "require",	  "clock",		  "date",
												   "difftime",	 "execute",		   "exit",		   "getenv",	  "remove",		  "rename",		  "setlocale",	  "time",
												   "tmpname",	 "byte",		   "char",		   "dump",		  "find",		  "format",		  "gmatch",		  "gsub",
												   "len",		 "lower",		   "match",		   "rep",		  "reverse",	  "sub",		  "upper",		  "pack",
												   "packsize",	 "unpack",		   "concat",	   "maxn",		  "insert",		  "pack",		  "unpack",		  "remove",
												   "move",		 "sort",		   "offset",	   "codepoint",	  "char",		  "len",		  "codes",		  "charpattern",
												   "coroutine",	 "table",		   "io",		   "os",		  "string",		  "utf8",		  "bit32",		  "math",
												   "debug",		 "package"};
			static bool TokenizeLuaStyleString(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				const char* p = in_begin;
				bool is_single_quote = false;
				bool is_double_quotes = false;
				bool is_double_square_brackets = false;
				switch (*p) {
					case '\'':
						is_single_quote = true;
						break;
					case '"':
						is_double_quotes = true;
						break;
					case '[':
						p++;
						if (p < in_end && *(p) == '[')
							is_double_square_brackets = true;
						break;
				}
				if (is_single_quote || is_double_quotes || is_double_square_brackets) {
					p++;
					while (p < in_end) {
						if ((is_single_quote && *p == '\'') || (is_double_quotes && *p == '"') || (is_double_square_brackets && *p == ']' && p + 1 < in_end && *(p + 1) == ']')) {
							out_begin = in_begin;
							out_end = is_double_square_brackets ? p + 2 : p + 1;
							return true;
						}
						if (*p == '\\' && p + 1 < in_end && (is_single_quote || is_double_quotes))
							p++;
						p++;
					}
				}
				return false;
			}
			static bool TokenizeLuaStyleIdentifier(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				const char* p = in_begin;
				if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') {
					p++;
					while ((p < in_end) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
						p++;
					out_begin = in_begin;
					out_end = p;
					return true;
				}
				return false;
			}
			static bool TokenizeLuaStyleNumber(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				const char* p = in_begin;
				const bool startsWithNumber = *p >= '0' && *p <= '9';
				if (*p != '+' && *p != '-' && !startsWithNumber)
					return false;
				p++;
				bool hasNumber = startsWithNumber;
				while (p < in_end && (*p >= '0' && *p <= '9')) {
					hasNumber = true;
					p++;
				}
				if (hasNumber == false)
					return false;
				if (p < in_end) {
					if (*p == '.') {
						p++;
						while (p < in_end && (*p >= '0' && *p <= '9'))
							p++;
					}
					if (p < in_end && (*p == 'e' || *p == 'E')) {
						p++;
						if (p < in_end && (*p == '+' || *p == '-'))
							p++;
						bool hasDigits = false;
						while (p < in_end && (*p >= '0' && *p <= '9')) {
							hasDigits = true;
							p++;
						}
						if (hasDigits == false)
							return false;
					}
				}
				out_begin = in_begin;
				out_end = p;
				return true;
			}
			static bool TokenizeLuaStylePunctuation(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				(void)in_end;
				switch (*in_begin) {
					case '[':
					case ']':
					case '{':
					case '}':
					case '!':
					case '%':
					case '#':
					case '^':
					case '&':
					case '*':
					case '(':
					case ')':
					case '-':
					case '+':
					case '=':
					case '~':
					case '|':
					case '<':
					case '>':
					case '?':
					case ':':
					case '/':
					case ';':
					case ',':
					case '.':
						out_begin = in_begin;
						out_end = in_begin + 1;
						return true;
				}
				return false;
			}
		} // namespace Lua
	} // namespace TextEditorLangs

	const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::Lua() {
		static bool inited = false;
		static LanguageDefinition langDef;
		if (!inited) {
			// Populate keywords
			for (auto& k : TextEditorLangs::Lua::kLuaKeywords)
				langDef.mKeywords.insert(k);
			// Populate known identifiers
			for (auto& k : TextEditorLangs::Lua::kLuaIdentifiers) {
				Identifier id;
				id.mDeclaration = "Built-in function";
				langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
			}
			// Lua-style tokenizer: strings (single, double, [[long]]), identifiers, numbers, punctuation
			langDef.mTokenize = [](const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end, PaletteIndex& paletteIndex) -> bool {
				paletteIndex = PaletteIndex::Max;
				while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
					in_begin++;
				if (in_begin == in_end) {
					out_begin = in_end;
					out_end = in_end;
					paletteIndex = PaletteIndex::Default;
				} else if (TextEditorLangs::Lua::TokenizeLuaStyleString(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::String;
				else if (TextEditorLangs::Lua::TokenizeLuaStyleIdentifier(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Identifier;
				else if (TextEditorLangs::Lua::TokenizeLuaStyleNumber(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Number;
				else if (TextEditorLangs::Lua::TokenizeLuaStylePunctuation(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Punctuation;
				return paletteIndex != PaletteIndex::Max;
			};
			langDef.mCommentStart = "--[[";
			langDef.mCommentEnd = "]]";
			langDef.mSingleLineComment = "--";
			langDef.mCaseSensitive = true;
			langDef.mName = "Lua";
			inited = true;
		}
		return langDef;
	}

} // namespace ImGui
