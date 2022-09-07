#pragma once

#include "framework.h"
#include "resource.h"
#include <memory>

using namespace std;
//스마트 포인터
struct Point {
    int x, y;
};

enum class Type {
    EMPTY = 0,
    PINK = 1, 
    RED = 2,
    GREEN = 3,
    BLUE = 4,
    EMERALD = 5,
    GRAY = 6,
    LIGHTBLUE = 7,
    YELLOW = 8,
    DARKGREEN = 9,
    WALL = 10
};

enum class GameStatus
{
    GAMEOVER,
    RUNNING,
    PAUSE
};