#include "gomoku/core/Zobrist.hpp"
#include <array>
#include <random>

namespace gomoku::zobrist {

namespace {
    constexpr std::size_t S = static_cast<std::size_t>(BOARD_SIZE) * BOARD_SIZE;
    std::array<uint64_t, 2 * S> Z_PCS {};
    uint64_t Z_SIDE = 0;

    void do_seed(uint64_t seed) noexcept
    {
        std::mt19937_64 rng(seed); // reproductible
        for (auto& v : Z_PCS)
            v = rng();
        Z_SIDE = rng();
    }

    struct ZInit {
        ZInit() { do_seed(0x9E3779B97F4A7C15ULL); }
    } ZINIT;
}

uint64_t piece(Cell c, FlatIdx idx) noexcept
{
    // Empty -> 0
    if (c == Cell::Empty)
        return 0ull;
    const std::size_t color = (c == Cell::White) ? 1 : 0;
    return Z_PCS[color * S + idx];
}

uint64_t side() noexcept { return Z_SIDE; }

void reseed(uint64_t seed) noexcept { do_seed(seed); }

} // namespace gomoku::zobrist
