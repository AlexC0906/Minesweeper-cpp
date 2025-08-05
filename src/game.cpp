#include "game.hpp"
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>


Game::Game(unsigned int rows, unsigned int cols, float cellSize, const std::string& bestTimeFile)
    : rows(rows)
    , cols(cols)
    , cellSize(cellSize)
    , gameOverFlag(false)
    , gameWonFlag(false)
    , firstClick(true)
    , savedTime(0)
    , bestTime(0)
    , fadeStarted(false)
    , fadeDuration(2.f)
    , bestTimeFile(bestTimeFile)
    , newRecord(false)
    , selectingDifficulty(false)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    // Initialize mine/flag counters
    totalMines = (rows * cols) / 6;
    flagsUsed = 0;
    // Load font for drawing numbers
    if (!font.loadFromFile("ARIAL.TTF")) {
        std::cerr << "Failed to load font ARIAL.TTF" << std::endl;
    }
    // Load flag texture for flagged cells
    if (!flagTexture.loadFromFile("red_flag.png")) {
        std::cerr << "Failed to load red_flag.png" << std::endl;
    }
    // Load clock texture for timer icon
    if (!clockTexture.loadFromFile("ceas_minesweeper.png")) {
        std::cerr << "Failed to load ceas_minesweeper.png" << std::endl;
    }
    // Load mine texture for revealed mines
    if (!mineTexture.loadFromFile("mine_minesweeper.png")) {
        std::cerr << "Failed to load mine_minesweeper.png" << std::endl;
    }
    // Load audio for flag actions
    if (!nudgeBuffer.loadFromFile("Nudge_Sound_Effect.wav")) {
        std::cerr << "Failed to load Nudge_Sound_Effect.wav" << std::endl;
    }
    nudgeSound.setBuffer(nudgeBuffer);
    if (!popBuffer.loadFromFile("Pop.wav")) {
        std::cerr << "Failed to load Pop.wav" << std::endl;
    }
    popSound.setBuffer(popBuffer);
    // Load victory music
    if (!victoryMusic.openFromFile("Victory_music.wav")) {
        std::cerr << "Failed to load Victory_music.wav" << std::endl;
    }
    victoryMusic.setLoop(true);
    // create window after loading assets to prevent initial blank
    window.create(sf::VideoMode(cols * cellSize, rows * cellSize + static_cast<int>(cellSize)), "Minesweeper");
    initGrid();  // set up grid; delay mine placement until first click
    loadBestTime(); // load record best time from file: bestTimeFile
}

void Game::run() {
    // initial render to set up UI element bounds
    render();
    while (window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
            continue;
        }
        // difficulty menu click handling
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f click(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
            if (selectingDifficulty) {
                if (easyRect.contains(click)) { applyDifficulty(1); continue; }
                if (mediumRect.contains(click)) { applyDifficulty(2); continue; }
                if (hardRect.contains(click)) { applyDifficulty(3); continue; }
                // clicked outside options: cancel menu
                selectingDifficulty = false;
                continue;
            } else if (diffBounds.contains(click)) {
                // open difficulty menu
                selectingDifficulty = true;
                continue;
            }
        }
        // Handle retry click when game lost and after fade completion
        // Handle play/try again click after fade (for win or loss)
        if (event.type == sf::Event::MouseButtonPressed && gameOverFlag && fadeStarted 
            && fadeClock.getElapsedTime().asSeconds() >= fadeDuration 
            && event.mouseButton.button == sf::Mouse::Left) {
            auto pos = sf::Mouse::getPosition(window);
            if (retryBounds.contains(static_cast<float>(pos.x), static_cast<float>(pos.y))) {
                reset();
                continue;
            }
        }
        // Handle mouse input for game actions
        if (event.type == sf::Event::MouseButtonPressed && !gameOverFlag) {
            auto mousePos = sf::Mouse::getPosition(window);
            int mx = mousePos.x;
            int my = mousePos.y;
            
            if (mx >= 0 && mx < static_cast<int>(cols * cellSize) && my >= static_cast<int>(cellSize)) {
                unsigned int colIdx = mx / static_cast<int>(cellSize);
                unsigned int rowIdx = (my - static_cast<int>(cellSize)) / static_cast<int>(cellSize);
                if (rowIdx < rows && colIdx < cols) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        if (firstClick) {
                            // First click: place mines and compute adjacents
                            placeMines(rowIdx, colIdx);
                            calculateAdjacents();
                            firstClick = false;
                            // clear any flags placed before game start
                            flagsUsed = 0;
                            for (auto& row : grid) {
                                for (auto& cell : row) {
                                    if (cell.getState() == CellState::Flagged) {
                                        cell.toggleFlag();
                                    }
                                }
                            }
                            revealCell(rowIdx, colIdx);
                        } else {
                            Cell& cell = grid[rowIdx][colIdx];
                            if (cell.getState() == CellState::Revealed && !cell.isMine() && cell.getAdjacentMines() > 0) {
                                // Chord reveal neighbors when flags match
                                int flagCount = 0;
                                for (int di = -1; di <= 1; ++di) {
                                    for (int dj = -1; dj <= 1; ++dj) {
                                        if (di == 0 && dj == 0) continue;
                                        int ni = static_cast<int>(rowIdx) + di;
                                        int nj = static_cast<int>(colIdx) + dj;
                                        if (ni >= 0 && ni < static_cast<int>(rows) && nj >= 0 && nj < static_cast<int>(cols) &&
                                            grid[ni][nj].getState() == CellState::Flagged) {
                                            flagCount++;
                                        }
                                    }
                                }
                                if (flagCount == static_cast<int>(cell.getAdjacentMines())) {
                                    for (int di = -1; di <= 1; ++di) {
                                        for (int dj = -1; dj <= 1; ++dj) {
                                            if (di == 0 && dj == 0) continue;
                                            int ni = static_cast<int>(rowIdx) + di;
                                            int nj = static_cast<int>(colIdx) + dj;
                                            if (ni >= 0 && ni < static_cast<int>(rows) && nj >= 0 && nj < static_cast<int>(cols) &&
                                                grid[ni][nj].getState() == CellState::Hidden) {
                                                revealCell(static_cast<unsigned int>(ni), static_cast<unsigned int>(nj));
                                            }
                                        }
                                    }
                                }
                            } else {
                                revealCell(rowIdx, colIdx);
                            }
                        }
                    } else if (event.mouseButton.button == sf::Mouse::Right) {
                        Cell& cell = grid[rowIdx][colIdx];
                        if (cell.getState() == CellState::Hidden) {
                            cell.toggleFlag();
                            flagsUsed++;
                            nudgeSound.play();
                        } else if (cell.getState() == CellState::Flagged) {
                            cell.toggleFlag();
                            flagsUsed--;
                            popSound.play();
                        }
                    }
                }
            }
        } else if (event.type == sf::Event::MouseButtonPressed) {
            if (gameOverFlag && !gameWonFlag && event.mouseButton.button == sf::Mouse::Left) {
                auto mousePos = sf::Mouse::getPosition(window);
                if (retryBounds.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                    reset();
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
        // stop timer
        savedTime = static_cast<unsigned int>(timer.getElapsedTime().asSeconds());
        // check for new best record
        bool isNew = (bestTime == 0 || savedTime < bestTime);
        newRecord = isNew;
        if (isNew) {
            bestTime = savedTime;
            saveBestTime();
        }
        std::cout << "You win!" << std::endl;
        // Reveal all mines to show win state
        for (auto& row : grid)
            for (auto& cell : row)
                if (cell.isMine())
                    cell.reveal();
        // start fade animation and play victory music
        if (!fadeStarted) {
            fadeStarted = true;
            fadeClock.restart();
            victoryMusic.play();
        }
    }
}

void Game::render() {
    window.clear();
   
    sf::RectangleShape uiBar(sf::Vector2f(static_cast<float>(window.getSize().x), cellSize));
    uiBar.setFillColor(sf::Color(94, 142, 60)); 
    uiBar.setPosition(0.f, 0.f);
    window.draw(uiBar);

    {
        unsigned int remaining = (totalMines > flagsUsed ? totalMines - flagsUsed : 0);
        // flag icon
        sf::Sprite flagSprite(flagTexture);
        float charSize = cellSize * 0.5f;
        float iconH = charSize * 1.5f;
        auto ts = flagTexture.getSize();
        float scale = iconH / ts.y;
        flagSprite.setScale(scale, scale);
        float iconW = ts.x * scale;
        float uiX = 5.f;
        float y = 5.f;
        float verticalOffset = 6.f;
        float cy = y - (iconH - charSize) / 2.f + verticalOffset;
        flagSprite.setPosition(uiX, cy);
        window.draw(flagSprite);
        sf::Text countText;
        countText.setFont(font);
        countText.setString(std::to_string(remaining));
        countText.setCharacterSize(static_cast<unsigned int>(charSize));
        countText.setFillColor(sf::Color::White);
        sf::FloatRect bt = countText.getLocalBounds();
        float tx = uiX + iconW + 4.f;
        float ty = y + (charSize - bt.height) / 2.f - bt.top + verticalOffset;
        countText.setPosition(tx, ty);
        window.draw(countText);
    }
    
    {
        //get elapsed seconds and cap at 999 and freeze on game over
        unsigned int secs;
        if (gameOverFlag) {
            secs = savedTime;
        } else {
            secs = static_cast<unsigned int>(timer.getElapsedTime().asSeconds());
        }
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
        // draw clock icon left of timer
        {
            sf::Sprite clockSprite(clockTexture);
            auto ts = clockTexture.getSize();
            // set icon height relative to text and preserve aspect ratio
            float iconH = timerText.getCharacterSize() * 1.5f;
            float scale = iconH / ts.y;
            clockSprite.setScale(scale, scale);
            float iconW = ts.x * scale;
           
            float padding = 1.f; 
            float cx = x - padding - iconW;
            
            float iconY = iconH;
            float cy = y - (iconY - timerText.getCharacterSize()) / 2.f;
            
            float verticalOffset = 6.f; 
            cy += verticalOffset;
            clockSprite.setPosition(cx, cy);
            window.draw(clockSprite);
        }
        timerText.setPosition(x, y);
        window.draw(timerText);
    }
    
    {
        float textSize = cellSize * 0.5f;
        float pad = 8.f;
        
        sf::Text diffText("Mode", font, static_cast<unsigned int>(textSize));
        diffText.setFillColor(sf::Color::White);
        sf::FloatRect tb = diffText.getLocalBounds();
        float width = tb.width + pad * 2.f;
        float height = tb.height + pad * 2.f;
       
        float btnX = (window.getSize().x - width) / 2.f;
        float btnY = (cellSize - height) / 2.f;
        
        //mode button
        sf::Color btnColor = sf::Color::Black;
        float r = pad; // corner radius
        // horizontal center rectangle
        sf::RectangleShape rectH(sf::Vector2f(width - 2 * r, height));
        rectH.setFillColor(btnColor);
        rectH.setPosition(btnX + r, btnY);
        window.draw(rectH);
        // vertical center rectangle
        sf::RectangleShape rectV(sf::Vector2f(width, height - 2 * r));
        rectV.setFillColor(btnColor);
        rectV.setPosition(btnX, btnY + r);
        window.draw(rectV);
        // corner circles
        const int cornerPoints = 20;
        sf::CircleShape corner(r, cornerPoints);
        corner.setFillColor(btnColor);
        // top-left
        corner.setPosition(btnX, btnY);
        window.draw(corner);
        // top-right
        corner.setPosition(btnX + width - 2 * r, btnY);
        window.draw(corner);
        // bottom-left
        corner.setPosition(btnX, btnY + height - 2 * r);
        window.draw(corner);
        // bottom-right
        corner.setPosition(btnX + width - 2 * r, btnY + height - 2 * r);
        window.draw(corner);
       
        float textX = btnX + (width - tb.width) / 2.f - tb.left;
        float textY = btnY + (height - tb.height) / 2.f - tb.top;
        diffText.setPosition(textX, textY);
        window.draw(diffText);
        
        diffBounds = sf::FloatRect(btnX, 0.f, width, cellSize);
    }
    for (unsigned int i = 0; i < rows; ++i) {
        for (unsigned int j = 0; j < cols; ++j) {
            Cell& cell = grid[i][j];
            // Set color based on state
            switch (cell.getState()) {
            case CellState::Hidden:
                // alternating hidden cell colors for checker pattern
                if ((i + j) % 2 == 0)
                    cell.setFillColor(sf::Color(170, 215, 81)); // lighter green shade
                else
                    cell.setFillColor(sf::Color(162, 209, 73)); // slightly darker light green shade
                break;
            case CellState::Flagged:
                // keep hidden background for flagged
                if ((i + j) % 2 == 0)
                    cell.setFillColor(sf::Color(170, 215, 81));
                else
                    cell.setFillColor(sf::Color(162, 209, 73));
                break;
            case CellState::Revealed:
                if (cell.isMine())
                    cell.setFillColor(sf::Color::Red);
                else
                    cell.setFillColor(sf::Color(200, 200, 200));
                break;
            }
            window.draw(cell);
            // draw flag icon for flagged cells
            if (cell.getState() == CellState::Flagged) {
                sf::Sprite flagSprite(flagTexture);
                // scale sprite to cell size
                auto ts = flagTexture.getSize();
                flagSprite.setScale(cellSize / ts.x, cellSize / ts.y);
                flagSprite.setPosition(cell.getPosition());
                window.draw(flagSprite);
            }
            // Draw adjacent mine count for revealed non-mine cells
            if (cell.getState() == CellState::Revealed && !cell.isMine() && cell.getAdjacentMines() > 0) {
                sf::Text text;
                text.setFont(font);
                text.setString(std::to_string(cell.getAdjacentMines()));
                text.setCharacterSize(static_cast<unsigned int>(cellSize * 0.5f));
                //set color based on number of adjacent mines
                switch (cell.getAdjacentMines()) {
                case 1: text.setFillColor(sf::Color::Blue); break;
                case 2: text.setFillColor(sf::Color(0, 128, 0)); break; 
                case 3: text.setFillColor(sf::Color(180, 0, 0)); break; 
                case 4: text.setFillColor(sf::Color(128, 0, 128)); break;        
                case 5: text.setFillColor(sf::Color(255, 105, 180)); break;        
                case 6: text.setFillColor(sf::Color(0, 255, 255)); break;          
                case 7: text.setFillColor(sf::Color::Black); break;
                case 8: text.setFillColor(sf::Color(128, 128, 128)); break;  
                default: text.setFillColor(sf::Color::White); break;
                }
                // Center text in cell
                sf::FloatRect bounds = text.getLocalBounds();
                float x = cell.getPosition().x + (cellSize - bounds.width) / 2.f;
                float y = cell.getPosition().y + (cellSize - bounds.height) / 2.f - bounds.top;
                text.setPosition(x, y);
                window.draw(text);
            }
            // draw mine icon for revealed mines
            if (cell.getState() == CellState::Revealed && cell.isMine()) {
                sf::Sprite mineSprite(mineTexture);
                auto mts = mineTexture.getSize();
                mineSprite.setScale(cellSize / mts.x, cellSize / mts.y);
                mineSprite.setPosition(cell.getPosition());
                window.draw(mineSprite);
            }
        }
    }
    // apply fade overlay if win fade started
    if (fadeStarted) {
        float elapsed = fadeClock.getElapsedTime().asSeconds();
        float t = elapsed / fadeDuration;
        if (t > 1.f) t = 1.f;
        sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)));
        overlay.setPosition(0.f, 0.f);
        overlay.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(150 * t)));
        window.draw(overlay);
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
        // display best time when won
        if (gameWonFlag) {
            sf::Text bestText;
            bestText.setFont(font);
            unsigned int btime = bestTime;
            std::string bestStr = std::to_string(btime);
            while (bestStr.length() < 3) bestStr = "0" + bestStr;
            // display "New best!" when record beaten, else show best time
            if (newRecord) {
                bestText.setString(std::string("New best: ") + bestStr);
            } else {
                bestText.setString(std::string("Best: ") + bestStr);
            }
            bestText.setCharacterSize(static_cast<unsigned int>(cellSize * 0.5f));
            bestText.setFillColor(sf::Color::White);
            sf::FloatRect bb = bestText.getLocalBounds();
            float bx = (window.getSize().x - bb.width) / 2.f - bb.left;
            float by = msg.getPosition().y + msg.getCharacterSize() + 5.f;
            bestText.setPosition(bx, by);
            window.draw(bestText);
        }
        // draw button after game over (Play Again on win, Try Again on loss)
        if (fadeStarted && fadeClock.getElapsedTime().asSeconds() >= fadeDuration) {
            // button label and size
            std::string label = gameWonFlag ? "Play Again" : "Try Again";
            sf::Text retryText;
            retryText.setFont(font);
            retryText.setString(label);
            retryText.setCharacterSize(static_cast<unsigned int>(cellSize * 0.5f));
            // compute text bounds
            sf::FloatRect tb = retryText.getLocalBounds();
            // padding and dimensions
            float padX = 16.f;
            float padY = 8.f;
            float w = tb.width + padX * 2.f;
            float h = tb.height + padY * 2.f;
            float r = padY; // corner radius
            // button origin centered horizontally, below message
            float x0 = (window.getSize().x - w) / 2.f;
            float y0;
            if (gameWonFlag) {
                // position below best time text: msg + msg size + small gap + best text size + extra padding
                y0 = msg.getPosition().y + msg.getCharacterSize() + 5.f
                    + static_cast<float>(cellSize * 0.5f) + 20.f;
            } else {
               
                y0 = msg.getPosition().y + msg.getCharacterSize() + 30.f;
            }
            // position text inside button
            retryText.setPosition(x0 + padX - tb.left, y0 + padY - tb.top);
            // center rectangle (horizontal)
            sf::RectangleShape rectH(sf::Vector2f(w - 2*r, h));
            rectH.setFillColor(sf::Color(50, 50, 50, 200));
            rectH.setPosition(x0 + r, y0);
            window.draw(rectH);
            // center rectangle (vertical)
            sf::RectangleShape rectV(sf::Vector2f(w, h - 2*r));
            rectV.setFillColor(sf::Color(50, 50, 50, 200));
            rectV.setPosition(x0, y0 + r);
            window.draw(rectV);
            // corner circles
            sf::CircleShape corner(r, 50);
            corner.setFillColor(sf::Color(50, 50, 50, 200));
            // top-left
            corner.setPosition(x0, y0);
            window.draw(corner);
            // top-right
            corner.setPosition(x0 + w - 2*r, y0);
            window.draw(corner);
            // bottom-left
            corner.setPosition(x0, y0 + h - 2*r);
            window.draw(corner);
            // bottom-right
            corner.setPosition(x0 + w - 2*r, y0 + h - 2*r);
            window.draw(corner);
            // text
            window.draw(retryText);
            // store bounds for click detection
            retryBounds = sf::FloatRect(x0, y0, w, h);
        }
    }
    // Render difficulty selection overlay on top if active
    if (selectingDifficulty) {
        // darken background
        sf::RectangleShape overlay(sf::Vector2f((float)window.getSize().x, (float)window.getSize().y));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);
        // draw menu options
        std::vector<std::string> opts = {"Easy", "Medium", "Hard"};
        float baseY = window.getSize().y * 0.4f;
        for (int i = 0; i < 3; ++i) {
            sf::Text optText(opts[i], font, static_cast<unsigned int>(cellSize * 0.6f));
            optText.setFillColor(sf::Color::White);
            sf::FloatRect lb = optText.getLocalBounds();
            float tx = (window.getSize().x - lb.width) / 2.f - lb.left;
            float ty = baseY + i * (lb.height + 20.f) - lb.top;
            optText.setPosition(tx, ty);
            window.draw(optText);
            // update bounds for click detection
            if (i == 0) easyRect = optText.getGlobalBounds();
            else if (i == 1) mediumRect = optText.getGlobalBounds();
            else hardRect = optText.getGlobalBounds();
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
            // shift all cells down by cellSize to make room for top UI bar
            rowCells.emplace_back(j * cellSize, i * cellSize + cellSize, cellSize);
        }
        grid.push_back(std::move(rowCells));
    }
}

// Load the best time from a file
// Load the best time from the configured file
void Game::loadBestTime() {
    std::ifstream fin(bestTimeFile);
    if (fin) {
        unsigned int t;
        if (fin >> t) bestTime = t;
    }
}

// Save the best time to the configured file
void Game::saveBestTime() {
    std::ofstream fout(bestTimeFile, std::ios::trunc);
    if (fout) fout << bestTime;
}

// Place mines randomly, excluding the first-clicked safe cell and its neighbors
void Game::placeMines(unsigned int safeRow, unsigned int safeCol) {
    unsigned int placed = 0;
    while (placed < totalMines) {
        unsigned int r = std::rand() % rows;
        unsigned int c = std::rand() % cols;
        // skip the safe cell, its neighbors, and already mined cells
        if (grid[r][c].isMine() ||
            (std::abs(static_cast<int>(r) - static_cast<int>(safeRow)) <= 1 &&
             std::abs(static_cast<int>(c) - static_cast<int>(safeCol)) <= 1)) {
            continue;
        }
        grid[r][c].setMine(true);
        placed++;
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
        // stop timer
        savedTime = static_cast<unsigned int>(timer.getElapsedTime().asSeconds());
        // start fade animation on loss
        if (!fadeStarted) {
            fadeStarted = true;
            fadeClock.restart();
        }
        return;
    }
    // Only auto-reveal neighbors if this cell has no adjacent mines
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
// Reset the game state for a new playthrough
void Game::reset() {
    gameOverFlag = false;
    gameWonFlag = false;
    firstClick = true;
    flagsUsed = 0;
    savedTime = 0;
    fadeStarted = false;
    timer.restart();
    // stop victory music if playing
    victoryMusic.stop();
    // reinitialize grid
    initGrid();
}
// apply a new difficulty setting and restart game
void Game::applyDifficulty(int choice) {
    // map difficulty to grid size and file
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
    // recreate window
    window.create(sf::VideoMode(cols * cellSize, rows * cellSize + static_cast<int>(cellSize)), "Minesweeper");
    totalMines = (rows * cols) / 6;
    // reset state
    gameOverFlag = false;
    gameWonFlag = false;
    firstClick = true;
    flagsUsed = 0;
    savedTime = 0;
    bestTime = 0;
    newRecord = false;
    fadeStarted = false;
    selectingDifficulty = false;
    timer.restart();
    fadeClock.restart();
    initGrid();
    loadBestTime();
}