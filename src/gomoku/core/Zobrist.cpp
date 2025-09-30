#include "gomoku/core/Zobrist.hpp"
#include <array>
#include <random>

namespace gomoku::zobrist {

namespace {
    std::array<uint64_t, 2 * BOARD_SIZE * BOARD_SIZE> Z_PCS {};
    uint64_t Z_SIDE = 0;

    inline int flat(int x, int y) { return y * BOARD_SIZE + x; }

    struct ZInit {
        ZInit()
        {
            std::mt19937_64 rng(0x9E3779B97F4A7C15ULL); // fixed seed for reproducibility
            for (auto& v : Z_PCS)
                v = rng();
            Z_SIDE = rng();
        }
    } ZINIT;
}

uint64_t piece(Cell c, uint8_t x, uint8_t y) noexcept
{
    if (c == Cell::Black)
        return Z_PCS[0 * BOARD_SIZE * BOARD_SIZE + flat(x, y)];
    if (c == Cell::White)
        return Z_PCS[1 * BOARD_SIZE * BOARD_SIZE + flat(x, y)];
    return 0ull;
}

uint64_t side() noexcept { return Z_SIDE; }

} // namespace gomoku::zobrist
