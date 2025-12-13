#include "scene/GameSelect.hpp"
#include "audio/Volumes.hpp"
#include "util/Logger.hpp"

namespace gomoku::scene {

GameSelectScene::GameSelectScene(Context& ctx)
    : AScene(ctx)
{
    LOG_INFO("GameSelect: Game selection scene initialization");

    initButtons(playerVsPlayerButton_, "vs_player_button", { 111, 696 }, 1.0f, [this]() { onPlayerVsPlayerClicked(); });
    initButtons(playerVsBotButton_, "vs_ai_button", { 693, 696 }, 1.0f, [this]() { onPlayerVsBotClicked(); });
    initButtons(backButton_, "back_button", { 1284, 695.5f }, 1.0f, [this]() { onBackClicked(); });
}

void GameSelectScene::update(sf::Time& deltaTime)
{
    playerVsPlayerButton_.update(deltaTime);
    playerVsBotButton_.update(deltaTime);
    backButton_.update(deltaTime);
}

void GameSelectScene::render(sf::RenderTarget& target) const
{
    playerVsPlayerButton_.render(target);
    playerVsBotButton_.render(target);
    backButton_.render(target);
}

void GameSelectScene::onThemeChanged()
{
    if (!context_.resourceManager)
        return;

    LOG_DEBUG("GameSelect: Texture update after theme change");
    if (context_.resourceManager->hasTexture("vs_player_button"))
        playerVsPlayerButton_.setTexture(&context_.resourceManager->getTexture("vs_player_button"));
    if (context_.resourceManager->hasTexture("vs_ai_button"))
        playerVsBotButton_.setTexture(&context_.resourceManager->getTexture("vs_ai_button"));
    if (context_.resourceManager->hasTexture("back_button"))
        backButton_.setTexture(&context_.resourceManager->getTexture("back_button"));
}

bool GameSelectScene::handleInput(sf::Event& event)
{
    bool consumed = false;
    if (context_.window) {
        auto handleBtn = [&](gomoku::ui::Button& btn) {
            bool c = btn.handleInput(event, *context_.window);
            if (event.type == sf::Event::MouseButtonReleased && c) {
                playSfx("ui_click", BUTTON_VOLUME);
                LOG_DEBUG("GameSelect: Button click detected");
            }
            return c;
        };
        consumed = handleBtn(playerVsPlayerButton_) || handleBtn(playerVsBotButton_) || handleBtn(backButton_);
    }
    return consumed;
}

void GameSelectScene::onPlayerVsPlayerClicked()
{
    LOG_INFO("GameSelect: Player vs Player mode selected");
    context_.vsAi = false;
    context_.inGame = true;
    {
        std::string path = std::string("assets/audio/") + context_.theme + "/ingame_theme.ogg";
        LOG_DEBUG("GameSelect: Starting game music: " + path);
        playMusic(path.c_str(), true, MUSIC_VOLUME);
    }
}

void GameSelectScene::onPlayerVsBotClicked()
{
    LOG_INFO("GameSelect: Player vs AI mode selected");
    context_.vsAi = true;
    context_.inGame = true;
    {
        std::string path = std::string("assets/audio/") + context_.theme + "/ingame_theme.ogg";
        LOG_DEBUG("GameSelect: Starting game music: " + path);
        playMusic(path.c_str(), true, MUSIC_VOLUME);
    }
}

void GameSelectScene::onBackClicked()
{
    if (context_.from_loadGame)
    {
        LOG_INFO("GameSelect: Back to load game menu");
        context_.showGameSelectMenu = false;
        context_.inGame = false;
        context_.showLoadGameMenu = true;
    }
    else
    {
        LOG_INFO("GameSelect: Back to main menu");
        context_.showGameSelectMenu = false;
        context_.inGame = false;
        context_.showMainMenu = true;
    }

}

} // namespace gomoku::scene