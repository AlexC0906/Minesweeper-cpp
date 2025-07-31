#include "game.hpp"
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>

Game::Game(unsigned int rows, unsigned int cols, float cellSize)
    : window(sf::VideoMode(cols * cellSize, rows * cellSize + static_cast<int>(cellSize)), "Minesweeper"),
      grid(), rows(rows), cols(cols), cellSize(cellSize), gameOverFlag(false), gameWonFlag(false) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    // Initialize mine/flag counters
    totalMines = (rows * cols) / 6;
    flagsUsed = 0;
    // Load font for drawing numbers
    if (!font.loadFromFile("ARIAL.TTF")) {
        std::cerr << "Failed to load font ARIAL.TTF" << std::endl;
    }
    initGrid();
    placeMines();
    calculateAdjacents();
}

void Game::run() {
    while (window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            window.close();
        // Handle mouse input
        if (event.type == sf::Event::MouseButtonPressed && !gameOverFlag) {
            auto mousePos = sf::Mouse::getPosition(window);
            int mx = mousePos.x;
            int my = mousePos.y;
            
            if (mx >= 0 && mx < static_cast<int>(cols * cellSize) && my >= static_cast<int>(cellSize)) {
                unsigned int colIdx = mx / static_cast<int>(cellSize);
                unsigned int rowIdx = (my - static_cast<int>(cellSize)) / static_cast<int>(cellSize);
                if (rowIdx < rows && colIdx < cols) {
                    if (event.mouseButton.button == sf::Mouse::Left)
                        revealCell(rowIdx, colIdx);
                    else if (event.mouseButton.button == sf::Mouse::Right) {
                        Cell &cell = grid[rowIdx][colIdx];
                        if (cell.getState() == CellState::Hidden) {
                            cell.toggleFlag();
                            flagsUsed++;
                        } else if (cell.getState() == CellState::Flagged) {
                            cell.toggleFlag();
                            flagsUsed--;
                        }
                    }
                }
            }
        }
    }
}

void Game::update() {
    if (gameOverFlag)
        return;
    // Check win condition: if all non-mine cells are revealed
    bool allRevealed = true;
    for (auto& row : grid) {
        for (auto& cell : row) {
            if (!cell.isMine() && cell.getState() != CellState::Revealed) {
                allRevealed = false;
                break;
            }
        }
        if (!allRevealed)
            break;
    }
    if (allRevealed) {
        gameOverFlag = true;
        gameWonFlag = true; // mark win for in-window message
        std::cout << "You win!" << std::endl;
        // Reveal all mines to show win state
        for (auto& row : grid)
            for (auto& cell : row)
                if (cell.isMine())
                    cell.reveal();
    }
}

void Game::render() {
    window.clear();
    // Draw remaining mines counter at top UI area
    {
        sf::Text flagText;
        flagText.setFont(font);
        unsigned int remaining = (totalMines > flagsUsed ? totalMines - flagsUsed : 0);
        flagText.setString("Mines: " + std::to_string(remaining));
        flagText.setCharacterSize(static_cast<unsigned int>(cellSize * 0.5f));
        flagText.setFillColor(sf::Color::White);
        flagText.setPosition(5.f, 5.f);
        window.draw(flagText);
    }
    
    {
        // get elapsed seconds and cap at 999
        unsigned int secs = static_cast<unsigned int>(timer.getElapsedTime().asSeconds());
        if (secs > 999) secs = 999;
        // format as three digits
        std::string timeStr = std::to_string(secs);
        while (timeStr.length() < 3) timeStr = "0" + timeStr;
        sf::Text timerText;
        timerText.setFont(font);
        timerText.setString(timeStr);
        timerText.setCharacterSize(static_cast<unsigned int>(cellSize * 0.5f));
        timerText.setFillColor(sf::Color::White);
        // position at top-right with padding
        sf::FloatRect tb = timerText.getLocalBounds();
        float x = window.getSize().x - tb.width - 5.f - tb.left;
        float y = 5.f;
        timerText.setPosition(x, y);
        window.draw(timerText);
    }
    for (unsigned int i = 0; i < rows; ++i) {
        for (unsigned int j = 0; j < cols; ++j) {
            Cell& cell = grid[i][j];
            // Set color based on state
            switch (cell.getState()) {
            case CellState::Hidden:
                cell.setFillColor(sf::Color(100, 200, 100));
                break;
            case CellState::Flagged:
                cell.setFillColor(sf::Color::Yellow);
                break;
            case CellState::Revealed:
                if (cell.isMine())
                    cell.setFillColor(sf::Color::Red);
                else
                    cell.setFillColor(sf::Color(200, 200, 200));
                break;
            }
            window.draw(cell);
            // Draw adjacent mine count for revealed non-mine cells
            if (cell.getState() == CellState::Revealed && !cell.isMine() && cell.getAdjacentMines() > 0) {
                sf::Text text;
                text.setFont(font);
                text.setString(std::to_string(cell.getAdjacentMines()));
                text.setCharacterSize(static_cast<unsigned int>(cellSize * 0.5f));
                text.setFillColor(sf::Color::Blue);
                // Center text in cell
                sf::FloatRect bounds = text.getLocalBounds();
                float x = cell.getPosition().x + (cellSize - bounds.width) / 2.f;
                float y = cell.getPosition().y + (cellSize - bounds.height) / 2.f - bounds.top;
                text.setPosition(x, y);
                window.draw(text);
            }
        }
    }
    // If game is over, show message
    if (gameOverFlag) {
        sf::Text msg;
        msg.setFont(font);
        msg.setString(gameWonFlag ? "You Win!" : "Game Over");
        msg.setCharacterSize(static_cast<unsigned int>(cellSize));
        msg.setFillColor(sf::Color::White);
        // Center message
        sf::FloatRect bounds = msg.getLocalBounds();
        float x = (window.getSize().x - bounds.width) / 2.f - bounds.left;
        float y = (window.getSize().y - bounds.height) / 2.f - bounds.top;
        msg.setPosition(x, y);
        window.draw(msg);
    }
    window.display();
}

void Game::initGrid() {
    grid.clear();
    grid.reserve(rows);
    for (unsigned int i = 0; i < rows; ++i) {
        std::vector<Cell> rowCells;
        rowCells.reserve(cols);
        for (unsigned int j = 0; j < cols; ++j) {
            // shift all cells down by cellSize to make room for top UI bar
            rowCells.emplace_back(j * cellSize, i * cellSize + cellSize, cellSize);
        }
        grid.push_back(std::move(rowCells));
    }
}

void Game::placeMines() {
    // Place mines randomly based on totalMines
    unsigned int placed = 0;
    while (placed < totalMines) {
        unsigned int r = std::rand() % rows;
        unsigned int c = std::rand() % cols;
        if (!grid[r][c].isMine()) {
            grid[r][c].setMine(true);
            placed++;
        }
    }
}

void Game::calculateAdjacents() {
    for (unsigned int i = 0; i < rows; ++i) {
        for (unsigned int j = 0; j < cols; ++j) {
            if (grid[i][j].isMine()) {
                grid[i][j].setAdjacentMines(-1);
                continue;
            }
            int count = 0;
            for (int di = -1; di <= 1; ++di) {
                for (int dj = -1; dj <= 1; ++dj) {
                    if (di == 0 && dj == 0) continue;
                    int ni = static_cast<int>(i) + di;
                    int nj = static_cast<int>(j) + dj;
                    if (ni >= 0 && ni < static_cast<int>(rows) && nj >= 0 && nj < static_cast<int>(cols)) {
                        if (grid[ni][nj].isMine()) count++;
                    }
                }
            }
            grid[i][j].setAdjacentMines(count);
        }
    }
}

void Game::revealCell(unsigned int row, unsigned int col) {
    Cell& cell = grid[row][col];
    if (cell.getState() != CellState::Hidden) return;
    cell.reveal();
    // If it's a mine, game over
    if (cell.isMine()) {
        gameOverFlag = true;
        gameWonFlag = false;
        // reveal all mines
        for (auto& r : grid)
            for (auto& c : r)
                if (c.isMine()) c.reveal();
        return;
    }
    // If no adjacent mines, reveal neighbors
    if (cell.getAdjacentMines() == 0) {
        revealNeighbors(row, col);
    }
}

void Game::revealNeighbors(unsigned int row, unsigned int col) {
    for (int di = -1; di <= 1; ++di) {
        for (int dj = -1; dj <= 1; ++dj) {
            if (di == 0 && dj == 0) continue;
            int ni = static_cast<int>(row) + di;
            int nj = static_cast<int>(col) + dj;
            if (ni >= 0 && ni < static_cast<int>(rows) && nj >= 0 && nj < static_cast<int>(cols)) {
                if (grid[ni][nj].getState() == CellState::Hidden && !grid[ni][nj].isMine()) {
                    revealCell(ni, nj);
                }
            }
        }
    }
}