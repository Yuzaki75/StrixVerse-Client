# Changelog

All notable changes to the StrixVerse Client project will be documented in this file.

## [Unreleased]

### Added (v0.2.0 Audit Fixes)
- LoadingScreen.cpp implementation with progress bar and world name display
- LOG_ERROR/LOG_INFO/LOG_WARN convenience macros in Logger.h
- Engine::GetWindow() and Engine::GetUIManager() accessor methods
- UIElement::getPosition() returning Position struct
- UILabel::setAlignment() method with Alignment enum
- UIButton convenience methods: setBackgroundColor, setTextColor, setOnClickCallback, setNormalColor, setHoverColor, setPressedColor, setFontSize
- AssetManager::GetTextureByRendererID for texture lookup by OpenGL ID
- TileRendererSystem::m_OwnedTextures for procedural texture lifetime management
- Entity::id public field for simplified entity index access
- ECS notification chain: EntityManager→ComponentManager→SystemManager wire-up in Engine
- Minimal placeholder headers for stub files (Animation, FrameBuffer, Mesh, RenderTarget, Vertex, GLContext, GLBuffer)

### Fixed
- Entity::id vs getIndex() mismatch - added id field synced with m_ID.index
- EntityManager::destroyEntity now notifies ComponentManager and SystemManager
- ComponentManager::addComponent/removeComponent now notify SystemManager of signature changes
- Component.h static_assert logic bug: `8 >= 64` was always false
- MAX_COMPONENTS consolidated to single definition in Component.h
- UIManager.cpp duplicate handleMouseUp method definition removed
- UIManager::render() now properly uses ServiceLocator::Get (not GetInstance)
- RenderSystem.cpp include paths fixed (Core/ → core/)
- TileRendererSystem.cpp syntax error in pixel fill loop (line truncated)
- TileRendererSystem::update signature updated to match System::update base
- TileRendererSystem include paths fixed (../world/ → ../core/world/)
- GameScreen.h include path (../core/World.h → ../core/world/World.h)
- GameScreen WorldManager instance replaced with ServiceLocator approach
- WorldBrowserScreen m_SelectedWord typo → m_SelectedWorld
- LoginScreen/RegisterScreen GetText() → getText(), setIsPassword → setPasswordMode
- RegisterScreen OnClick → AminOnClickCallback
- Empty stub files (Animation.h, Mesh.h, etc.) now have `#pragma once` guards
- MSVC template disambiguation with ComponentType::template Get<T>()
- EntityManager.h forward declarations for ComponentManager/SystemManager

### Changed
- CMakeLists.txt: Added InputSystem.cpp, InputSystem.h, PlayerSystem.h to source lists
- CMakeLists.txt: Added /FS compile flag and MultiProcessorCompilation=false for VS
- Logger.h now defines LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR macros
- Screen files now use Logger macros instead of raw string concatenation

## [0.2.0] - 2026-07-29
### Added
- Complete Entity Component System (ECS) implementation
  - EntityManager: Handles entity creation, destruction, and ID recycling with generational indices
  - ComponentManager: Dense storage for components with bitset-based entity signatures
  - SystemManager: Registers and manages systems, maintains entity lists based on component signatures
  - Core Components: Transform, Sprite, Animation, Velocity, Collider, Network, Player, Input, Health, Camera2D
  - Core Systems: Movement, Render, Input, NetworkSync, Player, Camera2D, TileRenderer
- ECS integration with Engine class for initialization, updates, and shutdown
- ECS integration with ServiceLocator for manager access
- Basic authentication system with token storage and validation
- Font rendering implementation using per-glyph OpenGL textures
- World save/load functionality with binary file storage (.dat format)
- Comprehensive authentication system with token expiration
- Screen system: 8 screens (Splash, Login, Register, Continue, WorldBrowser, Loading, Game, Settings)
- HUD system with health, mana, level, experience, currency, chat, notifications
- UI system: UIManager, UILabel, UIButton, UIPanel, UITextBox, UIImage
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
- **Scheduling**: One-shot and repeatng job system with thread safety
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