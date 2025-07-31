#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include "cell.hpp"

class Game {
public:
    Game(unsigned int rows, unsigned int cols, float cellSize);
    void run();

private:
    void processEvents();
    void update();
    void render();
    void initGrid();
    void placeMines();
    void calculateAdjacents();
    void revealCell(unsigned int row, unsigned int col);
    void revealNeighbors(unsigned int row, unsigned int col);

    sf::RenderWindow window;
    sf::Font font;
    std::vector<std::vector<Cell>> grid;
    unsigned int rows;
    unsigned int cols;
    float cellSize;
    bool gameOverFlag;
    bool gameWonFlag;
    unsigned int totalMines;  
    unsigned int flagsUsed;    
};