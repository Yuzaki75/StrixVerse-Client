cmake_minimum_required(VERSION 3.20)

project(StrixVerseClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------
# Compiler
# --------------------------------------------------

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# --------------------------------------------------
# Find Packages
# --------------------------------------------------

find_package(SDL3 CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# --------------------------------------------------
# Source Files
# --------------------------------------------------

set(SOURCES
    src/main.cpp

    # Core
    src/core/Application.cpp
    src/core/Config.cpp
    src/core/Engine.cpp
    src/core/Logger.cpp
    src/core/Time.cpp
    src/core/Window.cpp
)

# --------------------------------------------------
# Header Files
# --------------------------------------------------

set(HEADERS
    src/core/Application.h
    src/core/Config.h
    src/core/Engine.h
    src/core/Logger.h
    src/core/Time.h
    src/core/Window.h
)

# --------------------------------------------------
# Executable
# --------------------------------------------------

add_executable(StrixVerseClient
    ${SOURCES}
    ${HEADERS}
    src/graphics/Texture.cpp
    src/graphics/OpenGL/GLContext.cpp
    src/graphics/OpenGL/GLBuffer.h
)

# --------------------------------------------------
# Include Directories
# --------------------------------------------------

target_include_directories(StrixVerseClient PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/third_party
)

# --------------------------------------------------
# Link Libraries
# --------------------------------------------------

target_link_libraries(StrixVerseClient PRIVATE
    SDL3::SDL3
    Freetype::Freetype
    glm::glm
)

# --------------------------------------------------
# Output Directory
# --------------------------------------------------

set_target_properties(StrixVerseClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
)

# --------------------------------------------------
# Create Runtime Folders
# --------------------------------------------------

add_custom_command(TARGET StrixVerseClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/configs"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/logs"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/cache"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/screenshots"
)