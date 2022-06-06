#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#define READLINE_BUFFER 255

// MACRO TEMPORARIO PARA DEBUG
#define UNUSED(x) (void)x;

#include <stdio.h>
#include <string>

typedef char byte;

std::string readLineCpp(FILE *stream);

char *readLine(FILE *stream);

#endif
