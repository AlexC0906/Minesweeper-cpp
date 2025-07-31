#include "cell.hpp"

Cell::Cell(float x, float y, float size)
    : sf::RectangleShape(sf::Vector2f(size, size)), mine(false), adjacentMines(0), state(CellState::Hidden) {
    setPosition(x, y);
    setFillColor(sf::Color::Green);
    setOutlineThickness(1.f);
    setOutlineColor(sf::Color::Black);
}

void Cell::reveal() {
    if (state == CellState::Hidden)
        state = CellState::Revealed;
}

void Cell::toggleFlag() {
    if (state == CellState::Hidden)
        state = CellState::Flagged;
    else if (state == CellState::Flagged)
        state = CellState::Hidden;
}

bool Cell::isMine() const {
    return mine;
}

void Cell::setMine(bool m) {
    mine = m;
}

int Cell::getAdjacentMines() const {
    return adjacentMines;
}

void Cell::setAdjacentMines(int count) {
    adjacentMines = count;
}

CellState Cell::getState() const {
    return state;
}