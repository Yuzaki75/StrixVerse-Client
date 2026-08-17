cmake_minimum_required(VERSION 3.20)

project(StrixVerseClient LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Compiler
# --------------------------------------------------

if(MSVC)
    # Matches the server's flag set so both halves are held to the same
    # standard: conformance mode on, parallel compilation, UTF-8 source and
    # execution charset, and the conforming preprocessor.
    add_compile_options(
        /W4
        /permissive-
        /MP
        /utf-8
        /Zc:preprocessor
    )

    # <windows.h> arrives through the Winsock networking layer. Without these
    # its min/max macros break every std::min / std::max call site.
    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
else()
    add_compile_options(-Wall -Wextra)
endif()

# --------------------------------------------------
# Find Packages
# --------------------------------------------------

find_package(SDL3 CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Threads REQUIRED)

# --------------------------------------------------
# Source Files
# --------------------------------------------------

set(SOURCES

    # Entry
    src/main.cpp

    # Third Party
    third_party/glad/src/glad.c

    # --------------------------------------------------
    # Core
    # --------------------------------------------------

    src/core/Application.cpp
    src/core/Config.cpp
    src/core/Engine.cpp
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

    # --------------------------------------------------
    # Graphics
    # --------------------------------------------------

    src/graphics/Camera2D.cpp
    src/graphics/Color.cpp
    src/graphics/Font.cpp
    src/graphics/Renderer.cpp
    src/graphics/Shader.cpp
    src/graphics/SpriteBatch.cpp
    src/graphics/Texture.cpp
    src/graphics/UIRenderer.cpp


    # --------------------------------------------------
    # Networking
    # --------------------------------------------------

    src/networking/Connection.cpp
    src/networking/NetworkManager.cpp

    src/networking/PacketDispatcher.cpp
    src/networking/PacketRegistry.cpp

    src/networking/KeepAlive.cpp
    src/networking/NetworkStatistics.cpp
    src/networking/PingManager.cpp

    # --------------------------------------------------
    # ECS
    # --------------------------------------------------

    src/ecs/Entity.cpp
    src/ecs/EntityManager.cpp
    src/ecs/ComponentManager.cpp
    src/ecs/SystemManager.cpp

    src/ecs/InputSystem.cpp
    src/ecs/MovementSystem.cpp
    src/ecs/NetworkSyncSystem.cpp
    src/ecs/PlayerSystem.cpp
    src/ecs/RenderSystem.cpp
    src/ecs/CharacterRenderSystem.cpp
    src/ecs/TileRendererSystem.cpp
    src/ecs/Camera2DSystem.cpp
    src/ecs/CollisionSystem.cpp

    # --------------------------------------------------
    # UI
    # --------------------------------------------------

    src/ui/UIElement.cpp
    src/ui/UIManager.cpp
    src/ui/UIPanel.cpp
    src/ui/UIButton.cpp
    src/ui/UITextBox.cpp
    src/ui/UIImage.cpp
    src/ui/UILabel.cpp
    src/ui/UICheckBox.cpp
    src/ui/UIIcon.cpp
    src/ui/UIProgressBar.cpp
    src/ui/UIScrollPanel.cpp
    src/ui/UITiledImage.cpp
    src/ui/UIFonts.cpp
    src/ui/UIPatterns.cpp
    src/ui/UIScale.cpp

    # --------------------------------------------------
    # Screens
    # --------------------------------------------------

    src/screens/Screen.cpp
    src/screens/ScreenFactory.cpp

    src/screens/SplashScreen.cpp
    src/screens/MainMenuScreen.cpp
    src/screens/CreditsScreen.cpp
    src/screens/LoadingScreen.cpp
    src/screens/LoginScreen.cpp
    src/screens/RegisterScreen.cpp
    src/screens/ConnectingScreen.cpp
    src/screens/ContinueScreen.cpp
    src/screens/WorldBrowserScreen.cpp
    src/screens/SettingsScreen.cpp
    src/screens/GameScreen.cpp

    # --------------------------------------------------
    # Audio
    # --------------------------------------------------

    src/audio/AudioManager.cpp

    # --------------------------------------------------
    # HUD
    # --------------------------------------------------

    src/hud/HUD.cpp
)

# --------------------------------------------------
# Header Files
# --------------------------------------------------

set(HEADERS

    # --------------------------------------------------
    # Core
    # --------------------------------------------------

    src/core/Application.h
    src/core/AssetManager.h
    src/core/AuthService.h
    src/core/Config.h
    src/core/Engine.h
    src/core/Logger.h
    src/core/Scheduler.h
    src/core/ServiceLocator.h
    src/core/ThreadPool.h
    src/core/Timer.h
    src/core/Version.h
    src/core/Window.h

    src/core/world/Chunk.h
    src/core/world/Tile.h
    src/core/world/World.h

    src/core/WorldManager.h

    # --------------------------------------------------
    # Graphics
    # --------------------------------------------------

    src/graphics/Camera2D.h
    src/graphics/Color.h
    src/graphics/Font.h
    src/graphics/Renderer.h
    src/graphics/Shader.h
    src/graphics/SpriteBatch.h
    src/graphics/Texture.h
    src/graphics/UIRenderer.h


    # --------------------------------------------------
    # Networking
    # --------------------------------------------------

    src/networking/Connection.h
    src/networking/NetworkManager.h

    src/networking/Packet.h
    src/networking/PacketBuffer.h
    src/networking/PacketDispatcher.h
    src/networking/PacketHandler.h
    src/networking/PacketRegistry.h
    src/networking/PacketSender.h

    src/networking/Protocol.h

    src/networking/ChatMessagePacket.h
    src/networking/DisconnectPacket.h
    src/networking/HandshakePacket.h
    src/networking/InventoryPacket.h
    src/networking/InventoryUpdatePacket.h
    src/networking/KeepAlive.h
    src/networking/KeepAlivePacket.h
    src/networking/LoginFailedPacket.h
    src/networking/LoginPacket.h
    src/networking/LoginSuccessPacket.h
    src/networking/NetworkStatistics.h
    src/networking/PingManager.h
    src/networking/PingPacket.h
    src/networking/PongPacket.h
    src/networking/PlayerDataPacket.h
    src/networking/PlayerMovePacket.h
    src/networking/PlayerRemovePacket.h
    src/networking/PlayerSpawnPacket.h
    src/networking/RegisterPacket.h
    src/networking/WorldJoinPacket.h
    src/networking/WorldLeavePacket.h
    src/networking/WorldStatePacket.h

    # --------------------------------------------------
    # ECS
    # --------------------------------------------------

    src/ecs/AnimationComponent.h
    src/ecs/Camera2DComponent.h
    src/ecs/Camera2DSystem.h
    src/ecs/ColliderComponent.h
    src/ecs/CollisionSystem.h
    src/ecs/Component.h
    src/ecs/ComponentManager.h
    src/ecs/Entity.h
    src/ecs/EntityManager.h
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
    src/ecs/CharacterComponent.h
    src/ecs/CharacterRenderSystem.h
    src/graphics/CharacterPalette.h
    src/ecs/System.h
    src/ecs/SystemManager.h
    src/ecs/TileRendererSystem.h
    src/ecs/TransformComponent.h
    src/ecs/VelocityComponent.h

    # --------------------------------------------------
    # UI
    # --------------------------------------------------

    src/ui/UIButton.h
    src/ui/UICheckBox.h
    src/ui/UIElement.h
    src/ui/UIFonts.h
    src/ui/UIIcon.h
    src/ui/UIImage.h
    src/ui/UILabel.h
    src/ui/UIManager.h
    src/ui/UIPanel.h
    src/ui/UIPatterns.h
    src/ui/UIProgressBar.h
    src/ui/UIScale.h
    src/ui/UIScrollPanel.h
    src/ui/UITextBox.h
    src/ui/UITheme.h
    src/ui/UITiledImage.h

    # --------------------------------------------------
    # Screens
    # --------------------------------------------------

    src/screens/ConnectingScreen.h
    src/screens/ContinueScreen.h
    src/screens/GameScreen.h
    src/screens/LoadingScreen.h
    src/screens/LoginScreen.h
    src/screens/RegisterScreen.h
    src/screens/Screen.h
    src/screens/ScreenFactory.h
    src/screens/ScreenIDs.h
    src/screens/SettingsScreen.h
    src/screens/SplashScreen.h
    src/screens/MainMenuScreen.h
    src/screens/CreditsScreen.h
    src/screens/WorldBrowserScreen.h

    # --------------------------------------------------
    # HUD
    # --------------------------------------------------

    src/audio/AudioManager.h

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
    ${PROJECT_SOURCE_DIR}/third_party/glad/include
)

# --------------------------------------------------
# Link Libraries
# --------------------------------------------------

target_link_libraries(StrixVerseClient PRIVATE
    SDL3::SDL3
    Freetype::Freetype
    glm::glm
    Threads::Threads
    Ws2_32
)

# --------------------------------------------------
# Git Version
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
# Output Directories
# --------------------------------------------------

# Executables go to bin/, never into the build tree - build/ holds generated
# project files only, so it stays disposable.
set_target_properties(StrixVerseClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin/Release"
)

# --------------------------------------------------
# Runtime Directories
# --------------------------------------------------

# The client resolves "shaders/..." and "assets/..." relative to its working
# directory, so both trees are mirrored next to every executable. Copying whole
# directories keeps this correct as shaders and assets are added, instead of
# needing one command per file.

# Only the two directories the executable is actually produced in. The build
# tree was on this list, which copied the whole asset set into it on every
# build even though nothing ever ran from there. The client creates its own
# logs/ and saves/ at startup, so no directory needs preparing here.
set(STRIX_RUNTIME_DIRS
    "${PROJECT_SOURCE_DIR}/bin/Debug"
    "${PROJECT_SOURCE_DIR}/bin/Release"
)

foreach(RUNTIME_DIR IN LISTS STRIX_RUNTIME_DIRS)
    add_custom_command(TARGET StrixVerseClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/shaders"
            "${RUNTIME_DIR}/shaders"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/assets"
            "${RUNTIME_DIR}/assets"

        COMMENT "Deploying shaders and assets to ${RUNTIME_DIR}"
    )
endforeach()

# --------------------------------------------------
# Visual Studio Source Groups
# --------------------------------------------------

source_group(TREE ${PROJECT_SOURCE_DIR}/third_party PREFIX "Third Party"
    FILES
        third_party/glad/src/glad.c
)