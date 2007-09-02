#ifndef _GCC_WIN_STRINGS
#define _GCC_WIN_STRINGS

#if (defined(__GNUC__)  || defined(__GCCXML__))

#define _stricmp strcasecmp
#define _strnicmp strncasecmp

#endif
#endif
