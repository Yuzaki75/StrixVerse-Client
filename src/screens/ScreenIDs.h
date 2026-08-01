#pragma once

/**
 * Unique identifiers for all screens in the game.
 * Use these with ScreenManager and ScreenFactory to avoid string comparisons.
 */
enum class ScreenID
{
    Splash,
    Login,
    Register,
    Continue,
    WorldBrowser,
    Loading,
    Game,
    Settings
};