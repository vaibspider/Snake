/*
 * Copyright (c) Abhyudaya Sharma, 2018
 */

#include <stdio.h>
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// PDCurses defines bool as unsigned char
// Use common way to invoke strcpy
// Uses windows.h and unistd.h for sleep functions
#ifdef _WIN32
#define true TRUE
#define false FALSE
#include <windows.h>

#define stringCopy(dest, size, source) strcpy_s(dest, size, source)
#else
#include <unistd.h>
#include <stdbool.h>

#define stringCopy(dest, size, source) strncpy(dest, source, size)
#endif

#define SNAKE_HEAD ACS_DIAMOND 
#define SNAKE_LEN 20
#define ADD_POINT(point) do { mvaddch(point.y, point.x, ACS_BLOCK); } while (0)
#define MAX_REASON_LEN 100

struct Point {
    int y;
    int x;
};

inline bool arePointsEqual(struct Point x, struct Point y) {
    return x.x == y.x && x.y == y.y;
}

void initCurses() {
    initscr();
    raw();
    noecho();
    clear();
    start_color();
    keypad(stdscr, TRUE);
    curs_set(0); // Hide the cursor
    init_pair(1, COLOR_RED, COLOR_BLACK);
}

// Acts as a splash screen
bool runGame() {
    printw("Hello World.\nA simple movement tool.\nPress 'q' to quit or press any other key to continue...");
    refresh();
    int ch = getch();

    if (ch == 'q') {
        return false;
    } else {
        return true;
    }
}

inline bool isInBounds(int y, int x) {
    int maxX, maxY;
    getmaxyx(stdscr, maxY, maxX);
    if (y >= 0 && y < maxY && x >= 0 && x < maxX) {
        return true;
    } else {
        return false;
    }
}

bool isOverlapped(struct Point p, const struct Point* points, int snakeLen) {
    for (int i = 0; i < snakeLen; i++) {
        if (arePointsEqual(p, points[i])) {
            return true;
        }
    }

    return false;
}

void endGame(const char* reason, unsigned int score) {
    timeout(-1); // Get normal input
	clear();
    flushinp(); // Clear input buffer

    attron(A_BOLD);
    addstr("Game Over!\n");
    attroff(A_BOLD);

    addstr(reason);
    addch('\n');
    addstr("Your Score: ");
    char scoreStr[13]; // 32 bit ints should fit in a 12 characters + 1 \n char
    snprintf(scoreStr, 13, "%u\n", score);
    addstr(scoreStr);
    refresh();

    // sleep
    int time = 3; // seconds
#ifdef _WIN32
    Sleep(time * 1000);
#else
    sleep(time);
#endif

    addstr("Press any key to exit...");
    flushinp(); // Clear input buffer
    getch();
    
    refresh();
}

/*
 * reason is changed iff the function returns false
 */
bool moveSnake(struct Point* points, int snakeLen, int dy, int dx, char* reason) {
    // restrict moving off the screen
    if (!isInBounds(points[0].y + dy, points[0].x + dx)) {
        stringCopy(reason, MAX_REASON_LEN, "Your snake went out of bounds!");
        return false; // Reaching the bounds means the end of the game.
    }

    // check moved
    if (isOverlapped((struct Point) { points[0].y + dy, points[0].x + dx }, points, snakeLen)) {
        stringCopy(reason, MAX_REASON_LEN, "Your snake ate itself!");
        return false;
    }

    clear();

    // add snake's body points
    for (int i = snakeLen- 1; i > 0; i--) {
        points[i].y = points[i - 1].y;
        points[i].x = points[i - 1].x;

        ADD_POINT(points[i]);
    }

    points[0].y += dy;
    points[0].x += dx;
    mvaddch(points[0].y, points[0].x, SNAKE_HEAD);
    return true;
}

/*
 * Returns the timeout (ms) based on the difficulty
 * Higher the level, less is the timeout
 * Timeout is the delay between the snake moving automatically.
 * At lower levels (higer timeouts), the snake can be accelerated
 * by keeping the key pressed.
 */
int getTimeout(int level) {
    // TODO Create a difficulty based timeout
    return 200;
}

void validateInput(int* input) {
    switch (*input) {
        case KEY_UP:
        case KEY_DOWN:
        case KEY_RIGHT:
        case KEY_LEFT:
        case 'q':
            break;
        default:
            *input = ERR;
    }
}

/* Returns the location of the added food */
struct Point addFood(bool eaten, const struct Point* points, int snakeLen) {
    static int foodX;
    static int foodY;

    int maxX;
    int maxY;

    getmaxyx(stdscr, maxY, maxX);

    if (eaten) {
        // Check if the food is not part of the snake
        do {
            foodY = rand() % maxY;
            foodX = rand() % maxX;
        } while (isOverlapped((struct Point) {foodY, foodX}, points, snakeLen));
    }

    // Food is coloured
    attron(COLOR_PAIR(1)); 
    mvaddch(foodY, foodX, ACS_BLOCK);
    attroff(COLOR_PAIR(1));

    refresh();
    return (struct Point) { foodY, foodX };
}

void resetSnake(struct Point* points, int snakeLen) {
    clear();
    int maxX, maxY;
    getmaxyx(stdscr, maxY, maxX);

    int curX = maxX / 2;
    int curY = maxY / 2;

    // Snake head
    mvaddch(curY, curX, SNAKE_HEAD);
    points[0].y = curY;
    points[0].x = curX;

    // Snake body
    for (int i = 1; i < snakeLen; i++) {
        points[i].y = points[i - 1].y;
        points[i].x = points[i - 1].x - 1;
        ADD_POINT(points[i]);
    }
}

void loop() {
    // head is points[0]
    struct Point points[SNAKE_LEN];
    resetSnake(points, SNAKE_LEN);
    
    // add food
    struct Point foodLocation = addFood(true, points, SNAKE_LEN);
    refresh();

    bool flag = true;
    int direction = KEY_RIGHT; // Snake is initially aligned along the +x direction
    bool moved = false;
    bool getInput = true;
    unsigned int score = 0;
    char gameOverReason[MAX_REASON_LEN];

    while (flag) {
        // Check for terminal size change
        if (!isInBounds(points[0].y, points[0].x)) {
            resetSnake(points, SNAKE_LEN);
            foodLocation = addFood(points, SNAKE_LEN, true);
            refresh();
        }
        
        addFood(false, points, SNAKE_LEN);

        int input = ERR;
        if (getInput) {
            input = getch();
        }

        validateInput(&input);
        input = (input == ERR) ? direction : input;

        switch (input) {
            case KEY_UP:
                if (direction != KEY_DOWN) {
                    moved = moveSnake(points, SNAKE_LEN, -1, 0, gameOverReason);
                    direction = input;
                    getInput = true;
                } else {
                    getInput = false;
                    continue;
                }
                break;
            case KEY_DOWN:
                if (direction != KEY_UP) {
                    moved = moveSnake(points, SNAKE_LEN, 1, 0, gameOverReason);
                    direction = input;
                    getInput = true;
                } else {
                    getInput = false;
                    continue;
                }
                break;
            case KEY_LEFT:
                if (direction != KEY_RIGHT) {
                    moved = moveSnake(points, SNAKE_LEN, 0, -1, gameOverReason);
                    direction = input;
                    getInput = true;
                } else {
                    getInput = false;
                    continue;
                }
                break;
            case KEY_RIGHT:
                if (direction != KEY_LEFT) {
                    moved = moveSnake(points, SNAKE_LEN, 0, 1, gameOverReason);
                    direction = input;
                    getInput = true;
                } else {
                    getInput = false;
                    continue;
                }
                break;
            case 'q':
                stringCopy(gameOverReason, MAX_REASON_LEN, "Why did you quit? You were doing quite well!");
                flag = false;
                break;
        }
        
        // Terminate execution if snake was unable to move
        if (!moved) {
            flag = false;
        }

        // Check for eating food
        if (arePointsEqual(points[0], foodLocation)) {
            foodLocation = addFood(true, points, SNAKE_LEN);
            score++;
        }

        refresh();
        timeout(getTimeout(0)); // Start automatic movement after first keystroke
    }

    endGame(gameOverReason, score);
}

int main(int argc, char ** argv) {
    // Init curses
    initCurses();
    srand(time(NULL)); // Initialize rand with seed as current time

    if (runGame()) {
        /* Main logic of the program */
        loop();
    }

    endwin(); // cleanup curses
    return 0;
}
