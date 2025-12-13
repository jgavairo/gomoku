#pragma once
#include <string>
#include "gomoku/application/SessionController.hpp"

namespace gomoku::util {

struct saveData {
    
};

class GameSaver {
public:
    static std::string configFilePath();
    static bool saveIsExist();
    static GameSnapshot load(saveData& outSave);
    static void save(const saveData& save, GameSnapshot snapshot);
};

} // namespace gomoku::util


