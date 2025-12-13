#include "util/GameSaver.hpp"
#include "util/Logger.hpp"
#include "util/json.hpp"
#include "gomoku/application/SessionController.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace gomoku::util {

    std::string GameSaver::configFilePath()
    {
        const char* home = std::getenv("HOME");
        std::string base = home ? std::string(home) : std::string(".");
        fs::path dir = fs::path(base) / ".config" / "gomoku";
        fs::create_directories(dir);
        return (dir / "save.json").string();
    }

    bool GameSaver::saveIsExist()
    {
        std::ifstream in(configFilePath());
        if (!in.is_open())
            return false;
        return true;
    }

    // bool GameSaver::load(saveData& outSave)
    // {

    // }

    // void GameSaver::save(const saveData& save, GameSnapshot snapshot)
    // {

    // }

} // namespace gomoku::util


