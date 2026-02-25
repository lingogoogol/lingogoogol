#ifndef DEBUG_F_H
#define DEBUG_F_H

#include <string>

#ifdef DEBUG
#define handle_error(...) handle_error1(__FILE__, __LINE__, __VA_ARGS__)
#define check_GL_error(...) check_GL_error1(__FILE__, __LINE__, __VA_ARGS__)
#endif

#ifndef DEBUG
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#define handle_error(...)
#define check_GL_error(...)
#endif

void handle_error1(const char* file, int line, std::u32string message);
void check_GL_error1(const char* file, int line);

#endif