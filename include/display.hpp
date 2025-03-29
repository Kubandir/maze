#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <ncurses.h>
#include "mazeGenerator.hpp"

class Display {
private:
    int termsize[2];
    int offsetX;
    int offsetY;
    bool needsRedraw;
    bool resizeNeeded;
    std::chrono::steady_clock::time_point lastResizeTime;
    std::unordered_map<std::string, int> colorMap;

    void setupColors();
    int getVisualWidth(const std::string& str);
    std::string colorTags(const std::string& str);

public:
    Display();
    ~Display();

    void setup();
    void updateTermsize();
    void checkResize();
    void centerPrint(const std::string& str, bool vertical, int y);
    void drawMaze(const MazeGenerator& maze);
    void drawUI();
    void validPositionHint(const MazeGenerator& maze);
    void redraw(const MazeGenerator& maze);

    int getOffsetX() const;
    int getOffsetY() const;
    void setOffsetX(int x);
    void setOffsetY(int y);
    void setNeedsRedraw(bool value);
    bool getNeedsRedraw() const;
    void setResizeNeeded(bool value);
    bool getResizeNeeded() const;
    int* getTermsize();
};

#endif
