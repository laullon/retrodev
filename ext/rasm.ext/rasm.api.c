// --------------------------------------------------------------------------------------------------------------
//
//
//
// 
// Notes:
// Symbol export names vs rasm symbol exporter
// 
//
// --------------------------------------------------------------------------------------------------------------

//
// Wrap rasm inside us so we can tweak it using preprocessor
// and accessing opaque struct members.
// Output stubs are forward-declared here then the macros redirect every
// printf/fprintf/vfprintf call inside rasm.c to them at compile time.
// The macros are undefined after the include so the stubs themselves use
// the real CRT functions.
//
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
//
// Forward declarations of output capture stubs and exit interceptor
//
static int rasm_vfprintf_stub(FILE* stream, const char* format, va_list args);
static int rasm_fprintf_stub(FILE* stream, const char* format, ...);
static int rasm_printf_stub(const char* format, ...);
static void rasm_exit_stub(int code);
static FILE* rasm_fopen_stub(const char* filename, const char* mode);
//
// Redirect all rasm.c output calls, exit() and file opens to our stubs
//
#define vfprintf rasm_vfprintf_stub
#define fprintf rasm_fprintf_stub
#define printf rasm_printf_stub
#define exit rasm_exit_stub
#define fopen rasm_fopen_stub

#include <rasm.c>

//
// Restore real CRT functions and original fopen so everything below uses them directly
//
#undef vfprintf
#undef fprintf
#undef printf
#undef exit
#undef fopen

//
// Creates every directory component in path, walking left-to-right.
// Operates on a mutable copy so the original filename is never modified.
//
static void rasm_ensure_directories(const char* path) {
	char tmp[4096];
	char* p;
	size_t len;
	len = strlen(path);
	if (len == 0 || len >= sizeof(tmp))
		return;
	memcpy(tmp, path, len + 1);
	// Walk forward and create each directory component as we encounter it
	for (p = tmp + 1; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			*p = '\0';
#ifdef OS_WIN
			_mkdir(tmp);
#else
			mkdir(tmp, 0755);
#endif
			*p = '/';
		}
	}
}

//
// fopen stub: ensures parent directories exist before any write-mode open.
// Read-mode opens ("r", "rb") are passed straight through.
//
static FILE* rasm_fopen_stub(const char* filename, const char* mode) {
	if (mode != NULL && mode[0] != 'r')
		rasm_ensure_directories(filename);
	return fopen(filename, mode);
}

//
// Library-mode free: used when ae->flux==0 (file mode) and the process does not exit
// after assembly, so the OS will not reclaim memory automatically.
// Handles the rawfile cleanup that FreeAssenv skips in flux mode, then forces the
// full cleanup path by setting flux=1 before delegating to FreeAssenv.
//
static void rasm_free_assenvlibrary(struct s_assenv* ae) {
	int i;
	if (ae->enforce_symbol_case) {
		for (i = 0; i < ae->ifile; i++) {
			if (ae->rawlen[i])
				MemFree(ae->rawfile[i]);
		}
	}
	ae->flux = 1;
	FreeAssenv(ae);
}

//
// Module-level capture buffer: accumulates all rasm stdout output
// during a single assembly session
//
static char* rasm_cap_buf = NULL;
static size_t rasm_cap_len = 0;
static size_t rasm_cap_cap = 0;

//
// Resets the capture buffer for a new session without releasing the allocation
//
static void rasm_cap_reset(void) {
	rasm_cap_len = 0;
	if (rasm_cap_buf != NULL)
		rasm_cap_buf[0] = '\0';
}

//
// Appends len bytes from str to the capture buffer, growing it as needed
//
static void rasm_cap_append(const char* str, int len) {
	size_t needed;
	char* newbuf;
	if (len <= 0 || str == NULL)
		return;
	needed = rasm_cap_len + (size_t)len + 1;
	if (needed > rasm_cap_cap) {
		size_t newcap = rasm_cap_cap == 0 ? 4096 : rasm_cap_cap;
		while (newcap < needed)
			newcap *= 2;
		newbuf = (char*)realloc(rasm_cap_buf, newcap);
		if (newbuf == NULL)
			return;
		rasm_cap_buf = newbuf;
		rasm_cap_cap = newcap;
	}
	memcpy(rasm_cap_buf + rasm_cap_len, str, (size_t)len);
	rasm_cap_len += (size_t)len;
	rasm_cap_buf[rasm_cap_len] = '\0';
}

//
// vfprintf stub: captures stdout writes, passes stderr through unchanged
//
static int rasm_vfprintf_stub(FILE* stream, const char* format, va_list args) {
	va_list args_copy;
	int len;
	char* tmp;
	if ((stream != stdout) && (stream != stderr))
		return vfprintf(stream, format, args);
	va_copy(args_copy, args);
	len = vsnprintf(NULL, 0, format, args);
	if (len > 0) {
		tmp = (char*)malloc((size_t)len + 1);
		if (tmp != NULL) {
			vsnprintf(tmp, (size_t)len + 1, format, args_copy);
			rasm_cap_append(tmp, len);
			free(tmp);
		}
	}
	va_end(args_copy);
	return len;
}

//
// fprintf stub: delegates to vfprintf stub
//
static int rasm_fprintf_stub(FILE* stream, const char* format, ...) {
	va_list args;
	int ret;
	va_start(args, format);
	ret = rasm_vfprintf_stub(stream, format, args);
	va_end(args);
	return ret;
}

//
// printf stub: delegates to vfprintf stub targeting stdout
//
static int rasm_printf_stub(const char* format, ...) {
	va_list args;
	int ret;
	va_start(args, format);
	ret = rasm_vfprintf_stub(stdout, format, args);
	va_end(args);
	return ret;
}

//
// Transfers the capture buffer ownership to *textout and resets the module state
//
static void rasm_cap_transfer(char** textout) {
	if (textout != NULL) {
		*textout = rasm_cap_buf;
		rasm_cap_buf = NULL;
		rasm_cap_len = 0;
		rasm_cap_cap = 0;
	}
}

//
// Module-level exit interception state:
// rasm_exit_jmp    jump target set in RasmAssembleIntegrated before assembly
// rasm_exit_code   exit code captured from the exit() call
// rasm_exit_active flag: longjmp only when a session is in progress
// rasm_current_ae  ae pointer saved so the recovery path can free it;
//               FreeAssenv is a no-op in flux==0 so ae survives exit()
//
static jmp_buf rasm_exit_jmp;
static int rasm_exit_code = 0;
static int rasm_exit_active = 0;
static struct s_assenv* rasm_current_ae = NULL;

//
// exit() stub: saves the code and longjmps back to RasmAssembleIntegrated.
// If called outside an active session (should not happen) it is a no-op.
//
static void rasm_exit_stub(int code) {
	rasm_exit_code = code;
	if (rasm_exit_active)
		longjmp(rasm_exit_jmp, 1);
}

//
// Assembles the file described by param.
// On return, *textout receives a heap-allocated null-terminated string with all
// assembler output; the caller must free() it. Pass NULL to discard the output.
//
int RasmAssembleIntegrated(struct s_parameter* param, char** textout) {
	int ret;
	//
	// Initialise per-session state before arming the exit interceptor
	//
	rasm_cap_reset();
	rasm_exit_code = 0;
	rasm_current_ae = NULL;
	rasm_exit_active = 1;
	//
	// setjmp returns 0 on first call; non-zero when longjmp fires from exit()
	//
	if (setjmp(rasm_exit_jmp) != 0) {
		//
		// rasm called exit(): FreeAssenv was a no-op (flux==0) so ae is still live
		//
		rasm_exit_active = 0;
		if (rasm_current_ae != NULL) {
			rasm_free_assenvlibrary(rasm_current_ae);
			rasm_current_ae = NULL;
		}
		rasm_cap_transfer(textout);
		return rasm_exit_code;
	}
	rasm_current_ae = PreProcessing(param->filename, 0, NULL, 0, param);
	ret = Assemble(rasm_current_ae, NULL, NULL, NULL);
	//
	// Normal completion: disarm exit interceptor and free ae
	//
	rasm_exit_active = 0;
	rasm_free_assenvlibrary(rasm_current_ae);
	rasm_current_ae = NULL;
	rasm_cap_transfer(textout);
	return ret;
}