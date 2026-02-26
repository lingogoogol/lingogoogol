#ifndef LIB_H
#define LIB_H

//#define DEBUG
#define GLSL_VERSION "460"
#define OPENGL_VERSION_MAJOR 4
#define OPENGL_VERSION_MINOR 6

#include <Library/lib.h>

#ifndef DEBUG
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

#endif