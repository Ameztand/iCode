#pragma once

#include<queue>
#include <vector>
#include <cstdlib>  // rand(), srand()

#include "common.h"
#include "context.h"
//#include "gameState.h"


// ========== 玩家管理器 ===========
class Player {
private:
    //下一格(检测是否死亡用)
    Position NextPos;

    int dir_ = 0;//方向参数
    int purrDir_ = 0;//刷新时修改，记录上一个方向防止往回走

    //帧率计数器
    int V = SNAKE_UPDATE;//蛇（移动）刷新速度？
    int Frame = 0;//限制蛇刷新速度（移动速度）

    int E = 0;//极限模式加速能量参考

    // 蛇坐标队列
    std::queue<Position> posQueue;

    //存活状态
    bool Live_ = { true };

public:
    //初始化
    void initPlayerData(int x, int y, int dir);

    //蛇头前一格相关
    const Position& getNextPos() const;

    void setNextPos(int x, int y);

    //存活状态相关
    bool getLive() const;

    void setLive(bool item);

    //移动输入方面
    void up();

    void right();

    void down();

    void left();

    void setDir(int item);

    void updataPurrDir();

    int getDir() const;

    //队列方面
    void pushQ(int x, int y);

    void popQ();

    void clearQ();

    bool emptyQ();

    size_t sizeQ() const;

    const Position& getFront()const;

    const Position& getBack()const;

    const std::queue<Position>& getQueue()const;

    //速度方面
    void setV(long long item);

    int getV() const;

    void setFrame(int F);

    void FrameAdd();

    int getFrame() const;

    //能量方面
    int getE() const;

    void setE(int item);
};

// ========== 逻辑层===========
class Logic {
private:
    //极限模式主体对象
    bool mirror = false;

    //地图用于判断是否是蛇身，判断果子用数组
    int MapData[19][19] = { 0 };//0路 1蛇 2果子 4蛇头 5原点
    struct Position DotData[3] = { 0 };//果子坐标

    std::vector<Player>PlayerList;//玩家列表

public:
    //极限模式控制主体相关
    void Mirror();

    int getMirror()const;

    void initMirror();

    //玩家列表相关
    void addPlayer(int x, int y, int dir);

    void clearPlayerList();

    const std::vector<Player>& getPlayerList() const;

    Player& getPlayer(size_t index);

    const Player& getPlayer(size_t index) const;

    //地图相关
    int getMapData(int x, int y) const;

    void clearMapData();

    void initMapData(int mode);

    //果子相关
    void initDotData();

    const Position& getDotData(int i) const;

    void CreatDot(int nx, int ny, int i);

    void clearDot(int i);

    //判断移动请求
    void judgeMoveRequest(Context& ctx, Player& player);

    //更新渲染数据数据
    void updataData(Context& ctx, Player& player);

    //获取失败者
    int getLoser(Context& ctx);

    int Random();
};
