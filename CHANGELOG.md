# Changelog

All notable changes to the StrixVerse Client project will be documented in this file.

## [Unreleased]
### Added
- Complete Entity Component System (ECS) implementation
  - EntityManager: Handles entity creation, destruction, and ID recycling with generational indices
  - ComponentManager: Dense storage for components with bitset-based entity signatures
  - SystemManager: Registers and manages systems, maintains entity lists based on component signatures
  - Core Components: Transform, Sprite, Animation, Velocity, Collider, Network, Player, Input, Health
  - Core Systems: Movement, Render, Input, NetworkSync
- ECS integration with Engine class for initialization, updates, and shutdown
- ECS integration with ServiceLocator for manager access

### Added
- Comprehensive codebase audit completed (July 2026)
- Detailed architecture documentation
- Project status tracking document
- Feature roadmap and TODO list

### Changed
- Code organization and structure clarified through documentation
- Build system dependencies documented

### Fixed
- No code changes made during audit phase

## [0.1.0] - 2026-07-26
### Initial Code Audit

This release represents the initial comprehensive audit of the StrixVerse client codebase, documenting the current state of all systems and providing a roadmap for future development.

### Core Systems
- **Application Framework**: Basic lifecycle management implemented
- **Engine**: Main loop with fixed/variable timestep established
- **Game**: Scene management framework in place
- **Window System**: Complete SDL3/OpenGL 4.6 context and event handling
- **Configuration**: JSON-like parser with validation and persistence
- **Logging**: Thread-safe system with file/console output and levels
- **Timing**: High-resolution with FPS calculation and fixed timestep support
- **Scheduling**: One-shot and repeating job system with thread safety
- **ThreadPool**: Work-stealing thread pool implementation
- **Asset Management**: Texture and shader caching with reference counting
- **Service Location**: Dependency injection pattern implementation
- **Versioning**: Build-time version stamping via CMake

### Graphics Systems
- **Rendering Pipeline**: Basic OpenGL state management and frame buffering
- **Camera System**: Complete 2D camera with world/screen coordinate conversion
- **Sprite Batching**: Efficient texture-atlas-aware sprite rendering
- **Texture Handling**: Image loading via stb_image with mipmap/SRGB support
- **Shader System**: GLSL compilation from files with uniform setting
- **Color Utilities**: Comprehensive color manipulation and predefined constants

### Networking Systems
- **Transport Layer**: Robust TCP connection with Winsock and threading
- **Message System**: Complete packet serialization/deserialization with factory pattern
- **Connection Management**: Keep-alive and ping/pong latency monitoring
- **Reliability Features**: Thread-safe send/receive queues with condition variables
- **Statistics**: Bandwidth and packet tracking for performance monitoring

### Build System
- **CMake Configuration**: Complete setup for SDL3, Freetype, GLM, Glad, Threads
- **C++ Standard**: Enforces C++20 with required features
- **Output Organization**: Configurable build output directories
- **Resource Generation**: Windows resource file integration

### Known Limitations (Post-Audit)
1. **Zero Gameplay Logic**: No actual game mechanics implemented beyond networking
2. **Missing Core Systems**: No world, entity, player, UI, audio, or gameplay systems
3. **Stub Files**: Several graphics components exist only as empty/unimplemented stubs
4. **Placeholder Systems**: Some systems have minimal implementations needing expansion
5. **Feature Gaps**: Advanced features missing from otherwise complete systems

### Next Development Phase
Primary focus should be on implementing the Entity Component System and World/Tilemap systems to establish the foundation for gameplay mechanics, building upon the solid networking and resource management foundations already in place.