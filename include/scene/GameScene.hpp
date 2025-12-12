#pragma once
#include "gomoku/application/SessionController.hpp"
#include "gomoku/core/Types.hpp"
#include "gui/GameBoardRenderer.hpp"
#include "scene/AScene.hpp"
#include "ui/Button.hpp"
#include <SFML/Graphics.hpp>
#include <string>

namespace gomoku::scene {

class GameScene : public AScene {
public:
    GameScene(Context& context, bool vsAi);
    ~GameScene();

    bool handleInput(sf::Event& event) override;
    void update(sf::Time& deltaTime) override;
    void render(sf::RenderTarget& target) const override;
    void onThemeChanged() override;

private:
    void onQuitGameClicked();
    void onHintClicked();
    void onUndoClicked();
    void onRedoClicked();

    std::optional<gomoku::Pos> hintPos_;
    bool hintEnabled_ = false;
    // Sprite dédié pour afficher le pion helper
    mutable sf::Sprite helperSprite_;
    mutable bool helperSpriteReady_ = false;

    // Survol: pion du joueur courant à l'emplacement cliquable sous la souris
    std::optional<gomoku::Pos> hoverPos_;
    bool showHover_ = false;
    mutable sf::Sprite hoverSpriteWhite_;
    mutable sf::Sprite hoverSpriteBlack_;
    mutable bool hoverSpritesReady_ = false;


    bool vsAi_;
    gomoku::ui::Button quitGameButton_;
    gomoku::ui::Button hintButton_;
    gomoku::ui::Button undoButton_;
    gomoku::ui::Button redoButton_;
    gomoku::gui::GameBoardRenderer boardRenderer_;
    gomoku::SessionController gameSession_;
    gomoku::RuleSet rules_;

    mutable sf::Font font_;
    mutable bool fontOk_ = false;
    mutable sf::Text hudText_;
    mutable int lastAiMs_ = -1;
    bool pendingAi_ = false;
    mutable bool framePresented_ = false;
    bool aiThinking_ = false;
    mutable sf::Clock inputClock_;
    sf::Time blockBoardClicksUntil_ {};
    std::string illegalMsg_;
    mutable sf::Clock illegalClock_;
    mutable sf::Text msgText_;
};

} // namespace gomoku::scene
