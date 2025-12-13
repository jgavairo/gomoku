#include "scene/MainMenu.hpp"
#include "audio/Volumes.hpp"
#include "util/GameSaver.hpp"
#include "util/Logger.hpp"
#include <iostream>

namespace gomoku::scene {

MainMenu::MainMenu(Context& context)
    : AScene(context)
{
    std::cout << "[MainMenu] ctor" << std::endl;
    initButtons(playButton_, "play_button", { 111, 696 }, 1.0f, [this]() { onPlayClicked(); });
    initButtons(settingsButton_, "settings_button", { 693, 696 }, 1.0f, [this]() { onSettingsClicked(); });
    initButtons(exitButton_, "exit_button", { 1284, 695.5f }, 1.0f, [this]() { onExitClicked(); });
}

MainMenu::~MainMenu() = default;

bool MainMenu::handleInput(sf::Event& event)
{
    bool consumed = false;
    if (context_.window) {
        auto handleBtn = [&](gomoku::ui::Button& btn) {
            bool c = btn.handleInput(event, *context_.window);
            if (event.type == sf::Event::MouseButtonReleased && c)
                playSfx("ui_click", BUTTON_VOLUME);
            return c;
        };
        consumed = handleBtn(playButton_) || handleBtn(settingsButton_) || handleBtn(exitButton_);
    }
    return consumed;
}

void MainMenu::update(sf::Time& deltaTime)
{
    playButton_.update(deltaTime);
    settingsButton_.update(deltaTime);
    exitButton_.update(deltaTime);
}

void MainMenu::render(sf::RenderTarget& target) const
{
    playButton_.render(target);
    settingsButton_.render(target);
    exitButton_.render(target);
}

void MainMenu::onThemeChanged()
{
    if (!context_.resourceManager)
        return;
    if (context_.resourceManager->hasTexture("play_button"))
        playButton_.setTexture(&context_.resourceManager->getTexture("play_button"));
    if (context_.resourceManager->hasTexture("settings_button"))
        settingsButton_.setTexture(&context_.resourceManager->getTexture("settings_button"));
    if (context_.resourceManager->hasTexture("exit_button"))
        exitButton_.setTexture(&context_.resourceManager->getTexture("exit_button"));
}

void MainMenu::onPlayClicked()
{
    // WAINTING FIX >>> if save exist
    if (gomoku::util::GameSaver::hasSave()) {
        LOG_INFO("SAVE EXIST");
        context_.inGame = false;
        context_.showLoadGameMenu = true;
        context_.from_loadGame = true;
    }
    else
    {
        LOG_INFO("SAVE NOT EXIST");
        context_.inGame = false;
        context_.showGameSelectMenu = true;
        context_.from_loadGame = false;
    }
    // sinon afficher direct //gameselectmenu

    // keep menu music by default
}

void MainMenu::onSettingsClicked()
{
    context_.showSettingsMenu = true;
}

void MainMenu::onExitClicked()
{
    context_.shouldQuit = true;
}

} // namespace gomoku::scene