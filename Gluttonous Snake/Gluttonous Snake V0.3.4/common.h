#pragma once

#include<Windows.h>

#define SNAKE_UPDATE 20 //初始蛇蛇刷新
#define SNAKE_LEN 3     //初始蛇长
#define LOW_DOWN_TIME 200 //最短按下时间
#define FPS_LOGIC 60.0  //逻辑刷新率

// ========== 状态ID表 ==========
#define STA_END 0
#define STA_LOBBY 1
#define STA_SELECT 2
#define STA_PAUSE 3
#define STA_OVER 4
#define STA_DEATH 6
#define STA_PU_GAME 7 //游戏模式放最后方便扩展方便
#define STA_AT_GAME 8
#define STA_EX_GAME 9


//上右下左
extern const int dx[4];
extern const int dy[4];


extern RECT Larea, Marea, Rarea, Sarea;

struct Position {
    int x = 0;
    int y = 0;

    // 成员函数形式重载 ==
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};



