# Security Considerations

This document outlines known security practices and limitations in the StrixVerse Client.

## Authentication & Credentials

### Current Implementation
- Username and password are transmitted to the server over TCP
- The connection is **not encrypted** (no TLS/SSL)
- Passwords are cleared from memory after transmission

### Known Risks
⚠️ **PLAINTEXT PASSWORD TRANSMISSION**
- On untrusted networks (public WiFi, shared networks), credentials can be intercepted
- Network sniffers or MITM attacks can capture login packets
- This is a **critical security issue** for production deployment

### Recommendations
1. **Enable TLS/SSL immediately** for production deployments
   - Wrap the socket with OpenSSL or a modern TLS library
   - Use certificate pinning to prevent MITM attacks
   - Enforce minimum TLS version (1.2+)

2. **For development/testing:**
   - Use localhost only
   - Never expose the game on untrusted networks
   - Use a VPN when testing over public networks

3. **Future enhancements:**
   - Consider client-side password hashing before transmission (defense in depth)
   - Implement session token refresh mechanisms
   - Add certificate pinning for production

## Password Storage in Memory

### Current Implementation
- Passwords stored as `std::string` during login flow
- Cleared after transmission via `pendingPassword_.clear()`
- Session tokens stored in `NetworkManager::m_sessionToken`

### Improvements Made (v1.1+)
- `SecureString` class available for sensitive data
- Zeros memory on destruction, not just in `clear()`
- Use `SecureString` instead of `std::string` for passwords and tokens

### Usage Example
```cpp
// Old (not secure):
std::string password = passwordBox_->getText();
auth->BeginLogin(username, password);
// password may remain in memory

// New (better):
SecureString password(passwordBox_->getText());
auth->BeginLogin(username, password.str());
// password is zeroed when it goes out of scope
```

## Data Protection in Transit

### What's Protected
- None - all traffic is unencrypted

### What's Not Protected
- Username during login/register
- Password during login
- Email during registration
- Session token (after login)
- Chat messages
- World data and terrain

## Data at Rest

### Config File
- Located at `configs/client.json`
- Contains server address, window size, audio settings
- **No sensitive data stored here** ✓

### Save Data
- Located in `saves/` directory
- Local character progression, world state
- No encryption applied (intended)

### Remembered Username
- Located in `saves/.strixverse_login`
- Contains username only (password never saved) ✓

## Single Instance Lock

### Windows Named Mutex
- File: `src/main.cpp`
- Prevents multiple game instances per user session
- Intentional resource leak (handle never closed)
- Scoped to current user session via `Local\` prefix

### Risk Assessment: LOW
- No security implications
- OS reclaims handle on process exit

## Thread Safety

### Receive Buffer
- **Fixed (v1.1+):** Protected by `m_receiveBufferMutex`
- Game thread and socket thread can safely access
- Prevents data races and corruption

### Other Buffers
- Packet send queue: Protected by `m_sendMutex` ✓
- Packet receive queue: Protected by `m_receiveQueueMutex` ✓
- ServiceLocator: Protected by mutex ✓

## Component Memory Management

### ECS Storage
- **Fixed (v1.1+):** Proper alignment for placement new
- Destructors explicitly called on component removal
- Prevents undefined behavior and memory corruption

## Known Limitations

1. **No input validation** on all user-provided strings
   - Malformed packets could cause issues
   - Server-side validation is the primary defense

2. **No rate limiting** on client
   - Server should enforce
   - Clients could spam packets (not critical)

3. **No anti-cheat measures**
   - Client is untrusted code
   - Server must validate all actions

4. **Debug symbols in release builds**
   - May expose internal structure to attackers
   - Consider stripping for production

## Incident Reporting

If you discover a security vulnerability:
1. **Do NOT** post it publicly
2. Contact the development team directly
3. Provide detailed steps to reproduce
4. Allow 90 days for a fix before disclosure

## Compliance Notes

- Not HIPAA compliant (medical data)
- Not PCI-DSS compliant (credit cards)
- Not GDPR compliant (user data)
- Use only for non-sensitive multiplayer gaming

---

**Last Updated:** 2026-08-25  
**Status:** Known issues documented, critical fixes applied in v1.1
