#pragma once
#include "gomoku/application/SessionController.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace gomoku::util {

struct SaveData {
    bool vsAi;
};

class GameSaver {
public:
    static std::string saveFilePath();
    static bool hasSave();

    // Save game state (binary) and metadata
    static void save(const SaveData& data, const GameSnapshot& snapshot);

    // Load game state (binary) and metadata
    // Returns true if successful
    static bool load(SaveData& outData, std::vector<uint8_t>& outBoardData);
};

} // namespace gomoku::util
