cmake_minimum_required(VERSION 3.20)
# Test comment


project(StrixVerseClient LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)                        # NEW: strict ISO C++20


# --------------------------------------------------
# Compiler
# --------------------------------------------------


if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()


# --------------------------------------------------mersyeon123@gmail.com
# Find Packages
# --------------------------------------------------


find_package(SDL3 CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
# Set the directory for glad package
set(glad_DIR "${CMAKE_CURRENT_LIST_DIR}/../../vcpkg/installed/x64-windows/share/glad")
find_package(glad CONFIG REQUIRED)
find_package(Threads REQUIRED)                       # NEW: ThreadPool / Logger


# --------------------------------------------------
# Source Files
# --------------------------------------------------


set(SOURCES
    src/main.cpp

    # Core
    src/core/Application.cpp
    src/core/Config.cpp
    src/core/Engine.cpp
    src/core/Game.cpp
    src/core/Logger.cpp
    src/core/ThreadPool.cpp
    src/core/Timer.cpp
    src/core/Version.cpp
    src/core/Window.cpp
    src/core/Scheduler.cpp
    src/core/AuthService.cpp
    src/core/WorldManager.cpp
    src/core/AssetManager.cpp
    src/core/world/Tile.cpp
    src/core/world/Chunk.cpp
    src/core/world/World.cpp

    # Graphics
    src/graphics/Animation.cpp
    src/graphics/Camera2D.cpp
    src/graphics/Camera.cpp
    src/graphics/Color.cpp
    src/graphics/Font.cpp
    src/graphics/FrameBuffer.cpp
    src/graphics/Mesh.cpp
    src/graphics/Renderer.cpp
    src/graphics/RenderTarget.cpp
    src/graphics/Shader.cpp
    src/graphics/SpriteBatch.cpp
    src/graphics/Texture.cpp
    src/graphics/Vertex.cpp
    src/graphics/OpenGL/GLContext.cpp

    # Networking
    src/networking/Connection.cpp
    src/networking/NetworkManager.cpp
    src/networking/Packet.cpp
    src/networking/PacketBuffer.cpp
    src/networking/PacketFactory.cpp
    src/networking/PacketDispatcher.cpp
    src/networking/PacketRegistry.cpp
    src/networking/PacketReader.cpp
    src/networking/PacketSerializer.cpp
    src/networking/PacketSender.cpp
    src/networking/PacketHandler.cpp
    src/networking/PacketWriter.cpp
    src/networking/CharacterPacket.cpp
    src/networking/HandshakePacket.cpp
    src/networking/KeepAlive.cpp
    src/networking/KeepAlivePacket.cpp
    src/networking/LoginPacket.cpp
    src/networking/LoginResultPacket.cpp
    src/networking/NetworkStatistics.cpp
    src/networking/PingManager.cpp
    src/networking/PingPacket.cpp
    src/networking/PongPacket.cpp
    src/networking/PlayerMovePacket.cpp
    src/networking/PlayerRemovePacket.cpp
    src/networking/PlayerSpawnPacket.cpp
    src/networking/PlayerStatePacket.cpp
    src/networking/RegisterPacket.cpp
    src/networking/SessionPacket.cpp

    # ECS
    src/ecs/Entity.cpp
    src/ecs/EntityManager.cpp
    src/ecs/ComponentManager.cpp
    src/ecs/SystemManager.cpp
    src/ecs/MovementSystem.cpp
    src/ecs/InputSystem.cpp
    src/ecs/NetworkSyncSystem.cpp
    src/ecs/PlayerSystem.cpp
    src/ecs/RenderSystem.cpp
    src/ecs/TileRendererSystem.cpp
    src/ecs/Camera2DSystem.cpp

    # UI
    src/ui/UIElement.cpp
    src/ui/UIManager.cpp
    src/ui/UIPanel.cpp
    src/ui/UIButton.cpp
    src/ui/UITextBox.cpp
    src/ui/UIImage.cpp
    src/ui/UILabel.cpp

    # Screens
    src/screens/Screen.cpp
    src/screens/ScreenFactory.cpp
    src/screens/ScreenIDs.h     # Header-only
    src/screens/SplashScreen.cpp
    src/screens/LoadingScreen.cpp
    src/screens/LoginScreen.cpp
    src/screens/RegisterScreen.cpp
    src/screens/ContinueScreen.cpp
    src/screens/WorldBrowserScreen.cpp
    src/screens/SettingsScreen.cpp
    src/screens/GameScreen.cpp

    # HUD
    src/hud/HUD.cpp
)


# --------------------------------------------------
# Header Files
# --------------------------------------------------


set(HEADERS
    # Core
    src/core/Application.h
    src/core/Config.h
    src/core/Engine.h
    src/core/Game.h
    src/core/Logger.h
    src/core/ServiceLocator.h
    src/core/ThreadPool.h
    src/core/Timer.h
    src/core/Version.h
    src/core/Window.h
    src/core/Scheduler.h
    src/core/AuthService.h
    src/core/WorldManager.h
    src/core/AssetManager.h
    src/core/world/Tile.h
    src/core/world/Chunk.h
    src/core/world/World.h

    # Graphics
    src/graphics/Animation.h
    src/graphics/Camera2D.h
    src/graphics/Camera.h
    src/graphics/Color.h
    src/graphics/Font.h
    src/graphics/FrameBuffer.h
    src/graphics/Mesh.h
    src/graphics/Renderer.h
    src/graphics/RenderTarget.h
    src/graphics/Shader.h
    src/graphics/SpriteBatch.h
    src/graphics/Texture.h
    src/graphics/Vertex.h
    src/graphics/OpenGL/GLContext.h
    src/graphics/OpenGL/GLBuffer.h

    # Networking
    src/networking/Connection.h
    src/networking/NetworkManager.h
    src/networking/Packet.h
    src/networking/PacketBuffer.h
    src/networking/PacketFactory.h
    src/networking/PacketHandler.h
    src/networking/PacketDispatcher.h
    src/networking/PacketRegistry.h
    src/networking/PacketReader.h
    src/networking/PacketSerializer.h
    src/networking/PacketSender.h
    src/networking/PacketWriter.h
    src/networking/CharacterPacket.h
    src/networking/HandshakePacket.h
    src/networking/KeepAlive.h
    src/networking/KeepAlivePacket.h
    src/networking/LoginPacket.h
    src/networking/LoginResultPacket.h
    src/networking/NetworkStatistics.h
    src/networking/PingManager.h
    src/networking/PingPacket.h
    src/networking/PongPacket.h
    src/networking/PlayerMovePacket.h
    src/networking/PlayerRemovePacket.h
    src/networking/PlayerSpawnPacket.h
    src/networking/PlayerStatePacket.h
    src/networking/RegisterPacket.h
    src/networking/SessionPacket.h

    # ECS
    src/ecs/Component.h
    src/ecs/Entity.h
    src/ecs/EntityManager.h
    src/ecs/ComponentManager.h
    src/ecs/SystemManager.h
    src/ecs/System.h
    src/ecs/AnimationComponent.h
    src/ecs/Camera2DComponent.h
    src/ecs/ColliderComponent.h
    src/ecs/HealthComponent.h
    src/ecs/InputComponent.h
    src/ecs/InputSystem.h
    src/ecs/MovementSystem.h
    src/ecs/NetworkComponent.h
    src/ecs/NetworkSyncSystem.h
    src/ecs/PlayerComponent.h
    src/ecs/PlayerSystem.h
    src/ecs/RenderSystem.h
    src/ecs/SpriteComponent.h
    src/ecs/TransformComponent.h
    src/ecs/VelocityComponent.h
    src/ecs/Camera2DSystem.h
    src/ecs/TileRendererSystem.h

    # UI
    src/ui/UIElement.h
    src/ui/UIManager.h
    src/ui/UIPanel.h
    src/ui/UIButton.h
    src/ui/UITextBox.h
    src/ui/UIImage.h
    src/ui/UILabel.h

    # Screens
    src/screens/Screen.h
    src/screens/ScreenFactory.h
    src/screens/ScreenIDs.h
    src/screens/SplashScreen.h
    src/screens/LoadingScreen.h
    src/screens/LoginScreen.h
    src/screens/RegisterScreen.h
    src/screens/ContinueScreen.h
    src/screens/WorldBrowserScreen.h
    src/screens/SettingsScreen.h
    src/screens/GameScreen.h

    # HUD
    src/hud/HUD.h
)


# --------------------------------------------------
# Executable
# --------------------------------------------------


add_executable(StrixVerseClient
    ${SOURCES}
    ${HEADERS}
    resources/client.rc
)

# Force serialized PDB writes for parallel MSVC builds
if(MSVC)
    target_compile_options(StrixVerseClient PRIVATE /FS)
    set_target_properties(StrixVerseClient PROPERTIES
        VS_GLOBAL_MultiProcessorCompilation "false"
    )
endif()


# --------------------------------------------------
# Include Directories
# --------------------------------------------------


target_include_directories(StrixVerseClient PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_LIST_DIR}/../../vcpkg/installed/x64-windows/include
)


# --------------------------------------------------
# Link Libraries
# --------------------------------------------------


target_link_libraries(StrixVerseClient PRIVATE
    SDL3::SDL3
    Freetype::Freetype
    glm::glm
    Threads::Threads
    glad::glad
    Ws2_32
)


# --------------------------------------------------
# Version Stamping (optional git commit)             # NEW section
# --------------------------------------------------


find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        OUTPUT_VARIABLE STRIX_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE STRIX_GIT_RESULT
    )
    if(STRIX_GIT_RESULT EQUAL 0 AND STRIX_GIT_COMMIT)
        target_compile_definitions(StrixVerseClient PRIVATE
            STRIX_GIT_COMMIT="${STRIX_GIT_COMMIT}"
        )
    endif()
endif()


# --------------------------------------------------
# Output Directory
# --------------------------------------------------


set_target_properties(StrixVerseClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin/Release"
)


# --------------------------------------------------
# Create Runtime Folders
# --------------------------------------------------


add_custom_command(TARGET StrixVerseClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/Debug"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/bin/Release"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/configs"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/logs"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/screenshots"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/cache")