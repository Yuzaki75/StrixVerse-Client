# StrixVerse Client - File Index

## Core Systems (/src/core)
- Application.cpp/h: Application lifecycle management (Init → Run → Shutdown)
- Engine.cpp/h: Main engine loop with fixed/variable timestep updates
- Game.cpp/h: Gameplay loop driving movement, collision, building, chat, and camera
- Window.cpp/h: SDL3 window management and OpenGL 4.6 context creation
- Config.cpp/h: JSON-like configuration loading/saving with validation (incl. sfxVolume)
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
  - ColliderComponent.h: width, height, enabled flag (AABB collision for player/build reach)
  - NetworkComponent.h: networkID, isLocalPlayer, synced x/y/rotation (with interpolation buffers)
  - PlayerComponent.h: username, score
  - InputComponent.h: bitset for keyboard state (SDL scancodes)
  - HealthComponent.h: current and maximum health
- System files:
  - MovementSystem.h/.cpp: updates Transform position using Velocity * dt
  - RenderSystem.h/.cpp: sorts entities by layer/texture, batches draws via SpriteBatch
  - InputSystem.h/.cpp: copies SDL keyboard state into InputComponent bitset
  - NetworkSyncSystem.h/.cpp: synchronizes Transform component with NetworkComponent; remote-player interpolation (100ms lerp, extrapolation, snap)
  - CharacterRenderSystem.h/.cpp: animated procedural player figures; spritesheet swap point marked here

## Graphics Systems (/src/graphics)
- Renderer.cpp/h: OpenGL state management and frame buffering
- Camera2D.cpp/h: 2D camera with zoom, rotation, and coordinate conversion (gameplay follow + shift/ctrl+wheel zoom)
- SpriteBatch.cpp/h: Efficient 2D sprite batching, capacity 16384
- Texture.cpp/h: OpenGL texture wrapper with stb_image loading
- Shader.cpp/h: GLSL shader program compilation and uniform setting
- Color.cpp/h: Color utilities and predefined constants
- Font.cpp/h: Per-glyph font rendering (atlas batching still pending)
- Animation.cpp/h: Real animation system - AnimationClip struct plus Animator playback of named clips from spritesheet strips
- Camera.cpp/h: Legacy camera system (superseded by Camera2D)
- Vertex.h/cpp: Stub files for vertex format (empty)
- Mesh.h/cpp: Stub files for mesh data (empty)
- FrameBuffer.h/cpp: Stub files for framebuffer objects (empty)
- RenderTarget.h/cpp: Stub files for render targets (empty)
- OpenGL/GLContext.h/cpp: OpenGL context management wrapper
- OpenGL/GLBuffer.h: OpenGL buffer object helper

## Networking Systems (/src/networking)
- Connection.cpp/h: TCP connection with Winsock, threading, and packet queuing
- NetworkManager.cpp/h: High-level network management; dispatches ~25 packet types including server-driven auth
- Packet.h: Base packet interface
- Auth packets:
  - LoginPacket.h / LoginSuccessPacket.h / LoginFailedPacket.h / RegisterPacket.h: server-driven login/register flow
- Player packets:
  - PlayerMovePacket.h, PlayerSpawnPacket.h, PlayerRemovePacket.h, PlayerDataPacket.h, PlayerBuffsPacket.h
- World packets:
  - WorldJoinPacket.h, WorldLeavePacket.h, WorldListPacket.h, WorldStatePacket.h
  - WorldManagePackets.h: invite/role/ban/settings management plus WorldNotificationPacket (opcode 112 client-side ready; server pending)
- Gameplay packets:
  - StrixCorePacket.h: Strix Core claim/interact/updated messages
  - TileChangePacket.h: block break/place sync
  - ChunkLoadPacket.h: chunk transfer with totals used by LoadingScreen progress
  - BlockBreakPacket.h / BlockPlacePacket.h: build actions (server enforces BlockReachTiles)
  - InventoryPacket.h / InventoryUpdatePacket.h / UseItemPacket.h
  - ChatMessagePacket.h
- Infrastructure:
  - PacketBuffer.cpp/h: Serialization buffer for packet data
  - PacketDispatcher.cpp/h: Handler-based packet dispatching system
  - PacketFactory.cpp/h: Automatic packet registration system
  - PacketRegistry.cpp/h: Packet type registry for dynamic creation
  - PacketSender.h / PacketHandler.h: Send helpers and handler base
  - PingPacket.h / PongPacket.h / PingManager.cpp/h: Latency measurement
  - KeepAlivePacket.h / KeepAlive.cpp/h: Connection keep-alive mechanism
  - HandshakePacket.h / DisconnectPacket.h: Session setup/teardown
  - NetworkStatistics.cpp/h: Bandwidth and packet statistics tracking

## HUD Panels (/src/hud)
- HUD.cpp/h: Main HUD - stats, hotbar, chat with system-message styling, notification stack with severity styling, interaction prompts ([E] Leave/Interact/Strix Core/Lost Tech)
- PauseOverlay.cpp/h: Pause overlay (Esc)
- PlayerListPanel.cpp/h: Player roster panel (Tab hold)
- InventoryPanel.cpp/h: Inventory panel (I)
- CharacterPanel.cpp/h: Character panel (C)
- BuffDisplay.cpp/h: Buff display shell (empty until server sends buff packets)
- WorldManagerPanel.cpp/h: In-game world management panel (wrench key)
- Lost Tech shells (UI ready, server ids 29-32 placeholder, callbacks log not-implemented):
  - VaultPanel.cpp/h
  - GatePanel.cpp/h
  - StabilizerPanel.cpp/h
  - MemoryCrystalPanel.cpp/h

## Audio (/src/audio)
- AudioManager.cpp/h: Music playback via SDL3+stb_vorbis; SFX as WAV one-shots with random variants across a 16-voice pool; sfxVolume control wired to Settings

## FX (/src/fx)
- ParticleSystem.cpp/h: Particle effects - block-break debris, Strix Core bursts, ambient Aether

## Screens (/src/screens)
- SplashScreen, MainMenuScreen, LoginScreen, RegisterScreen, ContinueScreen, ConnectingScreen, WorldBrowserScreen (selection + create-on-join), LoadingScreen (real chunk-progress with indeterminate fallback), GameScreen, SettingsScreen, CreditsScreen
- Screen.cpp/h, ScreenFactory.cpp/h, ScreenIDs.h: Screen lifecycle, factory, and ID registry

## Resources (/Assets)
- /tiles: Tile sprites 001-028 (dirt through strix core tiers I-IV, doors, ores, marble/castle wall/neon trim)
- /items: Item sprites incl. seeds (1001-1017), gear/consumables (1-401), punch/wrench tools with previews (9001-9002)
- /fonts: VT323, ShareTechMono, PressStart2P with OFL licenses
- /ui: UI textures and icons (punch.png, wrench.png), world_loading art sets (nature_2 through nature_8)
- /branding: logo.png, icon.ico
- /vfx: Reserved for effect assets (.gitkeep)
- /Worlds: Placeholder for world/tilemap data
- /audio: Music and SFX files (world_theme.ogg gameplay track still missing)

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

## Stub Files (No Implementation)
The following remain stubs; they are not required by the current rendering path:
- Graphics: Vertex.h/cpp, Mesh.h/cpp, FrameBuffer.h/cpp, RenderTarget.h/cpp
