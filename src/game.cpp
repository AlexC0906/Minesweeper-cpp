#include "game.hpp"
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>

Game::Game(unsigned int rows, unsigned int cols, float cellSize)
    : window(sf::VideoMode(cols * cellSize, rows * cellSize), "Minesweeper"),
      grid(), rows(rows), cols(cols), cellSize(cellSize), gameOverFlag(false) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
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
            unsigned int colIdx = mousePos.x / static_cast<int>(cellSize);
            unsigned int rowIdx = mousePos.y / static_cast<int>(cellSize);
            if (rowIdx < rows && colIdx < cols) {
                if (event.mouseButton.button == sf::Mouse::Left)
                    revealCell(rowIdx, colIdx);
                else if (event.mouseButton.button == sf::Mouse::Right)
                    grid[rowIdx][colIdx].toggleFlag();
            }
        }
    }
}

void Game::update() {
    // ... update game logic ...
}

void Game::render() {
    window.clear();
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
    window.display();
}

void Game::initGrid() {
    grid.clear();
    grid.reserve(rows);
    for (unsigned int i = 0; i < rows; ++i) {
        std::vector<Cell> rowCells;
        rowCells.reserve(cols);
        for (unsigned int j = 0; j < cols; ++j) {
            rowCells.emplace_back(j * cellSize, i * cellSize, cellSize);
        }
        grid.push_back(std::move(rowCells));
    }
}

void Game::placeMines() {
    // Place a number of mines randomly (approx 1/6 of cells)
    unsigned int totalMines = (rows * cols) / 6;
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