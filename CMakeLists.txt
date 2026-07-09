cmake_minimum_required(VERSION 3.20)


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


# --------------------------------------------------
# Find Packages
# --------------------------------------------------


find_package(SDL3 CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
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
    src/core/Game.cpp                                # NEW
    src/core/Logger.cpp
    src/core/ThreadPool.cpp                          # NEW
    src/core/Time.cpp
    src/core/Version.cpp                             # NEW
    src/core/Window.cpp
)


# --------------------------------------------------
# Header Files
# --------------------------------------------------


set(HEADERS
    src/core/Application.h
    src/core/Config.h
    src/core/Engine.h
    src/core/Game.h                                  # NEW
    src/core/Logger.h
    src/core/ServiceLocator.h                        # NEW (header-only)
    src/core/ThreadPool.h                            # NEW
    src/core/Time.h
    src/core/Version.h                               # NEW
    src/core/Window.h

    src/graphics/OpenGL/GLBuffer.h                   # NEW: moved here from add_executable
)


# --------------------------------------------------
# Executable
# --------------------------------------------------


add_executable(StrixVerseClient
    ${SOURCES}
    ${HEADERS}
    src/graphics/Texture.cpp
    src/graphics/OpenGL/GLContext.cpp
    resources/client.rc
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
    Threads::Threads                                 # NEW
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
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/build"      # NEW
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/build"    # NEW
)


# --------------------------------------------------
# Create Runtime Folders
# --------------------------------------------------


add_custom_command(TARGET StrixVerseClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/configs"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/logs"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/screenshots"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/build/cache"
)