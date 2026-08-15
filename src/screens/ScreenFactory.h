#pragma once

#include <memory>
#include "Screen.h"
#include "ScreenIDs.h"

class Engine;

// Forward declarations of screens
class SplashScreen;
class LoginScreen;
class RegisterScreen;
class ConnectingScreen;
class ContinueScreen;
class WorldBrowserScreen;
class LoadingScreen;
class GameScreen;
class SettingsScreen;

/**
 * Factory for creating screens by ID.
 */
class ScreenFactory
{
public:
    /**
     * Create a screen by its ID.
     * @param id The screen ID to create.
     * @param engine Pointer to the engine (passed to screen constructor).
     * @return Unique pointer to the created screen, or nullptr if ID is unknown.
     */
    static std::unique_ptr<Screen> CreateScreen(ScreenID id, Engine* engine);
};