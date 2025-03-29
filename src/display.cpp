#include "../include/display.hpp"
#include <cstring>
#include <csignal>
#include <chrono>
#include <locale.h>
#include <wchar.h>
#include <algorithm>

extern void signalHandler(int);
extern void resizeHandler(int);

Display::Display() : offsetX(0), offsetY(0), needsRedraw(true), resizeNeeded(false) {
    termsize[0] = 0;
    termsize[1] = 0;
    lastResizeTime = std::chrono::steady_clock::now();

    colorMap = {
        {"white", 1},
        {"green", 2},
        {"gray", 3},
        {"red", 4},
        {"blue", 5}
    };
}

Display::~Display() {
    endwin();
}

void Display::setupColors() {
    if (has_colors()) {
        start_color();
        if (!can_change_color()) {
            init_pair(1, COLOR_WHITE, COLOR_BLACK); // White text
            init_pair(2, COLOR_GREEN, COLOR_BLACK); // Green text
            init_pair(3, COLOR_WHITE, COLOR_BLACK); // Gray  text
            init_pair(4, COLOR_RED, COLOR_BLACK);   // Red   text
            init_pair(5, COLOR_BLUE, COLOR_BLACK);  // Blue  text
        } else {
            init_pair(1, COLOR_WHITE, COLOR_BLACK); // White text
            init_pair(2, COLOR_GREEN, COLOR_BLACK); // Green text
            init_pair(3, 8, COLOR_BLACK);           // Gray  text
            init_pair(4, COLOR_RED, COLOR_BLACK);   // Red   text
            init_pair(5, COLOR_BLUE, COLOR_BLACK);  // Blue  text
        }
        attron(COLOR_PAIR(1));
    }
}

void Display::setup() {
    setlocale(LC_ALL, "en_US.UTF-8");
    initscr();
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    setupColors();
    updateTermsize();
}

void Display::updateTermsize() {
    getmaxyx(stdscr, termsize[0], termsize[1]);
    termsize[0]--;
    termsize[1]--;
}

void Display::checkResize() {
    if (!resizeNeeded) return;

    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastResizeTime);

    if (elapsed.count() < 100) return;

    lastResizeTime = currentTime;
    resizeNeeded = false;

    endwin();
    refresh();
    clear();
    updateTermsize();

    curs_set(0);

    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    setupColors();

    redrawwin(stdscr);
    refresh();
    setNeedsRedraw(true);
}

int Display::getVisualWidth(const std::string& str) {
    int width = 0;
    const char* ptr = str.c_str();
    wchar_t wc;
    mbstate_t state = {};

    while (*ptr) {
        size_t len = mbrtowc(&wc, ptr, MB_CUR_MAX, &state);
        if (len > 0) {
            width += wcwidth(wc);
            ptr += len;
        } else {
            ptr++;
        }
    }
    return width;
}

std::string Display::colorTags(const std::string &str) {
    std::string result;
    size_t i = 0;
    while (i < str.length()) {
        if (str[i] == '/') {
            size_t endTag = str.find('/', i + 1);
            if (endTag != std::string::npos) {
                i = endTag + 1;
            } else {
                result.push_back(str[i]);
                i++;
            }
        } else {
            result.push_back(str[i]);
            i++;
        }
    }
    return result;
}

void Display::centerPrint(const std::string& str, bool vertical, int y) {
    std::string plainStr = colorTags(str);
    int x;
    if (vertical) {
        y = termsize[0] / 2 - std::count(plainStr.begin(), plainStr.end(), '\n') / 2;
    }

    int visualWidth = getVisualWidth(plainStr);
    x = termsize[1] / 2 - visualWidth / 2;

    int curX = x;
    size_t i = 0;
    while (i < str.length()) {
        if (str[i] == '/') {
            size_t end = str.find('/', i + 1);
            if (end != std::string::npos) {
                std::string colorName = str.substr(i + 1, end - i - 1);
                auto it = colorMap.find(colorName);
                if (it != colorMap.end()) {
                    attron(COLOR_PAIR(it->second));
                }
                i = end + 1;
            } else {
                mvprintw(y, curX, "%s", str.substr(i, 1).c_str());
                curX++;
                i++;
            }
        } else {
            mbstate_t state = {};
            wchar_t wc;
            size_t len = mbrtowc(&wc, &str[i], MB_CUR_MAX, &state);

            if (len > 0 && len <= (str.length() - i)) {
                char buf[MB_CUR_MAX + 1];
                memcpy(buf, &str[i], len);
                buf[len] = '\0';
                mvprintw(y, curX, "%s", buf);

                curX += wcwidth(wc);
                i += len;
            } else {
                mvprintw(y, curX, "%s", str.substr(i, 1).c_str());
                curX++;
                i++;
            }
        }
    }
}

void Display::drawMaze(const MazeGenerator& maze) {
    int mazeWidth = maze.getWidth();
    int mazeHeight = maze.getHeight();

    if (mazeWidth <= 0 || mazeHeight <= 0) {
        return;
    }

    const auto& grid = maze.getGrid();
    const auto& solutionPath = maze.getSolutionPath();
    const auto& explorationPath = maze.getExplorationPath();

    int startY = termsize[0] / 2 - mazeHeight / 2;
    int startX = termsize[1] / 2 - mazeWidth / 2;

    int step = maze.getSolvingStep();
    bool explorationComplete = maze.isExplorationComplete();

    for (int row = 0; row < mazeHeight; row++) {
        for (int col = 0; col < mazeWidth; col++) {
            int yPos = startY + row + offsetY;
            int xPos = startX + col + offsetX;

            if (yPos < 0 || yPos >= termsize[0] || xPos < 0 || xPos >= termsize[1]) {
                continue;
            }

            bool isOnPath = false;
            bool isExplored = false;

            if (maze.isSolving() || !solutionPath.empty()) {
                if (!explorationComplete) {
                    for (int i = 0; i < step && i < static_cast<int>(explorationPath.size()); i++) {
                        if (explorationPath[i].first == row && explorationPath[i].second == col) {
                            isExplored = true;
                            break;
                        }
                    }
                } else {
                    for (const auto& cell : explorationPath) {
                        if (cell.first == row && cell.second == col) {
                            isExplored = true;
                            break;
                        }
                    }

                    for (int i = 0; i < step && i < static_cast<int>(solutionPath.size()); i++) {
                        if (solutionPath[i].first == row && solutionPath[i].second == col) {
                            isOnPath = true;
                            break;
                        }
                    }
                }
            }

            if (isOnPath) {
                attron(COLOR_PAIR(2));
                mvaddch(yPos, xPos, '.');
                attroff(COLOR_PAIR(2));
            } else if (isExplored) {
                attron(COLOR_PAIR(3));
                mvaddch(yPos, xPos, '*');
                attroff(COLOR_PAIR(3));
            } else {
                mvaddch(yPos, xPos, grid[row][col]);
            }
        }
    }
}

void Display::drawUI() {
    centerPrint("/red/↑→↓←/white/ - /gray/Move  /red/S/white/ - /gray/Solve /red/R/white/ - /gray/Regenerate  /red/M/white/ - /gray/Menu/white/  /red/Q/white/ - /gray/Quit/red/", false, termsize[0] - 1);
    centerPrint("/gray/Maze Generator/white/", false, 0);
    centerPrint("/green/seed/white/: /white/[/gray/XXXX-XXXX/white/] ", false, termsize[0] - 3);
}

void Display::validPositionHint(const MazeGenerator& maze) {
    int mazeWidth = maze.getWidth();
    int mazeHeight = maze.getHeight();

    int positionUp = termsize[0] / 2 - mazeHeight / 2 + offsetY;
    int positionDown = termsize[0] / 2 + mazeHeight / 2 + offsetY;
    int positionLeft = termsize[1] / 2 - mazeWidth / 2 + offsetX;
    int positionRight = termsize[1] / 2 + mazeWidth / 2 + offsetX;

    if (positionUp > termsize[0]) mvprintw(termsize[0], termsize[1] / 2 + offsetX, "↑");
    if (positionDown < 0) mvprintw(0, termsize[1] / 2 + offsetX, "↓");
    if (positionLeft > termsize[1]) mvprintw(termsize[0] / 2 + offsetY, termsize[1], "←");
    if (positionRight < 0) mvprintw(termsize[0] / 2 + offsetY, 0, "→");
    if (positionUp > termsize[0] && positionRight < 0) mvprintw(termsize[0], 0, "↗");
    if (positionUp > termsize[0] && positionLeft > termsize[1]) mvprintw(termsize[0], termsize[1], "↖");
    if (positionDown < 0 && positionRight < 0) mvprintw(0, 0, "↘");
    if (positionDown < 0 && positionLeft > termsize[1]) mvprintw(0, termsize[1], "↙");
}

void Display::redraw(const MazeGenerator& maze) {
    erase();
    drawMaze(maze);
    drawUI();
    validPositionHint(maze);
    refresh();
}

int Display::getOffsetX() const {
    return offsetX;
}

int Display::getOffsetY() const {
    return offsetY;
}

void Display::setOffsetX(int x) {
    offsetX = x;
}

void Display::setOffsetY(int y) {
    offsetY = y;
}

void Display::setNeedsRedraw(bool value) {
    needsRedraw = value;
}

bool Display::getNeedsRedraw() const {
    return needsRedraw;
}

void Display::setResizeNeeded(bool value) {
    resizeNeeded = value;
}

bool Display::getResizeNeeded() const {
    return resizeNeeded;
}

int* Display::getTermsize() {
    return termsize;
}
