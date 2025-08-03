#include "game.hpp"
#include <iostream>
#include <string>

int main() {
    // Start game in Easy mode; use in-game button to change difficulty
    const unsigned int rows = 10;
    const unsigned int cols = 10;
    const float cellSize = 64.f;
    const std::string bestTimeFile = "best_time_easy.txt";
    Game game(rows, cols, cellSize, bestTimeFile);
    game.run();
    return 0;
}