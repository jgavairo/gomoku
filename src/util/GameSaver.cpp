#include "util/GameSaver.hpp"
#include "gomoku/application/SessionController.hpp"
#include "util/Logger.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace gomoku::util {

std::string GameSaver::saveFilePath()
{
    const char* home = std::getenv("HOME");
    std::string base = home ? std::string(home) : std::string(".");
    fs::path dir = fs::path(base) / ".config" / "gomoku";
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }
    return (dir / "save.dat").string();
}

bool GameSaver::hasSave()
{
    return fs::exists(saveFilePath());
}

void GameSaver::save(const SaveData& data, const GameSnapshot& snapshot)
{
    std::vector<uint8_t> buffer;

    // 1. Metadata
    // [1 byte] vsAi (0 or 1)
    buffer.push_back(data.vsAi ? 1 : 0);

    // 2. Board Data
    auto pushInt = [&](uint32_t val) {
        buffer.push_back(static_cast<uint8_t>((val >> 0) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    };
    auto pushMove = [&](const gomoku::Move& m) {
        buffer.push_back(m.pos.x);
        buffer.push_back(m.pos.y);
        buffer.push_back(static_cast<uint8_t>(m.by));
    };

    // Move History
    pushInt(static_cast<uint32_t>(snapshot.moveHistory.size()));
    for (const auto& m : snapshot.moveHistory) {
        pushMove(m);
    }

    // Redo History
    pushInt(static_cast<uint32_t>(snapshot.redoHistory.size()));
    for (const auto& m : snapshot.redoHistory) {
        pushMove(m);
    }

    std::ofstream file(saveFilePath(), std::ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        std::cout << "[GameSaver] Game saved to " << saveFilePath() << " (" << buffer.size() << " bytes)" << std::endl;
    } else {
        std::cerr << "[GameSaver] Failed to open save file for writing" << std::endl;
    }
}

bool GameSaver::load(SaveData& outData, std::vector<uint8_t>& outBoardData)
{
    std::ifstream file(saveFilePath(), std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 1) {
        return false; // Too small
    }

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return false;
    }

    // 1. Metadata
    outData.vsAi = (buffer[0] != 0);

    // 2. Board Data
    // Copy the rest of the buffer
    outBoardData.assign(buffer.begin() + 1, buffer.end());

    return true;
}

} // namespace gomoku::util
