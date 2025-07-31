#include "game.hpp"

int main() {
    const unsigned int rows = 10;
    const unsigned int cols = 10;
    const float cellSize = 64.f;  
    Game game(rows, cols, cellSize);
    game.run();

    return 0;
}