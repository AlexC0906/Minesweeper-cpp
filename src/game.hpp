#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "cell.hpp"

class Game {
public:
    Game(unsigned int rows, unsigned int cols, float cellSize, const std::string& bestTimeFile);
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
    sf::Texture flagTexture;  // texture for flag icon
    sf::Texture clockTexture; // texture for clock icon
    sf::Texture mineTexture;  // texture for revealed mine icon
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
    unsigned int bestTime;
    std::string bestTimeFile; 
    bool newRecord;          
    sf::Clock fadeClock;
    bool fadeStarted;
    float fadeDuration;
    void loadBestTime();
    void saveBestTime();
    void reset();
    sf::FloatRect retryBounds; 
    // difficulty selection UI
    bool selectingDifficulty;
    sf::FloatRect diffBounds;       // bounds of difficulty button
    sf::FloatRect easyRect, mediumRect, hardRect; // menu option bounds
    void applyDifficulty(int choice);
};