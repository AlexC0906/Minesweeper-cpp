#include "game.hpp"
#include <iostream>
#include <string>

int main() {
    // Prompt user to select difficulty
    std::cout << "Select difficulty:\n"
                 "1. Easy \n"
                 "2. Medium \n"
                 "3. Hard \n"
                 "Enter choice [1-3]: ";
    int choice = 0;
    std::cin >> choice;
    unsigned int rows, cols;
    const float cellSize = 64.f;
    std::string bestTimeFile;
    switch (choice) {
        case 2:
            rows = 15; cols = 15;
            bestTimeFile = "best_time_medium.txt";
            break;
        case 3:
            rows = 19; cols = 19;
            bestTimeFile = "best_time_hard.txt";
            break;
        case 1:
        default:
            rows = 10; cols = 10;
            bestTimeFile = "best_time_easy.txt";
            break;
    }
    Game game(rows, cols, cellSize, bestTimeFile);
    game.run();

    return 0;
}