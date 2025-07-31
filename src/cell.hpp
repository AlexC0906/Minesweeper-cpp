#pragma once

#include <SFML/Graphics.hpp>

enum class CellState { Hidden, Revealed, Flagged };

class Cell : public sf::RectangleShape {
public:
    Cell(float x, float y, float size);
    void reveal();
    void toggleFlag();
    bool isMine() const;
    void setMine(bool mine);
    int getAdjacentMines() const;
    void setAdjacentMines(int count);
    CellState getState() const;
private:
    bool mine;
    int adjacentMines;
    CellState state;
};