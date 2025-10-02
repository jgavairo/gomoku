#include "scene/Settings.hpp"
#include <iostream>
#include "audio/Volumes.hpp"
#include "util/Preferences.hpp"

constexpr const char* ON_KEY = "sound_on";
constexpr const char* OFF_KEY = "sound_off";

namespace gomoku::scene {

SettingsScene::SettingsScene(Context& ctx)
    : AScene(ctx)
{
    initButtons(defaultBtn_, "default_theme_button", { 1020, 580 }, 0.2f, [this]() { applyTheme("default"); });
    initButtons(halloweenBtn_, "halloween_theme_button", { 1150, 580 }, 0.2f, [this]() { applyTheme("halloween"); });
    initButtons(pastelBtn_, "pastel_theme_button", { 1280, 580 }, 0.2f, [this]() { applyTheme("pastel"); });
    initButtons(backBtn_, "back_button", { 695, 730 }, 1.0f, [this]() { onBackClicked(); });
    initButtons(sfxToggleBtn_, "sound_on", { 1150, 340 }, 0.15f, [this]() { toggleSfx(); });
    initButtons(musicToggleBtn_, "sound_off", { 1150, 460 }, 0.15f, [this]() { toggleMusic(); });
}

void SettingsScene::update(sf::Time& deltaTime)
{
    defaultBtn_.update(deltaTime);
    halloweenBtn_.update(deltaTime);
    pastelBtn_.update(deltaTime);
    backBtn_.update(deltaTime);
    sfxToggleBtn_.update(deltaTime);
    musicToggleBtn_.update(deltaTime);
}

void SettingsScene::render(sf::RenderTarget& target) const
{
    defaultBtn_.render(target);
    halloweenBtn_.render(target);
    pastelBtn_.render(target);
    backBtn_.render(target);
    sfxToggleBtn_.render(target);
    musicToggleBtn_.render(target);
}

void SettingsScene::onThemeChanged()
{
    if (!context_.resourceManager)
        return;
    // Rebind back button (les autres sont rectangles sans texture par défaut)
    if (context_.resourceManager->hasTexture("back_button"))
        backBtn_.setTexture(&context_.resourceManager->getTexture("back_button"));
    if (context_.resourceManager->hasTexture("default_theme_button"))
        defaultBtn_.setTexture(&context_.resourceManager->getTexture("default_theme_button"));
    if (context_.resourceManager->hasTexture("halloween_theme_button"))
        halloweenBtn_.setTexture(&context_.resourceManager->getTexture("halloween_theme_button"));
    if (context_.resourceManager->hasTexture("pastel_theme_button"))
        pastelBtn_.setTexture(&context_.resourceManager->getTexture("pastel_theme_button"));
    // rafraîchit icônes (thémées)
    if (context_.resourceManager->hasTexture(context_.sfxEnabled ? ON_KEY : OFF_KEY))
        sfxToggleBtn_.setTexture(&context_.resourceManager->getTexture(context_.sfxEnabled ? ON_KEY : OFF_KEY));
    if (context_.resourceManager->hasTexture(context_.musicEnabled ? ON_KEY : OFF_KEY))
        musicToggleBtn_.setTexture(&context_.resourceManager->getTexture(context_.musicEnabled ? ON_KEY : OFF_KEY));
}

bool SettingsScene::handleInput(sf::Event& event)
{
    if (context_.window && defaultBtn_.handleInput(event, *context_.window)) { playSfx("ui_click", BUTTON_VOLUME); return true; }
    if (context_.window && halloweenBtn_.handleInput(event, *context_.window)) { playSfx("ui_click", BUTTON_VOLUME); return true; }
    if (context_.window && pastelBtn_.handleInput(event, *context_.window)) { playSfx("ui_click", BUTTON_VOLUME); return true; }
    if (context_.window && backBtn_.handleInput(event, *context_.window)) { playSfx("ui_click", BUTTON_VOLUME); return true; }
    if (context_.window && sfxToggleBtn_.handleInput(event, *context_.window)) { playSfx("ui_click", BUTTON_VOLUME); return true; }
    if (context_.window && musicToggleBtn_.handleInput(event, *context_.window)) { playSfx("ui_click", BUTTON_VOLUME); return true; }
    return false;
}

void SettingsScene::applyTheme(const std::string& themeName)
{
    if (!context_.resourceManager)
        return;
    const bool texOk = context_.resourceManager->setTexturePackage(themeName);
    const bool audOk = context_.resourceManager->setAudioPackage(themeName);

    if (audOk && texOk) {
        context_.theme = themeName;
        context_.themeChanged = true;
        std::string musicPath = std::string("assets/audio/") + themeName + "/menu_theme.ogg";
        playMusic(musicPath.c_str(), true, MUSIC_VOLUME);
        // persiste préférences
        gomoku::util::PreferencesData prefs;
        prefs.theme = context_.theme;
        prefs.sfxEnabled = context_.sfxEnabled;
        prefs.musicEnabled = context_.musicEnabled;
        gomoku::util::Preferences::save(prefs);
        std::cout << "Theme applied: " << themeName << std::endl;
    } else {
        std::cerr << "Failed to apply theme " << themeName << std::endl;
    }
}

void SettingsScene::onBackClicked()
{
    context_.showSettingsMenu = false;
    context_.showMainMenu = true;
}

void SettingsScene::toggleSfx()
{
    context_.sfxEnabled = !context_.sfxEnabled;
    {
        gomoku::util::PreferencesData prefs;
        prefs.theme = context_.theme;
        prefs.sfxEnabled = context_.sfxEnabled;
        prefs.musicEnabled = context_.musicEnabled;
        gomoku::util::Preferences::save(prefs);
    }
    if (context_.resourceManager && context_.resourceManager->hasTexture(context_.sfxEnabled ? ON_KEY : OFF_KEY))
        sfxToggleBtn_.setTexture(&context_.resourceManager->getTexture(context_.sfxEnabled ? ON_KEY : OFF_KEY));
}

void SettingsScene::toggleMusic()
{
    context_.musicEnabled = !context_.musicEnabled;
    if (context_.music) {
        if (context_.musicEnabled) {
            context_.music->setVolume(std::max(0.f, std::min(100.f, context_.musicVolume)));
            if (context_.music->getStatus() != sf::Music::Status::Playing)
                context_.music->play();
        } else {
            context_.music->setVolume(0.f);
        }
    }
    {
        gomoku::util::PreferencesData prefs;
        prefs.theme = context_.theme;
        prefs.sfxEnabled = context_.sfxEnabled;
        prefs.musicEnabled = context_.musicEnabled;
        gomoku::util::Preferences::save(prefs);
    }
    if (context_.resourceManager && context_.resourceManager->hasTexture(context_.musicEnabled ? ON_KEY : OFF_KEY))
        musicToggleBtn_.setTexture(&context_.resourceManager->getTexture(context_.musicEnabled ? ON_KEY : OFF_KEY));
}

} // namespace gomoku::scene


