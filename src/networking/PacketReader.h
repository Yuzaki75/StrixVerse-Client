#ifndef PACKET_READER_H
#define PACKET_READER_H

#include <string>
#include <stdexcept>
#include "PacketBuffer.h"

class PacketReader {
public:
    explicit PacketReader(PacketBuffer& buffer) : m_buffer(buffer) {}

    // Read raw data
    void read(void* data, size_t size) {
        m_buffer.read(data, size);
    }

    // Template for reading POD types
    template<typename T>
    typename std::enable_if<std::is_trivial<T>::value, T>::type
    read() {
        return m_buffer.read<T>();
    }

    // Read a string (length prefix uint32_t)
    std::string readString() {
        return m_buffer.readString();
    }

    // Read a boolean (as uint8_t)
    bool readBool() {
        return static_cast<bool>(read<uint8_t>());
    }

    // Read an 8-bit integer
    int8_t readInt8() { return read<int8_t>(); }
    uint8_t readUInt8() { return read<uint8_t>(); }

    // Read a 16-bit integer
    int16_t readInt16() { return read<int16_t>(); }
    uint16_t readUInt16() { return read<uint16_t>(); }

    // Read a 32-bit integer
    int32_t readInt32() { return read<int32_t>(); }
    uint32_t readUInt32() { return read<uint32_t>(); }

    // Read a 64-bit integer
    int64_t readInt64() { return read<int64_t>(); }
    uint64_t readUInt64() { return read<uint64_t>(); }

    // Read a float
    float readFloat() { return read<float>(); }

    // Read a double
    double readDouble() { return read<double>(); }

private:
    PacketBuffer& m_buffer;
};

#endif // PACKET_READER_H
