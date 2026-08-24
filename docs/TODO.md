# StrixVerse Client - TODO List

## High Priority
- [ ] Server-side: implement buff packets (client BuffDisplay is ready and waiting)
- [ ] Server-side: WorldInfo metadata (owner/type/capacity/favourites) for world browser rows
- [ ] Server-side: include per-player roles in roster data
- [x] Implement Audio system (loading, playback, mixing)
- [ ] Server-side: send chunk totals in WorldState so LoadingScreen can show determinate progress
- [x] Implement proper ECS notification chain between EntityManager->ComponentManager->SystemManager
- [ ] Fix MSBuild PDB locking issue (C1041) in parallel builds - configure /FS flag and VCTargetsPath

## Medium Priority
- [ ] Author character spritesheet art to replace procedural placeholder (swap point marked in CharacterRenderSystem.cpp)
- [ ] Convert dirtBreak/join/exit SFX from .mp3 to WAV
- [ ] Add gameplay music track world_theme.ogg to Assets
- [ ] Implement Lost Tech button callbacks once server packet senders exist (Vault/Gate/Stabilizer/MemoryCrystal, ids 29-32)
- [ ] Replace simple JSON parser in Config with full JSON library
- [x] Create basic UI system (HUD, menus)
- [x] Develop basic Player controller and movement system
- [ ] Implement resource streaming/async loading
- [ ] Implement font texture atlas batching for Font rendering
- [ ] Add world-row hover highlight to WorldBrowser (needs UIPanel hover support)
- [ ] Persist auth token across sessions
- [ ] Add TLS/SSL encryption to networking
- [ ] Integrate SpriteBatch rendering with ECS RenderSystem properly
- [ ] Complete AssetManager::GetTextureByRendererID index for fast lookup
- [ ] Add Camera integration to screens (SplashScreen, etc.)

## Low Priority
- [ ] Add time scaling/pause functionality to Timer
- [ ] Add priority levels to Scheduler and ThreadPool
- [ ] Implement dynamic resizing/work stealing in ThreadPool
- [ ] Add advanced window features (borderless, fullscreen toggle)
- [ ] Implement QoS/packet prioritization in networking
- [ ] Add packet compression to networking
- [ ] Add automatic reconnection with backoff to networking
- [ ] Add OpenGL error checking macros (GL_CHECK)

## Completed Systems (Reference)
✓ Core Systems: Logger, Timer, Scheduler, ThreadPool, AssetManager, Config, Window, Version, Engine, Application
✓ Graphics: Renderer, Camera2D, SpriteBatch (16384 capacity), Texture, Shader, Color, Animation (Animator/AnimationClip)
✓ Networking: Connection, NetworkManager, ~25 packet types incl. StrixCore/world manage/TileChange/Notification, KeepAlive, Ping
✓ Build System: CMake configuration with SDL3, Freetype, GLM, Glad, Threads
✓ Entry Point: Minimal main.cpp
✓ ECS: EntityManager, ComponentManager, SystemManager, 7+ systems, component types
✓ World: Tile, Chunk, World with save/load, network chunk loading with real progress
✓ UI: UIManager, UILabel, UIButton, UIPanel, UITextBox, UIImage, full keyboard dispatch
✓ Screens: Splash > Login > Register > Continue > Connecting > WorldBrowser (create-on-join) > Loading > Game, plus Settings/Credits/MainMenu
✓ Gameplay: movement/jump/gravity, AABB collision, block break/place with 2x2x2 reach + hover highlight, chat, camera follow/zoom, remote-player interpolation
✓ FX: ParticleSystem (block-break debris, Strix Core bursts, ambient Aether)
✓ Audio: AudioManager with music (SDL3+stb_vorbis) and SFX (WAV one-shots, variants, 16 voices), sfxVolume setting
✓ HUD: stats, hotbar (punch/wrench/items), chat with system styling, notification stack, interaction prompts, buff display shell
✓ Panels: PauseOverlay, PlayerListPanel, InventoryPanel, CharacterPanel, WorldManagerPanel, Lost Tech shells (Vault/Gate/Stabilizer/MemoryCrystal)

## Audit v0.2.0 Fixes (2026-07-29)