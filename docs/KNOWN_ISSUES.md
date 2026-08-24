# StrixVerse Client - Known Issues

## Gameplay Issues
### Lost Tech Panels (Vault/Gate/Stabilizer/MemoryCrystal)
- UI is ready but server ids 29-32 are placeholders
- Button callbacks currently log not-implemented; blocked on server-side Lost Tech packet senders

### Buff Display
- Renders empty until the server sends buff packets (server-side implementation pending)

### WorldBrowser
- World rows have no hover highlight; requires UIPanel hover support to implement

## Audio Issues
### AudioManager
- dirtBreak/join/exit SFX are still .mp3 and need WAV conversion before they can play
- Gameplay music track world_theme.ogg is missing from Assets (menu music works)

## Networking Issues
### Connection
- Plaintext TCP - no encryption/TLS support for secure connections
- No automatic reconnection with exponential backoff
- Auth token is not persisted across sessions; users re-authenticate on every launch
- Offline mode accepts any password of 8+ characters without server validation (intentional fallback, documented)

### Server-Side Gaps (client features waiting on these)
- Buff packets not sent
- WorldInfo metadata (owner/type/capacity/favourites) missing, so world browser rows show limited info
- Per-player roles absent from roster data
- Chunk totals missing from WorldState, so LoadingScreen falls back to indeterminate progress mode
- Lost Tech packet senders not implemented
- Notification opcode 112 has no server implementation (client handler is live)

## Graphics Systems Issues
### Font
- Per-glyph texture approach works but is inefficient; needs texture atlas batching

### Character Rendering
- Player figures are animated procedural placeholders; spritesheet art not yet authored (swap point marked in CharacterRenderSystem.cpp)

### SpriteBatch
- Capacity fixed at 16384 entries (resolved zoom-out tile drops); no atlas sub-region support yet

### Vertex/Mesh/FrameBuffer/RenderTarget
- All are stub files with no implementation; not required by the current rendering path

## Build System Issues
### CMakeLists.txt
- Could benefit from additional build options (debug/release toggles, sanitizers)
- No installation rules or packaging support
- Limited testing infrastructure (no unit test configuration)

## Version-Specific Issues
- Uses std::format which requires C++20 - ensure compiler support
- Some OpenGL calls may not be available on older graphics drivers
- Windows-specific networking code (Winsock) limits portability

## Resolved Issues (previously tracked)
- Animation system was an empty stub; Animator/AnimationClip now implemented and driving player figures
- Audio system was unimplemented; AudioManager now handles music and SFX with volume control
- SpriteBatch tile drop-out when zoomed out; fixed via capacity increase to 16384
- Zero gameplay logic; movement, collision, building, chat, interaction, and remote-player interpolation all live
- Missing world/UI/player systems; full screen flow, HUD panels, and world loading implemented
