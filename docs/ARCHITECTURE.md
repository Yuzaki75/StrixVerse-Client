# StrixVerse Client - Architecture Overview

## System Overview
The StrixVerse client follows a modular, layered architecture with clear separation of concerns. Core systems provide foundational services while higher-level systems handle game-specific functionality.

## Core Architecture Layers

### 1. Platform Abstraction Layer
- **Window**: SDL3 wrapper for window creation, OpenGL context, and input handling
- **Timer**: High-resolution timing with fixed/variable timestep support
- **Config**: JSON-like configuration system with validation and persistence

### 2. Core Services Layer
- **Logger**: Thread-safe logging to file and console with severity levels
- **ServiceLocator**: Dependency injection pattern for accessing systems
- **ThreadPool**: Work-stealing thread pool for parallel task execution
- **Scheduler**: Job scheduling system for one-off and repeating tasks
- **AssetManager**: Resource caching system for textures and shaders

### 3. Rendering Layer
- **Renderer**: OpenGL state management and frame buffering
- **Camera2D**: 2D camera with zoom, rotation, and coordinate conversion
- **SpriteBatch**: Efficient 2D sprite rendering with texture atlasing
- **Texture**: OpenGL texture wrapper with mipmap and SRGB support
- **Shader**: GLSL shader program management with uniform setting
- **Color**: Color utilities and predefined constants

### 4. Networking Layer
- **Connection**: Low-level TCP connection with Winsock and threading
- **NetworkManager**: High-level connection management and game-specific messaging
- **Packet System**: Serialization/deserialization with automatic registration
- **Support Systems**: Keep-alive, ping/pong, buffering, dispatching, statistics

## Key Design Patterns

### Service Locator
Systems register themselves with ServiceLocator during initialization and can be accessed globally via templated Get<T>() methods.

### Resource Acquisition Is Initialization (RAII)
All resources (OpenGL objects, memory, file handles) use RAII principles for automatic cleanup.

### Fixed Timestep Game Loop
Engine maintains separate update (fixed timestep) and render (variable rate) loops for deterministic physics.

### Component-Based Design (Implemented - Entity Component System)
The Entity Component System (ECS) is a core gameplay framework fully integrated into the engine. Entities are unique identifiers (index + generation), components are data-only structs, and systems process entities that match their component signatures. The ECS managers (EntityManager, ComponentManager, SystemManager) are owned by the Engine and accessed via ServiceLocator. Systems are automatically registered and updated during the engine's update phase. This design provides cache-efficient data processing and flexible entity composition.

## Data Flow

1. **Initialization**: 
   - Main → Application → Logger/Config → Window → Engine → Game/Resource Managers

2. **Main Loop**:
   - Engine: Process Events → Update (fixed timestep) → Render (variable rate)
   - Game: Update game logic → Render via SpriteBatch/Renderer
   - Renderer: Clear → Draw → Present

3. **Networking**:
   - NetworkManager sends/receives packets via Connection
   - PacketDispatcher routes packets to registered handlers
   - Handlers update game state through ServiceLocator systems

4. **Rendering**:
   - Game prepares sprites/data
   - SpriteBatch collects and sorts by texture
   - Renderer sets state and draws batches

## Extension Points

### Marked Future Systems
- InputManager: Keyboard/mouse/gamepad input handling
- AudioManager: Sound effects and music playback
- UIManager: User interface and HUD rendering
- PhysicsSystem: Collision detection and response
- WorldSystem: Tilemap/chunk management

### Plug-in Architecture
ServiceLocator allows systems to be replaced or extended without modifying core code.

## Threading Model

- **Main Thread**: Game logic, rendering, input
- **Worker Threads**: ThreadPool for async tasks (loading, pathfinding, etc.)
- **Network Thread**: Dedicated socket I/O in Connection class
- **Timer Thread**: High-frequency timing updates

## File Organization

```
/src
  /core: Fundamental systems (Application, Engine, Window, etc.)
  /graphics: Rendering systems (Renderer, Camera, SpriteBatch, etc.)
  /networking: Networking systems (Connection, Packet*, etc.)
  /[future]: World, Entity, Player, UI, Audio, Gameplay systems
```

## Dependencies
- SDL3: Windowing, input, OpenGL context
- Glad: OpenGL function loading
- GLM: Mathematics for graphics
- Freetype: Font rendering (planned)
- stb_image: Image loading (embedded)
- CMake: Build system