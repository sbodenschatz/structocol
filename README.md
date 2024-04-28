# structocol

A simple serialization and protocol library.
The name is a contraction of struct and protocol, because the intention is that protocol messages can be easily defined as C++ structs.
```C++
struct test_a {
	std::int16_t x;
	std::uint64_t y;
	std::int8_t z;
};

struct test_b {
	std::int64_t x;
	test_a y;
};

struct test_c {
	std::vector<test_a> vals;
	std::string text;
}
```

The members of these message structs (aggregate types) are iterated using [Boost.PFR](https://www.boost.org/libs/pfr).
Primitives are serialized into suitable binary forms and complex types like containers are serialized by recursively serializing their elements.

The following types are supported for serialization:
- `std::uint8_t`, `std::int8_t`, `char`, `char8_t`, `bool`: Serialized as single bytes
- `uint16_t`, `int16_t`, `char16_t`: Serialized as two bytes in big endian
- `uint32_t`, `int32_t`, `char32_t`: Serialized as four bytes in big endian
- `uint64_t`, `int64_t`: Serialized as eight bytes in big endian
- `float`, `double`: Serialized in big endian format, as 4 and 8 bytes respectively
- Dynamically sized containers as provided by the standard library, e.g. `std::vector<T>`, `std::deque<T>`, `std::map<K, V>`, `std::set<T>`, `std::multimap<K, V>`, `std::multiset<T>`:
	Serialized as their number of elements followed by the elements
- `std::basic_string<T>` except `std::wstring`: Like a dynamic sized container of characters. `std::wstring`'s encoding is too implementation-defined to use for serialization
- `std::pair<FT, ST>`: Serialized as `FT` followed by `ST`
- `std::tuple<T...>`: Serialized as concatenation of the serialized representation of the `T`s
- `std::variant<T...>`: Serialized as active index (as smallest sufficient `std::uint8_t`, `std::uint16_t`, `std::uint32_t`, `std::uint64_t`) followed by the active object
- `std::optional<T>`: Serialized as a boolean byte for whether the object is active, followed by the object if it is active
- `varint_t`: A wrapper around a `std::size_t`, that causes it to be serialized as the required number of bytes, each holding a 7-bit block and the highest bit indicating if another block is following
- `std::monostate`: Serialized as nothing, i.e. a byte sequence of length 0
- `T[N]`, `std::array<T, N>`: Serialized as the N `T` objects (fixed length)
- `std::bitset<N>`: Serialized as the contained bits, lowest indices (LSB) first
- `magic_number<values...>`: A wrapper statically holding a sequence numbers (or characters), serialized by writing the values,
	the special behavior of this is that the deserialization checks that the expected values were read and if not an error is thrown
	(usefull for format or protocol header signatures)



