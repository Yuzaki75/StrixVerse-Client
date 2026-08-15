#pragma once

/**
 * Unique identifiers for all screens in the game.
 * Use these with the Engine and ScreenFactory to avoid string comparisons.
 *
 * The flow the design defines:
 *   Splash -> Login -> Connecting -> Continue / WorldBrowser -> Loading -> Game
 * with Register branching off Login and back.
 */
enum class ScreenID
{
    Splash,
    Login,
    Register,
    Connecting,
    Continue,
    WorldBrowser,
    Loading,
    Game,
    Settings
};
