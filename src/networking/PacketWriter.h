#ifndef PACKET_WRITER_H
#define PACKET_WRITER_H

#include "PacketBuffer.h"
#include <string>
#include <stdexcept>

class PacketWriter {
public:
    explicit PacketWriter(PacketBuffer& buffer) : m_buffer(buffer) {}

    // Clear the underlying buffer
    void clear() { m_buffer.clear(); }

    // Get the underlying buffer (for sending)
    PacketBuffer& buffer() { return m_buffer; }
    const PacketBuffer& buffer() const { return m_buffer; }

    // Raw write
    void write(const void* data, size_t size) {
        m_buffer.write(data, size);
    }

    // Template for writing POD types
    template<typename T>
    typename std::enable_if<std::is_trivial<T>::value, void>::type
    write(const T& value) {
        m_buffer.write(value);
    }

    // Write a string (length prefix uint32_t)
    void writeString(const std::string& str) {
        m_buffer.writeString(str);
    }

    // Write a boolean (as uint8_t)
    void writeBool(bool value) {
        write(static_cast<uint8_t>(value ? 1 : 0));
    }

    // Write an 8-bit integer
    void writeInt8(int8_t value) { write<int8_t>(value); }
    void writeUInt8(uint8_t value) { write<uint8_t>(value); }

    // Write a 16-bit integer
    void writeInt16(int16_t value) { write<int16_t>(value); }
    void writeUInt16(uint16_t value) { write<uint16_t>(value); }

    // Write a 32-bit integer
    void writeInt32(int32_t value) { write<int32_t>(value); }
    void writeUInt32(uint32_t value) { write<uint32_t>(value); }

    // Write a 64-bit integer
    void writeInt64(int64_t value) { write<int64_t>(value); }
    void writeUInt64(uint64_t value) { write<uint64_t>(value); }

    // Write a float
    void writeFloat(float value) { write<float>(value); }

    // Write a double
    void writeDouble(double value) { write<double>(value); }

private:
    PacketBuffer& m_buffer;
};

#endif // PACKET_WRITER_H
