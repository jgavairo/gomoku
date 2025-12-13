#include "gomoku/application/SessionController.hpp"
#include "gomoku/ai/MinimaxSearchEngine.hpp"

namespace gomoku {

SessionController::SessionController(const RuleSet& rules, Controller black, Controller white)
    : rules_(rules)
    , gameService_(std::make_unique<application::GameService>(
          std::make_unique<ai::MinimaxSearchEngine>()))
    , black_(black)
    , white_(white)
{
    gameService_->startNewGame(rules_);
}

GameSnapshot SessionController::snapshot() const
{
    const auto& b = gameService_->getBoard();
    auto captures = b.capturedPairs();
    return GameSnapshot {
        .view = &b,
        .lastMove = last_,
        .toPlay = b.toPlay(),
        .captures = { captures.black, captures.white },
        .status = b.status(),
        .moveCount = b.moveCount(),
        .moveHistory = gameService_->getMoveHistory(),
        .redoHistory = gameService_->getRedoHistory()
    };
}

void SessionController::setController(Player side, Controller c)
{
    if (side == Player::Black)
        black_ = c;
    else
        white_ = c;
}

Controller SessionController::controller(Player side) const
{
    return (side == Player::Black) ? black_ : white_;
}

GamePlayResult SessionController::playHuman(Pos p)
{
    Move m { p, gameService_->getCurrentPlayer() };
    std::string why;
    if (!gameService_->isMoveLegal(m, &why))
        return { false, why, std::nullopt, std::nullopt };
    auto res = gameService_->makeMove(m);
    if (!res.success)
        return { false, res.error, std::nullopt, std::nullopt };
    last_ = p;
    return { true, {}, m, std::nullopt };
}

GamePlayResult SessionController::playAI(int timeMs)
{
    SearchStats st {};
    auto bm = gameService_->getAIMove(timeMs, &st);
    if (!bm)
        return { false, "No AI move", std::nullopt, st };
    auto res = gameService_->makeMove(*bm);
    if (!res.success)
        return { false, res.error, std::nullopt, st };
    last_ = bm->pos;
    return { true, {}, bm, st };
}

GamePlayResult SessionController::undo(int halfMoves)
{
    bool any = false;
    for (int i = 0; i < halfMoves; ++i) {
        if (!gameService_->undo())
            break;
        any = true;
    }
    if (any) {
        auto m = gameService_->getBoard().lastMove();
        if (m)
            last_ = m->pos;
        else
            last_.reset();
        return { true, {}, std::nullopt, std::nullopt };
    }
    return { false, "No moves to undo", std::nullopt, std::nullopt };
}

GamePlayResult SessionController::redo(int halfMoves)
{
    bool any = false;
    for (int i = 0; i < halfMoves; ++i) {
        if (!gameService_->redo())
            break;
        any = true;
    }
    if (any) {
        auto m = gameService_->getBoard().lastMove();
        if (m)
            last_ = m->pos;
        return { true, {}, m, std::nullopt };
    }
    return { false, "No moves to redo", std::nullopt, std::nullopt };
}

void SessionController::reset(Player start)
{
    gameService_->startNewGame(rules_);
    if (start == Player::White && gameService_->getCurrentPlayer() != Player::White) {
        // Hack: play a null move concept not available; defer future enhancement.
    }
    last_.reset();
}

GamePlayResult SessionController::load(const std::vector<uint8_t>& data)
{
    bool ok = gameService_->loadGame(data);
    if (ok) {
        auto m = gameService_->getBoard().lastMove();
        if (m)
            last_ = m->pos;
        else
            last_.reset();
        return { true, {}, std::nullopt, std::nullopt };
    }
    return { false, "Failed to load game data", std::nullopt, std::nullopt };
}

GamePlayResult SessionController::hint(int timeMs) const
{
    SearchStats st {};
    auto mv = gameService_->getAIMove(timeMs, &st);
    if (!mv)
        return { false, "No hint available", std::nullopt, st };
    return { true, {}, mv, st };
}

} // namespace gomoku
