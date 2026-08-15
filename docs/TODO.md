# StrixVerse Client - TODO List

## High Priority
- [x] Implement Entity Component System (ECS) (Phase 5) - including components, systems, engine integration, and game integration
- [x] Create World/Tilemap/Chunk/Tile systems
- [x] Develop basic Player controller and movement system
- [ ] Implement Audio system (loading, playback, mixing)
- [x] Create basic UI system (HUD, menus)
- [ ] Implement proper ECS notification chain between EntityManager->ComponentManager->SystemManager ✓ (wired up, needs validation)
- [ ] Fix MSBuild PDB locking issue (C1041) in parallel builds - configure /FS flag and VCTargetsPath

## Medium Priority
- [ ] Replace simple JSON parser in Config with full JSON library
- [ ] Add log rotation to Logger system
- [ ] Implement resource streaming/async loading
- [ ] Add advanced texture features (arrays, cubemaps, render targets)
- [ ] Implement font system with text layout/word wrapping
- [ ] Add input mapping/action system
- [ ] Implement resource bundling/packaging system
- [ ] Complete AssetManager::GetTextureByRendererID index for fast lookup
- [ ] Integrate SpriteBatch rendering with ECS RenderSystem properly (needs sprite shader)
- [ ] Fix Font rendering - current glyph-by-glyph approach needs atlas batching
- [ ] Add Camera integration to screens (SplashScreen, etc.)

## Low Priority
- [ ] Add time scaling/pause functionality to Timer
- [ ] Add priority levels to Scheduler and ThreadPool
- [ ] Implement dynamic resizing/work stealing in ThreadPool
- [ ] Add advanced window features (borderless, fullscreen toggle)
- [ ] Implement QoS/packet prioritization in networking
- [ ] Add packet compression to networking
- [ ] Add automatic reconnection with backoff to networking
- [ ] Add TLS/SSL encryption to networking
- [ ] Implement proper Texture Atlas for Font rendering
- [ ] Add OpenGL error checking macros (GL_CHECK)

## Completed Systems (Reference)
✓ Core Systems: Logger, Timer, Scheduler, ThreadPool, AssetManager, Config, Window, Version, Engine, Application
✓ Graphics: Renderer, Camera2D, SpriteBatch, Texture, Shader, Color
✓ Networking: Connection, NetworkManager, Packet system, KeepAlive, Ping/Keepalive
✓ Build System: CMake configuration with SDL3, Freetype, GLM, Glad, Threads
✓ Entry Point: Minimal main.cpp
✓ ECS: EntityManager, ComponentManager, SystemManager, 7 systems, 9 component types
✓ World: Tile, Chunk, World with save/load
✓ UI: UIManager, UILabel, UIButton, UIPanel, UITextBox, UIImage
✓ Screens: Splash, Login, Register, Continue, WorldBrowser, Loading, Game, Settings

## Audit v0.2.0 Fixes (2026-07-29)
- [x] Created LoadingScreen.cpp (was missing, caused linker error)
- [x] Fixed Entity class public `id` field for proper entity index access
- [x] Wired ECS notification chain (EntityManager->ComponentManager->SystemManager)
- [x] Fixed Component.h static_assert logic bug (6 >= 64 was false)
- [x] Consolidated MAX_COMPONENTS to single location (Component.h)
- [x] Fixed duplicate handleMouseUp in UIManager.cpp
- [x] Added LOG_ERROR/LOG_INFO/LOG_WARN macros to Logger.h
- [x] Added GetWindow()/inkUIManager() accessors to Engine
- [x] Added getPosition(), setAlignment(), setBackgroundColor() to UI classes
- [x] Fixed TileRendererSystem::update signature mismatch
- [x] Fixed include paths (Core/ vs core/, World.h location)
- [x] Fixed GetText→getText, setIsPassword→setPasswordMode API calls
- [x] Fixed OnClick→setOnClickCallback in RegisterScreen
- [x] Fixed WorldBrowserScreen m_ ﻿SelectedWord typo
- [x] Added InputSystem.cpp/h to CMakeLists.txt
- [x] Added PlayerSystem.h to CMakeLists.txt headers
- [x] Added /FS flag for MSVC PDB serialization
- [x] Fixed empty stub files (Animation.h, FrameBuffer.h, etc.) with placeholder guards
- [x] Fixed ComponentManager template disambiguation for MSVC
- [x] Added EntityManager.h forward declarations for notification chain
- [x] Fixed AssetManager to support GetTextureByRendererID lookup