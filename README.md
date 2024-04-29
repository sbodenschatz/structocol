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

## Serialization
The members of these message structs (aggregate types) are iterated using [Boost.PFR](https://www.boost.org/libs/pfr).
Primitives are serialized into suitable binary forms and complex types like containers are serialized by recursively serializing their elements and required descriptors (e.g. number of elements).

The following types are supported for serialization:
- `std::uint8_t`, `std::int8_t`, `char`, `char8_t`, `bool`: Serialized as single bytes
- `uint16_t`, `int16_t`, `char16_t`: Serialized as two bytes in big endian
- `uint32_t`, `int32_t`, `char32_t`: Serialized as four bytes in big endian
- `uint64_t`, `int64_t`: Serialized as eight bytes in big endian
- `float`, `double`: Serialized in big endian format, as 4 and 8 bytes respectively
- `varint_t`: A wrapper around a `std::size_t`, that causes it to be serialized as the required number of bytes, each holding a 7-bit block and the highest bit indicating if another block is following
- Dynamically sized containers as provided by the standard library, e.g. `std::vector<T>`, `std::deque<T>`, `std::map<K, V>`, `std::set<T>`, `std::multimap<K, V>`, `std::multiset<T>`:
	Serialized as their number of elements (as `varint_t`) followed by the elements
- `std::basic_string<T>` except `std::wstring`: Serialized like a dynamically sized container of characters, `std::wstring`'s encoding is too implementation-defined to use for serialization
- `std::pair<FT, ST>`: Serialized as `FT` followed by `ST`
- `std::tuple<T...>`: Serialized as concatenation of the serialized representation of the `T`s
- `std::variant<T...>`: Serialized as active index (as smallest statically sufficient of `std::uint8_t`, `std::uint16_t`, `std::uint32_t`, `std::uint64_t`) followed by the active object
- `std::optional<T>`: Serialized as a boolean byte for whether the object is active, followed by the object if it is active
- `std::monostate`: Serialized as nothing, i.e. a byte sequence of length 0
- `T[N]`, `std::array<T, N>`: Serialized as the N `T` objects (fixed length)
- `std::bitset<N>`: Serialized as the contained bits, lowest indices (LSB) first
- `magic_number<values...>`: A wrapper statically holding a sequence numbers (or characters), serialized by writing the values,
	the special behavior of this is that the deserialization checks that the expected values were read and if not an error is thrown
	(usefull for format or protocol header signatures)

## Buffers

Serialization and deserialization uses buffers to store / read the serialized representations.
Multiple buffer implementations are provided, with the one to use being passed using static polymorphism, i.e. by template-based duck-typing.
The buffer implementations provided by this library are
- `vector_buffer`: A memory buffer based on `std::vector<std::byte>`
- `istream_buffer` and `ostream_buffer`: A buffer implementation that operates on `std::istream` and `std::ostream` respectively
- `stdio_buffer`: A buffer implementation that operates on a `std::FILE*` C-style file handle

The main buffer implementation is `vector_buffer`, with the other two being mostly relevant for (de-)serializing directly to / from files.
If compiled with optional Boost.ASIO support, it provides integrations for being passed to (async) IO operations as an input or output buffer.

## Buffer Pools
The template classes `buffers_ring` and `recycling_buffers_queue` provide functionality to hold a pool of reusable buffer objects and differ by reuse order.
`buffers_ring` operates in FIFO order, `recycling_buffers_queue` reuses the most recently returned buffers first (i.e. LIFO).

## Protocol Handler
TODO

## Multiplexing
TODO
