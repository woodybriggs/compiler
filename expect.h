#include <stdlib.h>
#include <vector>

#ifndef EXPECT_H
#define EXPECT_H

void Expect(bool assertion, const char * format_string, ...) {
    if (!assertion) {
        va_list args;
        va_start(args, format_string);
        vfprintf(stderr, format_string, args);
        va_end(args);
        exit(SIGTERM);
    }
}

#endif