#pragma once

#include "gomoku/core/Types.hpp"
#include <array>
#include <cstdint>

namespace gomoku::rays {

struct CapRay {
    uint16_t fwd[3] {};
    uint16_t bwd[3] {};
};

constexpr uint16_t encode(int x, int y)
{
    return (unsigned)x < BOARD_SIZE && (unsigned)y < BOARD_SIZE
        ? static_cast<uint16_t>(y * BOARD_SIZE + x)
        : static_cast<uint16_t>(0xFFFF);
}

constexpr std::array<std::array<CapRay, BOARD_SIZE * BOARD_SIZE>, 4> makeCapRays()
{
    std::array<std::array<CapRay, BOARD_SIZE * BOARD_SIZE>, 4> rays {};
    constexpr int DX[4] = { 1, 0, 1, 1 };
    constexpr int DY[4] = { 0, 1, 1, -1 };
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            const int i = y * BOARD_SIZE + x;
            for (int d = 0; d < 4; ++d) {
                for (int k = 1; k <= 3; ++k) {
                    rays[d][i].fwd[k - 1] = encode(x + k * DX[d], y + k * DY[d]);
                    rays[d][i].bwd[k - 1] = encode(x - k * DX[d], y - k * DY[d]);
                }
            }
        }
    }
    return rays;
}

// Inline variable (since C++17) to provide a single definition across TUs
inline constexpr auto capRaysByDir = makeCapRays();

} // namespace gomoku::rays
