#pragma once

/**
 * Unique identifiers for all screens in the game.
 * Use these with the Engine and ScreenFactory to avoid string comparisons.
 *
 * The flow:
 *   Splash -> MainMenu -> Login -> Connecting -> Continue / WorldBrowser
 *          -> Loading -> Game
 * with Register branching off Login, and Settings and Credits reachable from
 * the main menu. Settings is also reachable from gameplay.
 */
enum class ScreenID
{
    Splash,
    MainMenu,
    Credits,
    Login,
    Register,
    Connecting,
    Continue,
    WorldBrowser,
    Loading,
    Game,
    Settings
};
