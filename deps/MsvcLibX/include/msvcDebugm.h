/*****************************************************************************\
*                                                                             *
*   Filename:	    debugm.h						      *
*                                                                             *
*   Description:    Debug macros					      *
*                                                                             *
*   Notes:	    Macros inspired by my Tcl/Bash/Batch debugging libraries. *
*		    The idea is to output the function call stack, indenting  *
*		    subroutines proportionally to the call depth.             *
*                   To ease reading the output, it must look like real C code.*
*                                                                             *
*    DEBUG_GLOBALS              Define global variables used by macros below: *
*    int iDebug = FALSE;        Global variable enabling debug output if TRUE.*
*    int iIndent = 0;           Global variable controlling debug indentation.*
*                                                                             *
*    DEBUG_PRINTF((format, ...))	Print a debug string if debug is on.  *
*                                       The double parenthesis are necessary  *
*                                       because C90 does not support macros   *
*                                       with variable list of arguments.      *
*    DEBUG_ENTER((format, ...))		Print a function name and arguments.  *
*                                       Increase indentation of further calls.*
*                                       It's the caller's responsibility to   *
*                                       format the routine name and arguments.*
*    DEBUG_LEAVE((format, ...))		Print a function return value.        *
*                                       Decrease indentation of further calls.*
*                                       It's the caller's responsibility to   *
*                                       format the return instruction & value.*
*                                                                             *
*                   Any call to DEBUG_ENTER MUST be matched by one call to    *
*                   DEBUG_LEAVE or RETURN_... when the function returns.      *                                                                             *
*                                                                             *
*   History:								      *
*    2012-01-16 JFL jf.larvoire@hp.com created this file.                     *
*    2012-02-03 JFL Renamed DEBUG_IF_IS_ON DEBUG_CODE_IF_ON.                  *
*		    Renamed file from debug.h to debugm.h because of a file   *
*		    name collision with another library on my PC.	      *
*    2014-02-10 JFL Added macros for an extra debug mode.		      *
*    2014-07-02 JFL renamed macro RETURN() as RETURN_CONST(), and defined     *
*		    new macro RETURN() to return nothing.		      *
*		    Idem for RETURN_COMMENT() as RETURN_CONST_COMMENT().      *
*    2016-09-09 JFL Flush every DEBUG_PRINTF output, to make sure to see      *
*		    every debug string printed before a program crash.	      *
*    2016-09-13 JFL Added macros DEBUG_WSTR2NEWUTF8() and DEBUG_FREEUTF8().   *
*    2016-10-04 JFL Added macros DEBUG_OFF(), DEBUG_MORE(), DEBUG_LESS().     *
*		    Allow using DEBUG_ON()/MORE()/LESS()/OFF() in release mode.
*		    							      *
*         Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#ifndef	_DEBUGM_H
#define	_DEBUGM_H	1

#include <stdio.h>	/* Macros use printf */

#ifdef _MSC_VER
#pragma warning(disable:4127) /* Avoid warnings on while(0) below */
#endif

#define DEBUG_DO(code) do {code} while (0)
#define DEBUG_DO_NOTHING() do {} while (0)

/* Conditional compilation based on Microsoft's standard _DEBUG definition */
#if defined(_DEBUG)

#define DEBUG_VERSION " Debug"

#define DEBUG_GLOBALS \
int iDebug = 0;		/* Global variable enabling debug output if TRUE. */ \
int iIndent = 0;	/* Global variable controlling debug indentation. */

extern int iDebug;	/* Global variable enabling of disabling debug messages */
#define DEBUG_ON() iDebug = 1		/* Turn debug mode on */
#define DEBUG_MORE() iDebug += 1	/* Increase the debug level */
#define DEBUG_LESS() iDebug -= 1	/* Decrease the debug level */
#define DEBUG_OFF() iDebug = 0		/* Turn debug mode off */
#define DEBUG_IS_ON() (iDebug > 0)	/* Check if the debug mode is enabled */
#define XDEBUG_ON() iDebug = 2		/* Turn extra debug mode on. Same as calling DEBUG_MORE() twice. */
#define XDEBUG_IS_ON() (iDebug > 1)	/* Check if the extra debug mode is enabled */

#define DEBUG_CODE(code) code	/* Code included in the _DEBUG version only */
#define DEBUG_CODE_IF_ON(code) DEBUG_CODE(if (DEBUG_IS_ON()) {code}) /*
				   Debug code executed if debug mode is on */
#define XDEBUG_CODE_IF_ON(code) DEBUG_CODE(if (XDEBUG_IS_ON()) {code}) /*
				   Debug code executed if extra debug mode is on */

extern int iIndent;		/* Debug messages indentation */
#define DEBUG_INDENT_STEP 2	/* How many spaces to add for each indentation level */
#define DEBUG_PRINT_INDENT() printf("%*s", iIndent, "")

/* Debug code, conditionally printing a string based on global variable 'debug' */
/* The enter and leave variants print, then respectively increase or decrease indentation,
   to make recursive calls easier to review. */
#define DEBUG_FPRINTF(args) DEBUG_DO(if (DEBUG_IS_ON()) {DEBUG_PRINT_INDENT(); fprintf args;})
#define DEBUG_PRINTF(args) DEBUG_DO(if (DEBUG_IS_ON()) {DEBUG_PRINT_INDENT(); printf args; fflush(stdout);})
#define XDEBUG_PRINTF(args) DEBUG_DO(if (XDEBUG_IS_ON()) {DEBUG_PRINT_INDENT(); printf args; fflush(stdout);})
#define DEBUG_ENTER(args)  DEBUG_DO(DEBUG_PRINTF(args); iIndent += DEBUG_INDENT_STEP;)
#define DEBUG_LEAVE(args)  DEBUG_DO(DEBUG_PRINTF(args); iIndent -= DEBUG_INDENT_STEP;)

#define DEBUG_RETURN_INT(i, comment) DEBUG_DO(int DEBUG_i = (i); \
  DEBUG_LEAVE(("return %d; // " comment "\n", DEBUG_i)); return DEBUG_i;)

/* print return instruction and decrease indent */
#define RETURN() DEBUG_DO(DEBUG_LEAVE(("return;\n")); return;)
#define RETURN_CONST(value) DEBUG_DO(DEBUG_LEAVE(("return %s;\n", #value)); return value;)
#define RETURN_INT(i) DEBUG_DO(int DEBUG_i = (i); \
  DEBUG_LEAVE(("return %d;\n", DEBUG_i)); return DEBUG_i;)
#define RETURN_STRING(s) DEBUG_DO(char *DEBUG_s = (s); \
  DEBUG_LEAVE(("return \"%s\";\n", DEBUG_s)); return DEBUG_s;)
#define RETURN_CHAR(c) DEBUG_DO(char DEBUG_c = (c); \
  DEBUG_LEAVE(("return '%c';\n", DEBUG_c)); return DEBUG_c;)
#define RETURN_BOOL(b) DEBUG_DO(int DEBUG_b = (b); \
  DEBUG_LEAVE(("return %s;\n", DEBUG_b ? "TRUE" : "FALSE")); return DEBUG_b;)

#define RETURN_COMMENT(args) DEBUG_DO(DEBUG_LEAVE(("return; // ")); \
  if (DEBUG_IS_ON()) printf args; return;)
#define RETURN_CONST_COMMENT(value, args) DEBUG_DO(DEBUG_LEAVE(("return %s; // ", #value)); \
  if (DEBUG_IS_ON()) printf args; return value;)
#define RETURN_INT_COMMENT(i, args) DEBUG_DO(int DEBUG_i = (i); \
  DEBUG_LEAVE(("return %d; // ", DEBUG_i)); if (DEBUG_IS_ON()) printf args; return DEBUG_i;)
#define RETURN_BOOL_COMMENT(b, args) DEBUG_DO(int DEBUG_b = (b); \
  DEBUG_LEAVE(("return %s; // ", DEBUG_b ? "TRUE" : "FALSE")); if (DEBUG_IS_ON()) printf args; return DEBUG_b;)

#else /* !defined(_DEBUG) */

#define DEBUG_VERSION ""	/* Non debug version: Simply don't say it */

#define DEBUG_GLOBALS

#define DEBUG_ON() (void)0
#define DEBUG_MORE() (void)0
#define DEBUG_LESS() (void)0
#define DEBUG_OFF() (void)0
#define DEBUG_IS_ON() 0
#define XDEBUG_IS_ON() 0
#define DEBUG_CODE(code) 	/* Code included in _DEBUG version only */
#define DEBUG_CODE_IF_ON(code)	/* Code included in _DEBUG version only */
#define XDEBUG_CODE_IF_ON(code)	/* Code included in _DEBUG version only */

#define DEBUG_PRINT_INDENT() DEBUG_DO_NOTHING() /* Print call-depth spaces */

#define DEBUG_FPRINTF(args) DEBUG_DO_NOTHING()  /* Print a debug string to a stream */
#define DEBUG_PRINTF(args) DEBUG_DO_NOTHING()   /* Print a debug string to stdout */
#define XDEBUG_PRINTF(args) DEBUG_DO_NOTHING()  /* Print an extra debug string to stdout */
#define DEBUG_ENTER(args)  DEBUG_DO_NOTHING()   /* Print and increase indent */
#define DEBUG_LEAVE(args)  DEBUG_DO_NOTHING()   /* Print and decrease indent */

#define DEBUG_RETURN_INT(i, comment) return(i)

/* print return instruction and decrease indent */
#define RETURN() return
#define RETURN_CONST(value) return(value)
#define RETURN_INT(i) return(i)
#define RETURN_STRING(s) return(s)
#define RETURN_CHAR(c) return(c)
#define RETURN_BOOL(b) return(b)

#define RETURN_COMMENT(args) return
#define RETURN_CONST_COMMENT(value, args) return(value)
#define RETURN_INT_COMMENT(i, args) return(i)
#define RETURN_BOOL_COMMENT(b, args) return(b)

#endif /* defined(_DEBUG) */

#define STRINGIZE(s) #s            /* Convert a macro name to a string */
#define VALUEIZE(s) STRINGIZE(s)   /* Convert a macro value to a string */
#define MACRODEF(s) "#define " #s " " STRINGIZE(s)

/* Display a macro name and value. */
#define DEBUG_PRINT_MACRO(name) DEBUG_DO( \
  const char *pszName = #name; /* Don't use STRINGIZE because we're already inside a macro */ \
  const char *pszValue = STRINGIZE(name); /* Don't use VALUEIZE because we're already inside a macro */ \
  DEBUG_PRINT_INDENT(); \
  if (strcmp(pszName, pszValue)) { \
    printf("#define %s %s\n", pszName, pszValue); \
  } else { /* Not 100% certain, but most likely. */ \
    printf("#undef %s\n", pszName); \
  } \
)

#ifdef _WIN32

/* Helper macros for displaying Unicode strings */
#define DEBUG_WSTR2UTF8(from, to, toSize) DEBUG_CODE( \
  WideCharToMultiByte(CP_UTF8, 0, from, lstrlenW(from)+1, to, toSize, NULL, NULL); \
)

/* Dynamically allocate a new buffer, then convert a Unicode string to UTF-8 */
/* The dynamic allocation is useful in modules using lots of UTF-16 pathnames.
   This avoids having many local buffers of length UTF8_PATH_MAX, which may
   make the stack grow too large and overflow. */
#define DEBUG_WSTR2NEWUTF8(pwStr, pUtf8)	\
  DEBUG_CODE(					\
    do {					\
      int nUtf8 = (int)lstrlenW(pwStr) * 2 + 1;	\
      pUtf8 = malloc(nUtf8);			\
      DEBUG_WSTR2UTF8(pwStr, pUtf8, nUtf8);	\
    } while (0);				\
  ) /* DEBUG_FREE(pUtf8) MUST be used to free the UTF-8 string after use, else there will be a memory leak */

#define DEBUG_FREEUTF8(pUtf8) DEBUG_CODE(free(pUtf8))

#endif /* defined(_WIN32) */

#endif /* !defined(_DEBUGM_H) */

