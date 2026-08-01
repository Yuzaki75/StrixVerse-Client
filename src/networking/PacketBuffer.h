#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H

#include <vector>
#include <cstring>
#include <stdexcept>
#include <string>

class PacketBuffer {
public:
    PacketBuffer() : m_buffer(), m_readPos(0) {}
    explicit PacketBuffer(size_t reserveSize) : m_buffer(), m_readPos(0) {
        m_buffer.reserve(reserveSize);
    }

    // Maximum string length allowed in packets (to prevent buffer overflow attacks)
    static const uint32_t MAX_STRING_LENGTH = 4096;

    // Clear the buffer and reset read position
    void clear() {
        m_buffer.clear();
        m_readPos = 0;
    }

    // Get the underlying buffer data
    const char* data() const { return m_buffer.data(); }
    char* data() { return m_buffer.data(); }

    // Get the current size of the buffer
    size_t size() const { return m_buffer.size(); }

    // Get the remaining bytes to read
    size_t remaining() const { return m_buffer.size() - m_readPos; }

    // Reset read position to start
    void resetReadPos() { m_readPos = 0; }

    // Reserve space in the buffer
    void reserve(size_t size) { m_buffer.reserve(size); }

    // Write raw data
    void write(const void* data, size_t size) {
        if (!data && size > 0) {
            throw std::invalid_argument("Cannot write null pointer with positive size");
        }
        m_buffer.insert(m_buffer.end(), static_cast<const char*>(data), static_cast<const char*>(data) + size);
    }

    // Read raw data
    void read(void* data, size_t size) {
        if (!data && size > 0) {
            throw std::invalid_argument("Cannot read into null pointer with positive size");
        }
        if (m_readPos + size > m_buffer.size()) {
            throw std::out_of_range("Not enough bytes in buffer to read");
        }
        std::memcpy(data, m_buffer.data() + m_readPos, size);
        m_readPos += size;
    }

    // Template for writing POD types
    template<typename T>
    typename std::enable_if<std::is_trivial<T>::value, void>::type
    write(const T& value) {
        write(&value, sizeof(T));
    }

    // Template for reading POD types
    template<typename T>
    typename std::enable_if<std::is_trivial<T>::value, T>::type
    read() {
        if (m_readPos + sizeof(T) > m_buffer.size()) {
            throw std::out_of_range("Not enough bytes in buffer to read");
        }
        T value;
        std::memcpy(&value, m_buffer.data() + m_readPos, sizeof(T));
        m_readPos += sizeof(T);
        return value;
    }

    // Write a string with length prefix (uint32_t)
    void writeString(const std::string& str) {
        uint32_t length = static_cast<uint32_t>(str.size());
        if (length > MAX_STRING_LENGTH) {
            throw std::length_error("String too long for packet (max " + std::to_string(MAX_STRING_LENGTH) + " bytes)");
        }
        write(length);
        if (length > 0) {
            write(str.data(), length);
        }
    }

    // Read a string with length prefix (uint32_t)
    std::string readString() {
        uint32_t length = read<uint32_t>();
        if (length > MAX_STRING_LENGTH) {
            throw std::runtime_error("String length too long (max " + std::to_string(MAX_STRING_LENGTH) + " bytes)");
        }
        if (m_readPos + length > m_buffer.size()) {
            throw std::out_of_range("Not enough bytes in buffer to read string");
        }
        std::string result(m_buffer.data() + m_readPos, length);
        m_readPos += length;
        return result;
    }

private:
    std::vector<char> m_buffer;
    size_t m_readPos;
};

#endif // PACKET_BUFFER_H
