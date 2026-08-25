#pragma once

#include <string>
#include <cstring>
#include <memory>

// -----------------------------------------------------------------------------
// SecureString
//
// Purpose:
//   String wrapper that zeros its memory on destruction and provides clear()
//   that actually overwrites the buffer. Intended for sensitive data like
//   passwords and authentication tokens.
//
// Usage:
//   SecureString password("hunter2");
//   // ...use password.c_str() or password.str()...
//   // Destructor or explicit clear() zeros the memory before deallocation
//
// Limitations:
//   - Does not prevent copies (std::string copies still exist in memory)
//   - Does not prevent access from debuggers or memory forensics
//   - C++17+ only (uses std::allocator)
//
// Best practices:
//   - Use std::move when passing between functions
//   - Call clear() immediately after use if the string will persist
//   - Never log or print SecureString contents
// -----------------------------------------------------------------------------
class SecureString
{
public:
    SecureString() = default;

    SecureString(const std::string& str)
    {
        assign(str);
    }

    SecureString(const char* str)
    {
        if (str)
            assign(std::string(str));
    }

    SecureString(const SecureString& other)
    {
        assign(other.m_data);
    }

    SecureString(SecureString&& other) noexcept
    {
        m_data = std::move(other.m_data);
        other.clear();
    }

    ~SecureString()
    {
        clear();
    }

    SecureString& operator=(const SecureString& other)
    {
        if (this != &other)
        {
            assign(other.m_data);
        }
        return *this;
    }

    SecureString& operator=(SecureString&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            m_data = std::move(other.m_data);
            other.clear();
        }
        return *this;
    }

    void assign(const std::string& str)
    {
        clear();
        m_data = str;
    }

    // Zeros the string and deallocates memory
    void clear()
    {
        if (!m_data.empty())
        {
            // Overwrite all bytes with zeros before deallocation
            std::fill(m_data.begin(), m_data.end(), '\0');
            std::string().swap(m_data);  // Deallocate
        }
    }

    const std::string& str() const { return m_data; }
    const char* c_str() const { return m_data.c_str(); }
    size_t size() const { return m_data.size(); }
    size_t length() const { return m_data.length(); }
    bool empty() const { return m_data.empty(); }

    bool operator==(const SecureString& other) const
    {
        return m_data == other.m_data;
    }

    bool operator==(const std::string& other) const
    {
        return m_data == other;
    }

private:
    std::string m_data;
};
