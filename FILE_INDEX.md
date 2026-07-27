# StrixVerse Client - File Index

## Core Systems (/src/core)
- Application.cpp/h: Application lifecycle management (Init → Run → Shutdown)
- Engine.cpp/h: Main engine loop with fixed/variable timestep updates
- Game.cpp/h: Scene management backbone (placeholder for World subsystem)
- Window.cpp/h: SDL3 window management and OpenGL 4.6 context creation
- Config.cpp/h: JSON-like configuration loading/saving with validation
- Logger.cpp/h: Thread-safe logging to file and console with levels
- Timer.cpp/h: High-resolution timing with delta time and FPS calculation
- Scheduler.cpp/h: Job scheduling system with one-shot/repeating tasks
- ThreadPool.cpp/h: Thread pool implementation with task queue
- AssetManager.cpp/h: Resource caching system for textures and shaders
- ServiceLocator.h: Header-only dependency injection/service locator
- Version.cpp/h: Version stamping system with CMake/Git overrides
- main.cpp: Minimal application entry point

## Entity Component System (/src/ecs)
- Entity.h: Lightweight entity handle with index and generation for ID recycling
- Entity.cpp: Implementation of Entity methods
- Component.h: Base Component class and ComponentType template for unique type IDs
- EntityManager.h/.cpp: Manages entity creation, destruction, and alive status with free list
- ComponentManager.h/.cpp: Registers component types, stores components in dense arrays, manages entity signatures
- System.h/.cpp: Base System class with init, update, render methods and signature management
- SystemManager.h/.cpp: Registers systems, initializes them with managers, maintains per-system entity lists
- Component files:
  - TransformComponent.h: position, rotation, scale
  - SpriteComponent.h: textureID, color, layer
  - AnimationComponent.h: frame timing, start/end/current frame, loop flag
  - VelocityComponent.h: vx, vy
  - ColliderComponent.h: width, height, enabled flag
  - NetworkComponent.h: networkID, isLocalPlayer, synced x/y/rotation
  - PlayerComponent.h: username, score
  - InputComponent.h: bitset for keyboard state (SDL scancodes)
  - HealthComponent.h: current and maximum health
- System files:
  - MovementSystem.h/.cpp: updates Transform position using Velocity * dt
  - RenderSystem.h/.cpp: sorts entities by layer/texture, batches draws via SpriteBatch
  - InputSystem.h/.cpp: copies SDL keyboard state into InputComponent bitset
  - NetworkSyncSystem.h/.cpp: synchronizes Transform component with NetworkComponent for networked entities

## Graphics Systems (/src/graphics)
- Renderer.cpp/h: OpenGL state management and frame buffering
- Camera2D.cpp/h: 2D camera with zoom, rotation, and coordinate conversion
- SpriteBatch.cpp/h: Efficient 2D sprite batching with texture atlasing
- Texture.cpp/h: OpenGL texture wrapper with stb_image loading
- Shader.cpp/h: GLSL shader program compilation and uniform setting
- Color.cpp/h: Color utilities and predefined constants
- Font.cpp/h: Basic font rendering system (minimal implementation)
- Camera.cpp/h: Legacy camera system (superseded by Camera2D)
- Vertex.h/cpp: Stub files for vertex format (empty)
- Mesh.h/cpp: Stub files for mesh data (empty)
- FrameBuffer.h/cpp: Stub files for framebuffer objects (empty)
- RenderTarget.h/cpp: Stub files for render targets (empty)
- Animation.h/cpp: Empty animation system files
- OpenGL/GLContext.h/cpp: OpenGL context management wrapper
- OpenGL/GLBuffer.h: OpenGL buffer object helper

## Networking Systems (/src/networking)
- Connection.cpp/h: TCP connection with Winsock, threading, and packet queuing
- NetworkManager.cpp/h: High-level network management with keep-alive/ping
- Packet*.cpp/h: Complete packet system (Login, Character, KeepAlive, Ping/Pong, Player movement/spawn/state)
- PacketBuffer.cpp/h: Serialization buffer for packet data
- PacketDispatcher.cpp/h: Handler-based packet dispatching system
- PacketFactory.cpp/h: Automatic packet registration system
- PacketRegistry.cpp/h: Packet type registry for dynamic creation
- PingManager.cpp/h: Ping/pong latency measurement system
- KeepAlive.cpp/h: Connection keep-alive mechanism
- NetworkStatistics.cpp/h: Bandwidth and packet statistics tracking

## Resources (/Assets)
- /Icons: Game icons and sprites
- /Worlds: Placeholder for world/tilemap data
- logo.png: Application logo
- /ui: Placeholder UI sprites (coin.txt, heart.txt, etc.)

## Build System
- CMakeLists.txt: CMake build configuration with SDL3, Freetype, GLM, Glad, Threads
- resources/client.rc: Windows resource file for version information

## Documentation
- TODO.md: List of pending tasks and features
- PROJECT_STATUS.md: Completion percentages and system status
- ARCHITECTURE.md: System architecture and design patterns
- FILE_INDEX.md: This file - complete listing of all source files
- CHANGELOG.md: Project change history
- KNOWN_ISSUES.md: List of known bugs and issues
- Readme.md: Project overview and instructions
- NETWORKING_SUMMARY.md: Networking subsystem details (historical)
- client folder structure.md: Folder structure documentation

## Missing/Stub Files
The following systems have no implementation or only stub files:
- World System: World.cpp/h, TileMap.cpp/h, Chunk.cpp/h, Tile.cpp/h, WorldManager.cpp/h
- Player System: Character.cpp/h, Inventory.cpp/h, Equipment.cpp/h, Skills.cpp/h, Stats.cpp/h, Movement.cpp/h
- Gameplay Systems: Building/Breaking/Crafting/Mining/Farming/Trading/Economy/NPC/Chat/Quest systems
- UI System: Login/Register/HUD/InventoryUI/ChatUI/CraftingUI/Settings/MainMenu/PauseMenu systems
- Audio System: AudioManager/Music/SoundEffects systems
- Resource Loading: AssetLoading/AsyncLoading/Streaming/Cache systems