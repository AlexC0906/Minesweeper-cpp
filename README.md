# 💣 Minesweeper – Remake in C++ with SFML
A complete remake of the classic Minesweeper game, developed in C++ using the SFML 2.5.1 library.  
Includes 3 difficulty levels, sound effects, a timer, automatic reveal on numbered tiles, and saving the best time for each difficulty.

---

## 🔧 Features
🎨 Graphical interface (no terminal)  
📏 **3 difficulty levels**:  
 • Easy – 10×10 grid  
 • Medium – 15×15 grid  
 • Hard – 19×19 grid  
🏴 Right-click to place/remove flags  
🖱 Left-click to reveal cells  
⏱ Timer for each game + best time saved to file  
🔊 Sound effects for placing/removing flags  
🎵 Victory music on win  
💀 *Game Over* screen showing all mines + **Try Again** button  
🎉 Victory screen with best time + **Play Again** button  
⚡ Automatic reveal of surrounding cells when clicking on a number with the correct number of adjacent flags  

---

## 🎮 Controls
| Action | Input |
|--------|-------|
| Reveal cell | Left mouse click |
| Place/Remove flag | Right mouse click |
| Automatic reveal around number | Left click on a revealed number when the correct number of flags is placed |
| Retry after win/lose | Button click |

---

## ▶️ How to run the project 

1. Make sure you have **SFML 2.5.1** installed.
2. Compile with:
   ```bash
   g++ -std=c++17 src/*.cpp -o main.exe -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system
3. Run with:
   ```bash
   .\main.exe 
