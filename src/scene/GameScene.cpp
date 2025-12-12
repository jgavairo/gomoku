#include "scene/GameScene.hpp"
#include "audio/Volumes.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

namespace gomoku::scene {

GameScene::GameScene(Context& context, bool vsAi)
    : AScene(context)
    , vsAi_(vsAi)
    , gameSession_(gomoku::SessionController())
{
    // Initialisation du bouton Back
    initButtons(quitGameButton_, "quit_game_button", { 50, 900 }, 0.5f, [this]() { onQuitGameClicked(); });

    initButtons(hintButton_, "hint_button", { 1780, 50 }, 0.075f, [this]() { onHintClicked(); });

    initButtons(undoButton_, "undo", { 1780, 150 }, 0.125f, [this]() { onUndoClicked(); });

    // Initialisation du renderer de plateau
    if (context_.resourceManager) {
        boardRenderer_.setTextures(
            context_.resourceManager->getTexture("board"),
            context_.resourceManager->getTexture("pawn1"),
            context_.resourceManager->getTexture("pawn2"),
            context_.resourceManager->getTexture("pawn_hint"));
        // Sprite helper dédié (optionnel)
        if (context_.resourceManager->hasTexture("pawn_hint")) {
            helperSprite_.setTexture(context_.resourceManager->getTexture("pawn_hint"));
            helperSpriteReady_ = true;
        }
        // Sprites de survol (utilisent les pions existants)
        hoverSpriteWhite_.setTexture(context_.resourceManager->getTexture("pawn1"));
        hoverSpriteBlack_.setTexture(context_.resourceManager->getTexture("pawn2"));
        hoverSpritesReady_ = true;
    }

    rules_ = gomoku::RuleSet();

    // Configure controllers according to mode
    if (vsAi_) {
        // Default SessionController ctor is Black:Human, White:AI; keep as is for now
        gameSession_.setController(gomoku::Player::Black, gomoku::Controller::AI);
        gameSession_.setController(gomoku::Player::White, gomoku::Controller::Human);
        pendingAi_ = true;
        framePresented_ = false; // wait for next render() before starting AI
    } else {
        gameSession_.setController(gomoku::Player::Black, gomoku::Controller::Human);
        gameSession_.setController(gomoku::Player::White, gomoku::Controller::Human);
    }

    // Initial board binding (renderer now reads directly from IBoardView)
    {
        auto snap = gameSession_.snapshot();
        const_cast<gomoku::gui::GameBoardRenderer&>(boardRenderer_).setBoardView(snap.view);
    }

    // HUD setup (lazy font load)
    fontOk_ = font_.loadFromFile("assets/ui/DejaVuSans.ttf");
    if (fontOk_) {
        hudText_.setFont(font_);
        hudText_.setCharacterSize(20);
        hudText_.setFillColor(sf::Color::White);
        hudText_.setPosition(20.f, 20.f);
        msgText_.setFont(font_);
        msgText_.setCharacterSize(20);
        msgText_.setFillColor(sf::Color(255, 80, 80));
        msgText_.setPosition(20.f, 48.f);
    }
}

GameScene::~GameScene() = default;

void GameScene::onThemeChanged()
{
    if (!context_.resourceManager)
        return;
    // Rebind textures sur le renderer
    const_cast<gomoku::gui::GameBoardRenderer&>(boardRenderer_).setTextures(context_.resourceManager->getTexture("board"), context_.resourceManager->getTexture("pawn1"), context_.resourceManager->getTexture("pawn2"), context_.resourceManager->getTexture("pawn_hint"));
    // Rebind texture helper si disponible
    if (context_.resourceManager->hasTexture("pawn_hint")) {
        helperSprite_.setTexture(context_.resourceManager->getTexture("pawn_hint"));
        helperSpriteReady_ = true;
    } else {
        helperSpriteReady_ = false;
    }
}

bool GameScene::handleInput(sf::Event& event)
{
    if (context_.window && quitGameButton_.handleInput(event, *context_.window))
        return true;

    if (context_.window && hintButton_.handleInput(event, *context_.window))
        return true;

    if (context_.window && undoButton_.handleInput(event, *context_.window))
        return true;
        
    // Placement des pions sur clic souris
    if (context_.window && event.type == sf::Event::MouseMoved) {
        const auto size = context_.window->getSize();
        const float centerX = static_cast<float>(size.x) * 0.5f;
        const float centerY = static_cast<float>(size.y) * 0.5f;
        const float tileW = std::min(static_cast<float>(size.x) * 0.8f / 18.f,
            static_cast<float>(size.y) * 0.8f * 2.f / 18.f);
        const float tileH = tileW * 0.5f;
        sf::Vector2f mp = context_.window->mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
        const float X = mp.x - centerX;
        const float Y = mp.y - centerY;
        const float u = (Y / (tileH * 0.5f) + X / (tileW * 0.5f)) * 0.5f;
        const float v = (Y / (tileH * 0.5f) - X / (tileW * 0.5f)) * 0.5f;
        const int N = 19;
        const int C = (N - 1) / 2;
        int i = static_cast<int>(std::lround(u)) + C;
        int j = static_cast<int>(std::lround(v)) + C;
        i = std::max(0, std::min(18, i));
        j = std::max(0, std::min(18, j));
        // Validation de proximité identique au clic
        const float ui = static_cast<float>(i - C);
        const float vj = static_cast<float>(j - C);
        const float snappedX = centerX + (ui - vj) * (tileW * 0.5f);
        const float snappedY = centerY + (ui + vj) * (tileH * 0.5f);
        const float dx = snappedX - mp.x;
        const float dy = snappedY - mp.y;
        const float maxDist = std::min(tileW, tileH) * 0.9f;
        if ((dx * dx + dy * dy) <= (maxDist * maxDist)) {
            hoverPos_ = gomoku::Pos { (uint8_t)i, (uint8_t)j };
            showHover_ = true;
        } else {
            hoverPos_.reset();
            showHover_ = false;
        }
        return false;
    }


    if (context_.window && event.type == sf::Event::MouseButtonPressed) {
        auto btn = event.mouseButton.button;
        if (btn == sf::Mouse::Left || btn == sf::Mouse::Right) {
            // Allow UI clicks, but ignore board placement while AI is thinking or during cooldown
            if (aiThinking_ || pendingAi_ || (blockBoardClicksUntil_ > sf::Time::Zero && inputClock_.getElapsedTime() < blockBoardClicksUntil_)) {
                return true; // consume to avoid buffering a move
            }
            const auto size = context_.window->getSize();
            const float centerX = static_cast<float>(size.x) * 0.5f;
            const float centerY = static_cast<float>(size.y) * 0.5f;

            const int N = 19;
            const int C = (N - 1) / 2;

            const float tileW = std::min(static_cast<float>(size.x) * 0.8f / 18.f,
                static_cast<float>(size.y) * 0.8f * 2.f / 18.f);
            const float tileH = tileW * 0.5f;

            sf::Vector2f mp = context_.window->mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
            const float X = mp.x - centerX;
            const float Y = mp.y - centerY;

            const float u = (Y / (tileH * 0.5f) + X / (tileW * 0.5f)) * 0.5f;
            const float v = (Y / (tileH * 0.5f) - X / (tileW * 0.5f)) * 0.5f;

            int i = static_cast<int>(std::lround(u)) + C;
            int j = static_cast<int>(std::lround(v)) + C;

            i = std::max(0, std::min(18, i));
            j = std::max(0, std::min(18, j));

            // Validation de proximité
            const float ui = static_cast<float>(i - C);
            const float vj = static_cast<float>(j - C);
            const float snappedX = centerX + (ui - vj) * (tileW * 0.5f);
            const float snappedY = centerY + (ui + vj) * (tileH * 0.5f);
            const float dx = snappedX - mp.x;
            const float dy = snappedY - mp.y;
            const float maxDist = std::min(tileW, tileH) * 0.9f; // Zone cliquable étendue (était 0.35f)

            if ((dx * dx + dy * dy) <= (maxDist * maxDist)) {
                // Fixer la position de survol valide
                hoverPos_ = gomoku::Pos { (uint8_t)i, (uint8_t)j };
                showHover_ = true;
                if (btn == sf::Mouse::Left) {
                    // If it's human's turn, try to play
                    auto before = gameSession_.snapshot();
                    if (gameSession_.controller(before.toPlay) == gomoku::Controller::Human) {
                        auto result = gameSession_.playHuman(gomoku::Pos { (uint8_t)i, (uint8_t)j });
                        if (result.ok) {
                            // Après un coup posé, on masque le hover
                            hoverPos_.reset();
                            showHover_ = false;
                            if (hintEnabled_) {
                                hintEnabled_ = false;
                                hintPos_.reset();
                            }
                            auto snap1 = gameSession_.snapshot();
                            const_cast<gomoku::gui::GameBoardRenderer&>(boardRenderer_).setBoardView(snap1.view);
                            // SFX: pose de pion selon couleur jouée
                            playSfx(snap1.toPlay == gomoku::Player::Black ? "place_white" : "place_black", PLACE_PAWN_VOLUME);
                            // Capture détectée ? compare les paires capturées avant/après
                            if (snap1.captures.first > before.captures.first || snap1.captures.second > before.captures.second) {
                                playSfx("capture", CAPTURE_VOLUME);
                            }
                            // Check end of game
                            if (snap1.status != gomoku::GameStatus::Ongoing)
                                return true;
                            // If vs AI and AI to play, schedule it with a one-frame delay
                            if (vsAi_ && gameSession_.controller(snap1.toPlay) == gomoku::Controller::AI) {
                                pendingAi_ = true;
                                framePresented_ = false; // wait for next render() before starting AI
                            }
                        } else {
                            // Show reason why illegal
                            illegalMsg_ = result.why;
                            if (fontOk_) {
                                msgText_.setString(illegalMsg_);
                                illegalClock_.restart();
                            }
                        }
                    }
                }
            }
        }
        return true;
    }
    return false;
}

void GameScene::update(sf::Time& deltaTime)
{
    quitGameButton_.update(deltaTime);
    hintButton_.update(deltaTime);
    undoButton_.update(deltaTime);

    // Run pending AI move only after at least one frame has been presented
    if (pendingAi_ && framePresented_) {
        pendingAi_ = false;
        aiThinking_ = true;
        auto before = gameSession_.snapshot();
        auto t0 = std::chrono::steady_clock::now();
        auto aiResult = gameSession_.playAI(/* budget in ms */ 500);
        auto t1 = std::chrono::steady_clock::now();
        lastAiMs_ = (int)std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        auto snap = gameSession_.snapshot();
        const_cast<gomoku::gui::GameBoardRenderer&>(boardRenderer_).setBoardView(snap.view);
        // Désactiver l'overlay helper après le coup IA
        if (hintEnabled_) {
            hintEnabled_ = false;
            hintPos_.reset();
        }
        // SFX: pose de pion IA (même logique que côté humain: on déduit la couleur depuis toPlay)
        playSfx(snap.toPlay == gomoku::Player::Black ? "place_white" : "place_black", PLACE_PAWN_VOLUME);
        // Capture détectée côté IA ?
        if (snap.captures.first > before.captures.first || snap.captures.second > before.captures.second) {
            playSfx("capture", CAPTURE_VOLUME);
        }
        aiThinking_ = false;
        // Start short cooldown to swallow any clicks pressed during AI thinking
        inputClock_.restart();
        blockBoardClicksUntil_ = sf::milliseconds(120);
    }
}

void GameScene::render(sf::RenderTarget& target) const
{
    // Fond de jeu
    if (context_.resourceManager && context_.resourceManager->hasTexture("gameBackground")) {
        sf::Sprite bg(context_.resourceManager->getTexture("gameBackground"));
        bg.setScale(sf::Vector2f(1.0f, 1.0f));
        target.draw(bg);
    }
    // Plateau (cible est la fenêtre; cast suffisant ici)
    const_cast<gomoku::gui::GameBoardRenderer&>(boardRenderer_).render(static_cast<sf::RenderWindow&>(target));
    // Helper pawn overlay
    if (hintEnabled_ && hintPos_ && helperSpriteReady_) {
        const auto size = context_.window->getSize();
        const float centerX = static_cast<float>(size.x) * 0.5f;
        const float centerY = static_cast<float>(size.y) * 0.5f;
        const float tileW = std::min(static_cast<float>(size.x) * 0.8f / 18.f,
            static_cast<float>(size.y) * 0.8f * 2.f / 18.f);
        const float tileH = tileW * 0.5f;

        const int i = hintPos_->x;
        const int j = hintPos_->y;
        const auto p = gomoku::gui::GameBoardRenderer::isoToScreen(i, j, tileW, tileH, centerX, centerY);

        float pawnSize = tileW * 0.6f;
        float scale = pawnSize / static_cast<float>(helperSprite_.getTexture()->getSize().x);
        helperSprite_.setPosition({ p.x - pawnSize * 0.5f, p.y - pawnSize * 0.5f - 5.f });
        helperSprite_.setScale(scale, scale);
        target.draw(helperSprite_);
    }

    // Hover pawn (pion du joueur courant, translucide)
    if (showHover_ && hoverPos_ && hoverSpritesReady_) {
        auto snap = gameSession_.snapshot();
        const bool whiteToPlay = (snap.toPlay == gomoku::Player::White);
        sf::Sprite& hov = whiteToPlay ? hoverSpriteWhite_ : hoverSpriteBlack_;

        const auto size = context_.window->getSize();
        const float centerX = static_cast<float>(size.x) * 0.5f;
        const float centerY = static_cast<float>(size.y) * 0.5f;
        const float tileW = std::min(static_cast<float>(size.x) * 0.8f / 18.f,
            static_cast<float>(size.y) * 0.8f * 2.f / 18.f);
        const float tileH = tileW * 0.5f;
        const int i = hoverPos_->x;
        const int j = hoverPos_->y;
        const auto p = gomoku::gui::GameBoardRenderer::isoToScreen(i, j, tileW, tileH, centerX, centerY);

        float pawnSize = tileW * 0.6f;
        float scale = pawnSize / static_cast<float>(hov.getTexture()->getSize().x);
        hov.setPosition({ p.x - pawnSize * 0.5f, p.y - pawnSize * 0.5f - 5.f });
        hov.setScale(scale, scale);
        auto old = hov.getColor();
        sf::Color c = old;
        c.a = 110; // léger transparent
        hov.setColor(c);
        target.draw(hov);
        hov.setColor(old);
    }
    // UI
    quitGameButton_.render(target);
    hintButton_.render(target);
    undoButton_.render(target);
    // HUD: toPlay, captures, last move, AI time
    auto snap = gameSession_.snapshot();
    if (fontOk_) {
        char buf[128];
        auto caps = snap.captures;
        std::snprintf(buf, sizeof(buf), "To play: %s   Captures ●:%d ○:%d   Moves: %d%s%s%s",
            (snap.toPlay == gomoku::Player::Black ? "● Black" : "○ White"),
            caps.first, caps.second,
            snap.moveCount,
            (lastAiMs_ >= 0 ? "   |  AI:" : ""),
            (lastAiMs_ >= 0 ? " ms" : ""),
            "");
        std::string line(buf);
        if (snap.lastMove) {
            line += "   |  Last: ";
            line += std::to_string((int)snap.lastMove->x);
            line += ",";
            line += std::to_string((int)snap.lastMove->y);
        }
        if (lastAiMs_ >= 0) {
            line += "   |  AI time: ";
            line += std::to_string(lastAiMs_);
            line += "ms";
        }
        hudText_.setString(line);
        target.draw(hudText_);
    }

    // Illegal message (timed)
    if (fontOk_ && !illegalMsg_.empty() && illegalClock_.getElapsedTime().asSeconds() < 2.0f) {
        target.draw(msgText_);
    }

    // Endgame banner / screen
    if (snap.status != gomoku::GameStatus::Ongoing) {
        // Affiche un écran de victoire plein écran selon le vainqueur
        auto winner = gomoku::opponent(snap.toPlay);
        const char* key = (winner == gomoku::Player::Black) ? "black_win" : "white_win";
        if (context_.resourceManager && context_.resourceManager->hasTexture(key)) {
            sf::Sprite winBg(context_.resourceManager->getTexture(key));
            auto win = context_.window->getSize();
            auto texSize = winBg.getTexture()->getSize();
            float scaleX = static_cast<float>(win.x) / static_cast<float>(texSize.x);
            float scaleY = static_cast<float>(win.y) / static_cast<float>(texSize.y);
            float scale = std::max(scaleX, scaleY);
            winBg.setScale(scale, scale);
            winBg.setPosition(
                (static_cast<float>(win.x) - static_cast<float>(texSize.x) * scale) * 0.5f,
                (static_cast<float>(win.y) - static_cast<float>(texSize.y) * scale) * 0.5f);
            target.draw(winBg);
        }
    }

    // Mark that at least one frame has been presented; allows AI to start next update
    framePresented_ = true;
}

void GameScene::onUndoClicked()
{
    if (vsAi_) {
        if (gameSession_.snapshot().moveCount < 2)
            return;
        std::cout << "Undo clicked" << std::endl;
        gameSession_.undo(2);
        hintEnabled_ = false;
        hintPos_.reset();
    } else if (!vsAi_) {
        std::cout << "Undo clicked" << std::endl;
        gameSession_.undo(1);
        hintEnabled_ = false;
        hintPos_.reset();
    }
}

void GameScene::onQuitGameClicked()
{
    //Save plateau si game non finie WAITING FIX
    //
    //
    //
    printf("on Quit Game Clicked\n");
    context_.inGame = false;
    context_.showMainMenu = true;
    std::string musicPath = std::string("assets/audio/") + context_.theme + "/menu_theme.ogg";
    playMusic(musicPath.c_str(), true, MUSIC_VOLUME);
}

void GameScene::onHintClicked()
{
    std::cout << "Hint clicked" << std::endl;
    // Toggle: si déjà affiché, on masque
    if (hintEnabled_) {
        return;
    }

    auto result = gameSession_.hint(500);
    if (!result.mv)
        return;
    std::cout << "Hint: " << result.mv->pos << std::endl;
    hintPos_ = result.mv->pos;
    hintEnabled_ = true;
    if (result.stats) {
        std::cout << "  Depth: " << result.stats->depthReached
                  << ", Nodes: " << result.stats->nodes
                  << ", TT hits: " << result.stats->ttHits << std::endl;
    }
}

} // namespace gomoku::scene