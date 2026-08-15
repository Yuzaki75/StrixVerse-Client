#include "ScreenFactory.h"

#include "ConnectingScreen.h"
#include "ContinueScreen.h"
#include "GameScreen.h"
#include "LoadingScreen.h"
#include "LoginScreen.h"
#include "RegisterScreen.h"
#include "SettingsScreen.h"
#include "SplashScreen.h"
#include "WorldBrowserScreen.h"

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
        case ScreenID::Connecting:
            return std::make_unique<ConnectingScreen>(engine);
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
    }

    return nullptr;
}
