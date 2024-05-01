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
Both buffer pools alos manage the active buffers in a queue which can be used as a message queueing mechanism, i.e.
putting each message into a buffer in the queue and having a task that sends them to the network after the previous one has completed transmission.

## Protocol Handler
The library also provides a layer of so-called protocol handlers in the form of the template class `protocol_handler<Msgs...>`.
It is instantiated with a list of possible message classes and manages encoding and decoding the types of serialized message objects.
That is, with just the serialization layer, the deserializing code needs to know which type to expect for decoding,
as the serialization layer only encodes the state of the object but not its type.
Note that encoding an arbitrary type without known context would be very hard to do in C++ as there is no real reflection mechanism available yet.
With the protocol handler, the list of possible types is known and the type can be encoded as an index into the list.
Based on this, one should note that **the list of message types *must* match on both sides of the serialization and so must the structure of the messages**.
For simplicity and efficiency of both, the code and the encoding, **structocol currently does not implement a compatibility layer that allows adding or removing fields or messages**, like e.g. the one that protocol buffers provides.

For the serializing side, `protocol_handler<Msgs...>` provides `encode_message` that serializes the type index followed by the serialized state of the object for a given message object, and `calculate_message_size` that can calculate in advance how many bytes `encode_message` would produce.
For the deserializing side, it provides `decode_message` which decode the message and returns it wrapped in a `std::variant<Msgs...>`,
and `process_message` with takes a callable object that must be callable with all message type known by the `protocol_handler` and calls the appropriate overload with the decoded message.

## Multiplexing
While for datagram-based protocols, reading all messages out of a datagram buffer (using a protocol handler on top of the serialization mechanism) until the buffer is consumed is often sufficient, stream-based protocols that don't want to block in the middle of deserialization for data to arrive need a way of delimiting messages in the read input,
to know when a message has been fully received and can be deserialized.
A practical way of doing this is prefixing every message with its own length in bytes.
That amount is then first read into a buffer before dispatching that buffer to deserialization when it is complete.
To implement this on top of Boost.ASIO's asynchronous IO operations, the [`multiplexing.hpp` header](include/structocol/multiplexing.hpp) provides `encode_message_multiplexed` for sending and `async_read_multiplexed` and `async_process_multiplexed` for receiving.
Here, `async_read_multiplexed` only takes care of reading one message worth of data into a given buffer and invoking the completion handler when the data are available in the buffer.
`async_process_multiplexed` builds on top of this, and maps the received message buffer through a given `protocol_handler`'s `process_message` member function, 
so that the given handler is invoked with the decoded message object or a `boost::system::error_code` if ASIO reported an error from the receiving operation.
