#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <exception>

class exception_with_buffer : public std::exception {
   public:
    // default constructor
    exception_with_buffer() : _buffer(new char[_buffer_size]) {}

    // destructor
    ~exception_with_buffer() {
        if (_buffer) {
            delete[] _buffer;
            _buffer = nullptr;
        }
    }

    // copy constructor
    exception_with_buffer(const exception_with_buffer& other) : exception_with_buffer() {
        write_to_buffer(other._buffer);
    }

    // copy assignment
    exception_with_buffer& operator=(const exception_with_buffer& other) {
        write_to_buffer(other._buffer);

        return *this;
    }

    // move constructor and assignment
    exception_with_buffer(exception_with_buffer&& other);
    exception_with_buffer& operator=(exception_with_buffer&& other);

    const char* what() const noexcept override {
        return _buffer;
    }

   protected:
    void write_to_buffer(const char* buffer);

   private:
    static const size_t _buffer_size = 65536;
    char* _buffer;
};

#endif  // COMMON_HPP_
