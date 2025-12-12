#include "gomoku/application/GameService.hpp"
#include "gomoku/core/Board.hpp"
#include <algorithm>
#include <stdexcept>

namespace gomoku::application {

GameService::GameService(std::unique_ptr<ISearchEngine> searchEngine)
    : board_(std::make_unique<Board>())
    , searchEngine_(std::move(searchEngine))
{
}

GameService::~GameService() = default;

void GameService::startNewGame(const RuleSet& rules)
{
    rules_ = rules;
    board_->reset();
    moveHistory_.clear();
}

void GameService::reset()
{
    board_->reset();
    moveHistory_.clear();
}

GameStatus GameService::getGameStatus() const
{
    return board_->status();
}

Player GameService::getCurrentPlayer() const
{
    return board_->toPlay();
}

PlayResult GameService::makeMove(const Position& pos)
{
    Move move { pos, getCurrentPlayer() };
    return makeMove(move);
}

PlayResult GameService::makeMove(const Move& move)
{
    // Validate the move first
    std::string reason;
    if (!isMoveLegal(move, &reason)) {
        PlayErrorCode code = PlayErrorCode::RuleViolation;
        if (reason == "Invalid position")
            code = PlayErrorCode::InvalidPosition;
        else if (reason == "Not this player's turn")
            code = PlayErrorCode::NotPlayersTurn;
        else if (reason == "Position already occupied")
            code = PlayErrorCode::Occupied;
        else if (reason == "Game already finished")
            code = PlayErrorCode::GameFinished;
        // Reste: RuleViolation (double-three, must-break...) déjà mappé
        return PlayResult::fail(code, reason);
    }

    // Attempt to play the move
    auto result = board_->tryPlay(move, rules_);
    if (result.success) {
        moveHistory_.push_back(move);
    }

    return result;
}

bool GameService::canUndo() const
{
    return !moveHistory_.empty();
}

bool GameService::undo()
{
    if (!canUndo()) {
        return false;
    }

    bool success = board_->undo();
    if (success && !moveHistory_.empty()) {
        moveHistory_.pop_back();
    }
    return success;
}

bool GameService::canRedo() const
{
    return board_->canRedo();
}

bool GameService::redo()
{
    if (!canRedo()) {
        return false;
    }

    bool success = board_->redo(rules_);
    if (success) {
        auto m = board_->lastMove();
        if (m) {
            moveHistory_.push_back(*m);
        }
    }
    return success;
}

std::vector<uint8_t> GameService::saveGame() const
{
    return board_->save();
}

bool GameService::loadGame(const std::vector<uint8_t>& data)
{
    // Load board state
    if (!board_->load(data, rules_)) {
        return false;
    }

    // Reconstruct moveHistory_ from board's history
    // Board::load replays moves, so board's history is correct.
    // We need to sync GameService's moveHistory_ (which is just vector<Move>)
    moveHistory_.clear();
    // Board doesn't expose full history directly as vector<Move>, but we can get it via lastMoves
    // or we can iterate.
    // Actually, Board::moveHistory is private.
    // But we can use Board::lastMoves(N) where N is moveCount.
    int count = board_->moveCount();
    if (count > 0) {
        auto moves = board_->lastMoves(count);
        // lastMoves returns most recent first. We want chronological order.
        moveHistory_.reserve(count);
        for (auto it = moves.rbegin(); it != moves.rend(); ++it) {
            moveHistory_.push_back(*it);
        }
    }

    return true;
}

const IBoardView& GameService::getBoard() const
{
    return *board_;
}

std::vector<Move> GameService::getLegalMoves() const
{
    return board_->legalMoves(getCurrentPlayer(), rules_);
}

bool GameService::isMoveLegal(const Move& move, std::string* reason) const
{
    return validateMove(move, reason);
}

CaptureCount GameService::getCaptureCount() const
{
    return board_->capturedPairs();
}

std::optional<Move> GameService::getAIMove(int timeMs, SearchStats* outStats)
{
    if (!searchEngine_) {
        if (outStats)
            outStats->clear();
        return std::nullopt;
    }

    SearchStats stats;
    auto move = searchEngine_->suggestMove(*board_, rules_, timeMs, &stats);

    if (outStats)
        *outStats = stats;

    return move;
}

void GameService::setSearchEngine(std::unique_ptr<ISearchEngine> engine)
{
    searchEngine_ = std::move(engine);
}

bool GameService::validateMove(const Move& move, std::string* reason) const
{
    auto base = moveValidator_.validate(*board_, rules_, move);
    if (!base.ok) {
        if (reason)
            *reason = base.reason;
        return false;
    }

    auto self = const_cast<GameService*>(this);
    PlayResult pr;
    bool ok = self->board_->speculativeTry(move, rules_, &pr);
    if (!ok) {
        if (reason)
            *reason = pr.error;
        return false;
    }
    return true;
}

} // namespace gomoku::application
