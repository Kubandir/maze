#include <iostream>
#include <vector>
#include <ncurses.h>
#include <random>
#include <csignal>
#include <string>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <locale.h>
#include <wchar.h>  // Add wide character support
#include <cstring>  // For mbstowcs

int offsetX = 0;
int offsetY = 0;

bool needs_redraw = true;
std::random_device rd;
std::mt19937 gen(rd());

int randomInt(int x, int y) {
    int lower = std::min(x, y);
    int upper = std::max(x, y);

    std::uniform_int_distribution<> dis(lower, upper);

    return dis(gen);
}

void signalHandler(int signum) {
    endwin();
    clear();
    std::cout << "Goodbye!" << std::endl;
    exit(signum);
}

int termsize[2] {0, 0};
bool resize_needed = false;
auto last_resize_time = std::chrono::steady_clock::now();

void update_termsize() {
    getmaxyx(stdscr, termsize[0], termsize[1]);
    termsize[0]--;
    termsize[1]--;
}

void resizeHandler(int) {
    resize_needed = true;
}

void needResize() {
    if (!resize_needed) return;

    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_resize_time);

    if (elapsed.count() < 100) return;

    last_resize_time = current_time;
    resize_needed = false;

    endwin();
    refresh();
    clear();
    update_termsize();

    redrawwin(stdscr);
    refresh();
    needs_redraw = true;
}

void setup() {
    setlocale(LC_ALL, "en_US.UTF-8");

    signal(SIGINT, signalHandler);
    signal(SIGWINCH, resizeHandler);

    initscr();
    curs_set(0);
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);


    if (has_colors()) {
        start_color();
        if (!can_change_color()) {
            init_pair(1, COLOR_WHITE, COLOR_BLACK);
            init_pair(2, COLOR_GREEN, COLOR_BLACK);
            init_pair(3, COLOR_WHITE, COLOR_BLACK);
            init_pair(4, COLOR_RED, COLOR_BLACK);
        } else {
            init_pair(1, COLOR_WHITE, COLOR_BLACK);
            init_pair(2, COLOR_GREEN, COLOR_BLACK);
            init_pair(3, 8, COLOR_BLACK);
            init_pair(4, COLOR_RED, COLOR_BLACK);
        }
        attron(COLOR_PAIR(1));
    }

    update_termsize();
}

int charCount(const std::string& str) {
    int count = 0;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\n') {
            count++;
        }
    }
    return count;
}

int getVisualWidth(const std::string& str) {
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

std::string colorTags(const std::string &str) {
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

void centerPrint(std::string str, bool vertical, int y) {
    update_termsize();
    std::string plainStr = colorTags(str);
    int x;
    if (vertical) {
        y = termsize[0] / 2 - charCount(plainStr) / 2;
    }

    int visual_width = getVisualWidth(plainStr);
    x = termsize[1] / 2 - visual_width / 2;

    std::unordered_map<std::string, int> colorMap{
        {"white", 1},
        {"green", 2},
        {"gray", 3},
        {"red", 4}
    };

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

int width = 0;
int height = 0;
std::vector<std::vector<char>> grid;

void mazeGen(int w, int h) {
    width = w;
    height = h;

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

void display() {
    int startY = termsize[0] / 2 - height / 2;
    int startX = termsize[1] / 2 - width;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            mvaddch(startY + row + offsetY, startX + col + offsetX, grid[row][col]);
        }
    }
}

void updateCell(int y, int x, char c) {
    grid[y][x] = c;
    needs_redraw = true;

    if (y >= 0 && y < height && x >= 0 && x < width) {
        mvaddch(y, x, c);
        refresh();
    }
}

int main() {
    setup();
    update_termsize();

    mazeGen(41, 21);

    auto last_frame_time = std::chrono::steady_clock::now();
    const int target_frame_time_ms = 10;

    while(1) {
        needResize();

        int ch = getch();
        if (ch == 'q') break;

        bool position_changed = false;
        if (ch == KEY_UP) { offsetY--; position_changed = true; }
        if (ch == KEY_DOWN) { offsetY++; position_changed = true; }
        if (ch == KEY_LEFT) { offsetX--; position_changed = true; }
        if (ch == KEY_RIGHT) { offsetX++; position_changed = true; }

        if (ch == 'r') {
            mazeGen(41, 21);
            needs_redraw = true;
        }

        if (position_changed) {
            needs_redraw = true;
        }

        if (needs_redraw) {
            erase();
            display();
            centerPrint("/red/↑→↓←/white/ - /gray/Move  /red/R/white/ - /gray/Regenerate  /red/Q/white/ - /gray/Quit/white/", false, termsize[0] - 1);

            refresh();
            needs_redraw = false;
        }

        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_frame_time).count();

        if (elapsed < target_frame_time_ms) {
            napms(target_frame_time_ms - elapsed);
        }

        last_frame_time = std::chrono::steady_clock::now();

        if (ch == ERR) {
            napms(10);
        }
    }
    endwin();
    return 0;
}
