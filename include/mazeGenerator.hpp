#ifndef MAZEGENERATOR_HPP
#define MAZEGENERATOR_HPP

#include <vector>
#include <random>
#include <queue>
#include <chrono>
#include <functional>
#include <thread>

class MazeGenerator {
private:
    int width;
    int height;
    std::vector<std::vector<char>> grid;
    std::vector<std::pair<int, int>> solutionPath;
    std::mt19937 gen;
    bool solving;
    int solvingStep;
    static const int solvingAnimationDelay = 20;
    std::chrono::steady_clock::time_point lastStepTime;

    int randomInt(int lower, int upper);

public:
    MazeGenerator();
    void setSeed(unsigned int seed);
    void generate(int w, int h);
    void startSolving();
    bool isSolving() const;
    bool solveStep();
    const std::vector<std::vector<char>>& getGrid() const;
    const std::vector<std::pair<int, int>>& getSolutionPath() const;
    int getSolvingStep() const;
    int getWidth() const;
    int getHeight() const;
    void updateCell(int y, int x, char c);
};

#endif
