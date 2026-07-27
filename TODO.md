# StrixVerse Client - TODO List

## High Priority
- [x] Implement Entity Component System (ECS) (Phase 5) - including components, systems, engine integration, and game integration
- [ ] Create World/Tilemap/Chunk/Tile systems
- [ ] Develop basic Player controller and movement system
- [ ] Implement Audio system (loading, playback, mixing)
- [ ] Create basic UI system (HUD, menus)

## Medium Priority
- [ ] Replace simple JSON parser in Config with full JSON library
- [ ] Add log rotation to Logger system
- [ ] Implement resource streaming/async loading
- [ ] Add advanced texture features (arrays, cubemaps, render targets)
- [ ] Implement font system with text layout/word wrapping
- [ ] Add input mapping/action system
- [ ] Implement resource bundling/packaging system

## Low Priority
- [ ] Add time scaling/pause functionality to Timer
- [ ] Add priority levels to Scheduler and ThreadPool
- [ ] Implement dynamic resizing/work stealing in ThreadPool
- [ ] Add advanced window features (borderless, fullscreen toggle)
- [ ] Implement QoS/packet prioritization in networking
- [ ] Add packet compression to networking
- [ ] Add automatic reconnection with backoff to networking
- [ ] Add TLS/SSL encryption to networking

## Completed Systems (Reference)
✓ Core Systems: Logger, Timer, Scheduler, ThreadPool, AssetManager, Config, Window, Version
✓ Graphics: Renderer, Camera2D, SpriteBatch, Texture, Shader, Color
✓ Networking: Connection, NetworkManager, Packet system, KeepAlive, Ping/Keepalive
✓ Build System: CMake configuration with SDL3, Freetype, GLM, Glad, Threads
✓ Entry Point: Minimal main.cpp