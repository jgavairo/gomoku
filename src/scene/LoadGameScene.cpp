#include "scene/LoadGameScene.hpp"
#include "audio/Volumes.hpp"
#include "util/Logger.hpp"

namespace gomoku::scene {

LoadGameScene::LoadGameScene(Context& ctx)
    : AScene(ctx)
{
    LOG_INFO("Load or new game scene initialization");

    initButtons(continueButton_, "vs_player_button", { 111, 696 }, 1.0f, [this]() { onContinueClicked(); });
    initButtons(newGameButton_, "vs_ai_button", { 693, 696 }, 1.0f, [this]() { onNewGameClicked(); });
    initButtons(backButton_, "back_button", { 1284, 695.5f }, 1.0f, [this]() { onBackClicked(); });
}

void LoadGameScene::update(sf::Time& deltaTime)
{
    continueButton_.update(deltaTime);
    newGameButton_.update(deltaTime);
    backButton_.update(deltaTime);
}

void LoadGameScene::render(sf::RenderTarget& target) const
{
    continueButton_.render(target);
    newGameButton_.render(target);
    backButton_.render(target);
}

void LoadGameScene::onThemeChanged()
{
    if (!context_.resourceManager)
        return;

    LOG_DEBUG("GameSelect: Texture update after theme change");
    if (context_.resourceManager->hasTexture("vs_player_button"))
        continueButton_.setTexture(&context_.resourceManager->getTexture("vs_player_button"));
    if (context_.resourceManager->hasTexture("vs_ai_button"))
        newGameButton_.setTexture(&context_.resourceManager->getTexture("vs_ai_button"));
    if (context_.resourceManager->hasTexture("back_button"))
        backButton_.setTexture(&context_.resourceManager->getTexture("back_button"));
}

bool LoadGameScene::handleInput(sf::Event& event)
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
        consumed = handleBtn(continueButton_) || handleBtn(newGameButton_) || handleBtn(backButton_);
    }
    return consumed;
}

void LoadGameScene::onContinueClicked()
{
    //WAITING IMPLEMENT
}

void LoadGameScene::onNewGameClicked()
{
    //WAITING IMPLEMENT
}

void LoadGameScene::onBackClicked()
{
    LOG_INFO("LoadGame: Back to main menu");
    context_.showGameSelectMenu = false;
    context_.inGame = false;
    context_.showMainMenu = true;
}

} // namespace gomoku::scene