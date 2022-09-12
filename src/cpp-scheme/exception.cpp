#include "exception.hpp"

#include <cstring>

exception_with_buffer::exception_with_buffer(exception_with_buffer&& other) : exception_with_buffer() {
    if (_buffer != nullptr) {
        delete[] _buffer;
    }

    _buffer = other._buffer;
    other._buffer = nullptr;
}

exception_with_buffer& exception_with_buffer::operator=(exception_with_buffer&& other) {
    if (_buffer != nullptr) {
        delete[] _buffer;
    }

    _buffer = other._buffer;
    other._buffer = nullptr;

    return *this;
}

void exception_with_buffer::write_to_buffer(const char* buffer) {
    if (buffer != nullptr) {
        if (_buffer == nullptr) {
            // in case of a moved-out object
            _buffer = new char[_buffer_size];
        }

        strncpy(_buffer, buffer, _buffer_size);
    }
}
