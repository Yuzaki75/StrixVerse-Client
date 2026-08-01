#include "ScreenFactory.h"

#include "SplashScreen.h"
#include "LoginScreen.h"
#include "RegisterScreen.h"
#include "ContinueScreen.h"
#include "WorldBrowserScreen.h"
#include "LoadingScreen.h"
#include "GameScreen.h"
#include "SettingsScreen.h"

std::unique_ptr<Screen> ScreenFactory::CreateScreen(ScreenID id, Engine* engine)
{
    switch (id)
    {
        case ScreenID::Splash:
            return std::make_unique<SplashScreen>(engine);
        case ScreenID::Login:
            return std::make_unique<LoginScreen>(engine);
        case ScreenID::Register:
            return std::make_unique<RegisterScreen>(engine);
        case ScreenID::Continue:
            return std::make_unique<ContinueScreen>(engine);
        case ScreenID::WorldBrowser:
            return std::make_unique<WorldBrowserScreen>(engine);
        case ScreenID::Loading:
            return std::make_unique<LoadingScreen>(engine);
        case ScreenID::Game:
            return std::make_unique<GameScreen>(engine);
        case ScreenID::Settings:
            return std::make_unique<SettingsScreen>(engine);
        default:
            return nullptr;
    }
}