#include <stddef.h>

#ifndef SIMPLE_STRING_H
#define SIMPLE_STRING_H

size_t strlen(const char * str);

struct String
{
    char * data;
    unsigned long long length;

    String(const char * string) {
        length = strlen(string);
        data = (char *) string;
    }
    
    String(char * _data, unsigned long long _length) {
        length = _length;
        data = _data;
    }
    
    String() {
        data = 0;
        length = 0;
    }


    bool operator==(const char * comparison_string) {
        bool result = true;

        unsigned long long comparison_string_length = strlen(comparison_string);
        if (length != comparison_string_length) {
            result = false;
        }
        else {
            unsigned long long c = 0;
            while (c < length) {
                if (data[c] != comparison_string[c]) result = false;
                c += 1;
            }
        }
        return result;
    }
};

#endif
