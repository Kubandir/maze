#include "../include/mazeGenerator.hpp"

MazeGenerator::MazeGenerator() : width(0), height(0), solving(false), solvingStep(0), explorationComplete(false) {
    std::random_device rd;
    gen.seed(rd());
}

void MazeGenerator::setSeed(unsigned int seed) {
    gen.seed(seed);
}

int MazeGenerator::randomInt(int lower, int upper) {
    std::uniform_int_distribution<> dis(lower, upper);
    return dis(gen);
}

void MazeGenerator::generate(int w, int h) {
    width = w;
    height = h;
    solutionPath.clear();
    explorationPath.clear();
    solving = false;
    solvingStep = 0;
    explorationComplete = false;

    grid = std::vector<std::vector<char>>(height, std::vector<char>(width, '#'));

    int startY = 1;
    int startX = 1;
    grid[startY][startX] = ' ';

    std::vector<std::pair<int, int>> stack;
    stack.push_back({startY, startX});

    const int dy[4] = {-2, 0, 2, 0};
    const int dx[4] = {0, 2, 0, -2};

    while (!stack.empty()) {
        int y = stack.back().first;
        int x = stack.back().second;

        std::vector<int> directions;
        for (int i = 0; i < 4; i++) {
            int ny = y + dy[i];
            int nx = x + dx[i];

            if (ny > 0 && ny < height - 1 && nx > 0 && nx < width - 1 && grid[ny][nx] == '#') {
                directions.push_back(i);
            }
        }

        if (directions.empty()) {
            stack.pop_back();
            continue;
        }

        int dir = directions[randomInt(0, directions.size() - 1)];

        int ny = y + dy[dir];
        int nx = x + dx[dir];

        grid[ny][nx] = ' ';
        grid[y + dy[dir]/2][x + dx[dir]/2] = ' ';

        stack.push_back({ny, nx});
    }

    grid[0][1] = ' ';
    grid[height-1][width-2] = ' ';
}

void MazeGenerator::startSolving() {
    if (solving) return;

    solving = true;
    solvingStep = 0;
    solutionPath.clear();
    explorationPath.clear();
    explorationComplete = false;
    lastStepTime = std::chrono::steady_clock::now();

    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    std::vector<std::vector<std::pair<int, int>>> parent(height, std::vector<std::pair<int, int>>(width, {-1, -1}));
    std::queue<std::pair<int, int>> q;

    int startY = 0, startX = 1;
    int targetY = height - 1, targetX = width - 2;

    q.push({startY, startX});
    visited[startY][startX] = true;
    explorationPath.push_back({startY, startX});

    const int dy[4] = {-1, 0, 1, 0};
    const int dx[4] = {0, 1, 0, -1};

    bool found = false;
    while (!q.empty()) {
        auto [y, x] = q.front();
        q.pop();

        if (y == targetY && x == targetX) {
            found = true;
            break;
        }

        for (int i = 0; i < 4; i++) {
            int ny = y + dy[i];
            int nx = x + dx[i];

            if (ny >= 0 && ny < height && nx >= 0 && nx < width &&
                !visited[ny][nx] && grid[ny][nx] == ' ') {
                visited[ny][nx] = true;
                parent[ny][nx] = {y, x};
                q.push({ny, nx});
                explorationPath.push_back({ny, nx});
            }
        }
    }

    if (found) {
        int py = targetY, px = targetX;
        std::vector<std::pair<int, int>> path;

        while (!(py == startY && px == startX)) {
            path.push_back({py, px});
            auto p = parent[py][px];
            py = p.first;
            px = p.second;
        }
        path.push_back({startY, startX});

        for (int i = path.size() - 1; i >= 0; i--) {
            solutionPath.push_back(path[i]);
        }
    } else {
        solving = false;
    }
}

bool MazeGenerator::isSolving() const {
    return solving;
}

bool MazeGenerator::solveStep() {
    if (!solving) return false;

    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - lastStepTime).count();

    if (elapsedTime < solvingAnimationDelay) {
        return true;
    }

    lastStepTime = currentTime;

    if (!explorationComplete) {
        if (solvingStep < static_cast<int>(explorationPath.size())) {
            solvingStep++;
            return true;
        } else {
            explorationComplete = true;
            solvingStep = 0;
            return true;
        }
    } else {
        if (solvingStep < static_cast<int>(solutionPath.size())) {
            solvingStep++;
            return true;
        } else {
            solving = false;
            return false;
        }
    }
}

int MazeGenerator::getSolvingStep() const {
    return solvingStep;
}

const std::vector<std::vector<char>>& MazeGenerator::getGrid() const {
    return grid;
}

const std::vector<std::pair<int, int>>& MazeGenerator::getSolutionPath() const {
    return solutionPath;
}

const std::vector<std::pair<int, int>>& MazeGenerator::getExplorationPath() const {
    return explorationPath;
}

bool MazeGenerator::isExplorationComplete() const {
    return explorationComplete;
}

int MazeGenerator::getWidth() const {
    return width;
}

int MazeGenerator::getHeight() const {
    return height;
}

void MazeGenerator::updateCell(int y, int x, char c) {
    if (y >= 0 && y < height && x >= 0 && x < width) {
        grid[y][x] = c;
    }
}
