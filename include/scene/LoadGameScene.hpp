#pragma once
#include "scene/AScene.hpp"
#include "ui/Button.hpp"

namespace gomoku::scene {

    class LoadGameScene : public AScene {
    public:
        explicit LoadGameScene(Context& ctx);
        ~LoadGameScene() override = default;

        void update(sf::Time& deltaTime) override;
        void render(sf::RenderTarget& target) const override;
        bool handleInput(sf::Event& event) override;
        void onThemeChanged() override;

    private:
        void onContinueClicked();
        void onNewGameClicked();
        void onBackClicked();

        gomoku::ui::Button continueButton_;
        gomoku::ui::Button newGameButton_;
        gomoku::ui::Button backButton_;
    };
} // namespace gomoku::scene
