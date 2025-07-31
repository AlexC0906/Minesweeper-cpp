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
    void placeMines(unsigned int safeRow, unsigned int safeCol); //Place mines on first click, excluding the first clicked cell
    void calculateAdjacents();
    void revealCell(unsigned int row, unsigned int col);
    void revealNeighbors(unsigned int row, unsigned int col);

    sf::RenderWindow window;
    sf::Font font;
    sf::Clock timer;  
    std::vector<std::vector<Cell>> grid;
    unsigned int rows;
    unsigned int cols;
    float cellSize;
    bool gameOverFlag;
    bool gameWonFlag;
    unsigned int totalMines;  
    unsigned int flagsUsed;    
    bool firstClick;          
    unsigned int savedTime;   
};