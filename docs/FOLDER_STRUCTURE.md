StrixVerse/
├── Client/
│
├── Assets/
│   ├── Worlds/
│   │   ├── nature_1/
│   │   │   ├── map.json
│   │   │   ├── collision.json
│   │   │   ├── objects.json
│   │   │   ├── npcs.json
│   │   │   ├── spawn.json
│   │   │   ├── terrain.png
│   │   │   ├── tiles.png
│   │   │   ├── preview.png
│   │   │   └── music.ogg
│   │   │
│   │   ├── nature_2/
│   │   ├── nature_3/
│   │   ├── nature_4/
│   │   ├── nature_5/
│   │   ├── nature_6/
│   │   ├── nature_7/
│   │   └── nature_8/
│   │
│   ├── Textures/
│   │   ├── Characters/
│   │   ├── Monsters/
│   │   ├── NPCs/
│   │   ├── Items/
│   │   ├── UI/
│   │   ├── Effects/
│   │   └── Icons/
│   │
│   ├── Fonts/
│   │
│   ├── Audio/
│   │   ├── Music/
│   │   ├── SFX/
│   │   └── Ambient/
│   │
│   ├── Shaders/
│   │
│   ├── Animations/
│   │
│   └── Localization/
│
├── Config/
│
├── Logs/
│
├── Cache/
│
├── Screenshots/
│
├── src/
│   │
│   ├── main.cpp
│   │
│   ├── core/
│   │   ├── Application.h
│   │   ├── Application.cpp
│   │   ├── Engine.h
│   │   ├── Engine.cpp
│   │   ├── Window.h
│   │   ├── Window.cpp
│   │   ├── Config.h
│   │   ├── Config.cpp
│   │   ├── Logger.h
│   │   ├── Logger.cpp
│   │   ├── Time.h
│   │   └── Time.cpp
│   │
│   ├── graphics/
│   │   ├── Renderer.h
│   │   ├── Renderer.cpp
│   │   ├── Camera.h
│   │   ├── Camera.cpp
│   │   ├── SpriteBatch.h
│   │   ├── SpriteBatch.cpp
│   │   ├── Texture.h
│   │   ├── Texture.cpp
│   │   ├── Shader.h
│   │   ├── Shader.cpp
│   │   ├── Font.h
│   │   ├── Font.cpp
│   │   ├── Animation.h
│   │   ├── Animation.cpp
│   │   ├── Color.h
│   │   ├── Color.cpp
│   │   │
│   │   └── opengl/
│   │       ├── GLContext.h
│   │       ├── GLContext.cpp
│   │       ├── GLBuffer.h
│   │       ├── GLBuffer.cpp
│   │       ├── GLVertexArray.h
│   │       ├── GLVertexArray.cpp
│   │       ├── GLShader.h
│   │       └── GLShader.cpp
│   │
│   ├── world/
│   │   ├── World.h
│   │   ├── World.cpp
│   │   ├── WorldManager.h
│   │   ├── WorldManager.cpp
│   │   ├── TileMap.h
│   │   ├── TileMap.cpp
│   │   ├── Chunk.h
│   │   ├── Chunk.cpp
│   │   ├── Tile.h
│   │   ├── Tile.cpp
│   │   ├── Tileset.h
│   │   └── Tileset.cpp
│   │
│   ├── entity/
│   │   ├── Entity.h
│   │   ├── Entity.cpp
│   │   ├── EntityManager.h
│   │   ├── EntityManager.cpp
│   │   ├── Player.h
│   │   ├── Player.cpp
│   │   ├── NPC.h
│   │   ├── NPC.cpp
│   │   ├── Monster.h
│   │   ├── Monster.cpp
│   │   ├── Projectile.h
│   │   └── Projectile.cpp
│   │
│   ├── components/
│   │   ├── Transform.h
│   │   ├── Transform.cpp
│   │   ├── Sprite.h
│   │   ├── Sprite.cpp
│   │   ├── Animator.h
│   │   ├── Animator.cpp
│   │   ├── Collider.h
│   │   ├── Collider.cpp
│   │   ├── Health.h
│   │   └── Health.cpp
│   │
│   ├── input/
│   │   ├── Input.h
│   │   ├── Input.cpp
│   │   ├── Keyboard.h
│   │   ├── Keyboard.cpp
│   │   ├── Mouse.h
│   │   └── Mouse.cpp
│   │
│   ├── physics/
│   │   ├── Physics.h
│   │   ├── Physics.cpp
│   │   ├── Collision.h
│   │   └── Collision.cpp
│   │
│   ├── ui/
│   │   ├── UIManager.h
│   │   ├── UIManager.cpp
│   │   ├── Widget.h
│   │   ├── Widget.cpp
│   │   ├── Button.h
│   │   ├── Button.cpp
│   │   ├── Label.h
│   │   ├── Label.cpp
│   │   ├── TextBox.h
│   │   ├── TextBox.cpp
│   │   ├── InventoryWindow.h
│   │   ├── InventoryWindow.cpp
│   │   ├── ChatWindow.h
│   │   ├── ChatWindow.cpp
│   │   ├── Minimap.h
│   │   ├── Minimap.cpp
│   │   ├── SkillBar.h
│   │   └── SkillBar.cpp
│   │
│   ├── network/
│   │   ├── NetworkClient.h
│   │   ├── NetworkClient.cpp
│   │   ├── Packet.h
│   │   ├── Packet.cpp
│   │   ├── PacketReader.h
│   │   ├── PacketReader.cpp
│   │   ├── PacketWriter.h
│   │   └── PacketWriter.cpp
│   │
│   ├── audio/
│   │   ├── AudioManager.h
│   │   ├── AudioManager.cpp
│   │   ├── Music.h
│   │   ├── Music.cpp
│   │   ├── Sound.h
│   │   └── Sound.cpp
│   │
│   ├── resources/
│   │   ├── AssetManager.h
│   │   ├── AssetManager.cpp
│   │   ├── ResourceCache.h
│   │   └── ResourceCache.cpp
│   │
│   ├── scripting/
│   │   ├── Script.h
│   │   ├── Script.cpp
│   │   ├── ScriptManager.h
│   │   └── ScriptManager.cpp
│   │
│   ├── utilities/
│   │   ├── FileSystem.h
│   │   ├── FileSystem.cpp
│   │   ├── Random.h
│   │   ├── Random.cpp
│   │   ├── Math.h
│   │   ├── Math.cpp
│   │   ├── UUID.h
│   │   └── UUID.cpp
│   │
│   └── third_party/
│
├── CMakeLists.txt
├── CMakePresets.json
└── vcpkg.json