/*
 * Structocol project
 *
 * Copyright 2020
 */

#ifndef STRUCTOCOL_EXCEPTIONS_INCLUDED
#define STRUCTOCOL_EXCEPTIONS_INCLUDED

#include <stdexcept>

namespace structocol {

class length_error : public std::length_error {
public:
	using std::length_error::length_error;
};

class message_length_overflow : public length_error {
	using length_error::length_error;
};

class buffer_length_error : public length_error {
	using length_error::length_error;
};

class runtime_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

class io_error : public runtime_error {
	using runtime_error::runtime_error;
};

class deserialization_data_error : public runtime_error {
	using runtime_error::runtime_error;
};

} // namespace structocol

#endif // STRUCTOCOL_EXCEPTIONS_INCLUDED
