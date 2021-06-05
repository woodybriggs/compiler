#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "types.h"

#ifndef ARRAY_HH
#define ARRAY_HH

template<class T>
struct Array {
    T * base;
    T * next_free;
    T * end;
    uint64 size;
    uint64 count;
    bool initialised;

    T * operator[](uint64 index) {
        return (base + (sizeof(T) * index));
    }
};

template <class T>
Array<T> CreateArray(uint32 inital_size_in_bytes) {
    Array<T> array = {};
    array.base = (T *) malloc(inital_size_in_bytes);
    if (array.base) {
        memset(array.base, 0, inital_size_in_bytes);
        array.next_free = array.base;
        array.end = array.base + inital_size_in_bytes;
        array.size = inital_size_in_bytes;
        array.initialised = true;
    } else {
        array.initialised = false;
    }
    return array;
}

template<class T>
void DestoryArray(Array<T> *array) {
    free(array->base);
}

void ByteCopy(byte * destination, byte * source, uint32 length) {
    for (uint32 i = 0; i < length; i++) {
        destination[i] = source[i];
    }
}

byte * AllocAndCopyBytes(Array<byte>*array, byte * string, uint64 length) {
    byte * result = 0;
    if (((array->next_free + length) < array->end)) {
        result = array->next_free;
        array->next_free += length;
        array->count += length;
        ByteCopy(result, string, length);
    } else {
        assert("No Memory Left In array\n");
    }
    return result;
}

template<class T>
T * Alloc(Array<T>* array) {
    T * result = 0;
    if (((array->next_free + sizeof(T)) < array->end)) {
        result = array->next_free;
        array->next_free = array->next_free + sizeof(T);
        array->count += 1;
    } else {
        assert("No Memory Left In array\n");
    }
    return result;
}

template<class T>
void AppendItem(Array<T> *array, T item) {
    T * i = Alloc<T>(array);
    *i = item;
}

template<class T>
T * AppendItemReturnPtr(Array<T> *array, T item) {
    T * i = Alloc<T>(array);
    *i = item;
    return i;
}

template<class T>
T AppendItemReturnItem(Array<T> *array, T item) {
    T * i = Alloc<T>(array);
    *i = item;
    return *i;
}

template<class T>
uint64 AppendItemReturnIndex(Array<T> *array, T item) {
    T * i = Alloc<T>(array);
    *i = item;
    return array->count-1;
}



byte * AllocBytes(Array<byte>* array, uint32 length) {
    byte * result = 0;
    if (((array->next_free + length) < array->end)) {
        result = array->next_free;
        array->next_free = array->next_free + length;
        array->count += 1;
    } else {
        assert("No Memory Left In array\n");
    }
    return result;
}

#endif /* array.hh */