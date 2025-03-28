#include <iostream>
#include <vector>
#include <ncurses.h>
#include <random>
#include <csignal>
#include <chrono>
#include <thread>
#include "../include/mazeGenerator.hpp"
#include "../include/display.hpp"

bool resizeNeeded = false;
MazeGenerator* globalMazePtr = nullptr;
Display* globalDisplayPtr = nullptr;

void signalHandler(int signum) {
    if (globalDisplayPtr) {
        endwin();
    }
    std::cout << "Goodbye!" << std::endl;
    exit(signum);
}

void resizeHandler(int) {
    resizeNeeded = true;
}

void mainLoop(Display& display, MazeGenerator& maze) {
    auto lastFrameTime = std::chrono::steady_clock::now();
    const int targetFrameTimeMs = 10;

    while (true) {
        if (resizeNeeded) {
            display.checkResize();
            resizeNeeded = false;
        }

        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;

        bool positionChanged = false;
        if (ch == KEY_UP) {
            display.setOffsetY(display.getOffsetY() + 1);
            positionChanged = true;
        }
        if (ch == KEY_DOWN) {
            display.setOffsetY(display.getOffsetY() - 1);
            positionChanged = true;
        }
        if (ch == KEY_LEFT) {
            display.setOffsetX(display.getOffsetX() + 1);
            positionChanged = true;
        }
        if (ch == KEY_RIGHT) {
            display.setOffsetX(display.getOffsetX() - 1);
            positionChanged = true;
        }
        if (ch == 's' || ch == 'S') {
            maze.startSolving();
            display.setNeedsRedraw(true);
        }
        if (ch == 'r' || ch == 'R') {
            std::random_device rd;
            maze.setSeed(rd());
            maze.generate(41, 21);
            display.setNeedsRedraw(true);
        }

        if (maze.isSolving()) {
            bool stepAdvanced = maze.solveStep();
            if (stepAdvanced) {
                display.setNeedsRedraw(true);
            }
        }

        if (positionChanged) {
            display.setNeedsRedraw(true);
        }

        if (display.getNeedsRedraw()) {
            display.redraw(maze);
            display.setNeedsRedraw(false);
        }

        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFrameTime).count();

        if (elapsed < targetFrameTimeMs) {
            napms(targetFrameTimeMs - elapsed);
        }

        lastFrameTime = std::chrono::steady_clock::now();
    }
}

int main() {
    try {
        Display display;
        MazeGenerator maze;

        globalMazePtr = &maze;
        globalDisplayPtr = &display;

        display.setup();
        display.updateTermsize();

        std::random_device rd;
        maze.setSeed(rd());
        maze.generate(41, 21);

        mainLoop(display, maze);

        endwin();
        return 0;
    }
    catch (const std::exception& e) {
        endwin();
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
}
