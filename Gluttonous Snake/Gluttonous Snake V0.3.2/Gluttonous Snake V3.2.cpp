/*
26.4.20V3.2版贪吃蛇

采用状态栈架构，封装在Stata类里并且需要手动把状态ID push到类里（方便其他函数比较当前状态）
并且提供一个获取栈深度的接口在某些场合化简比较多个栈状态（便于后续扩展游戏模式）
同时，关于游戏时间，检测空格长按和限制刷新率也封装在了类里面

相比3.1，消去了全局对象，改为依赖注入，但是依旧很乱
优化死亡动画（添加X）
解决撞尾问题
*/


#define SNAKE_UPDATE 20 //初始蛇蛇刷新
#define FPS_LOGIC 60.0  //逻辑刷新率
#define LOW_DOWN_TIME 200 //最短按下时间
#define X_CODE_PIONE  347   //(0,0)左上坐标
#define Y_CODE_PIONE  373   //(0,0)右下坐标
#define SNAKE_LEN 3     //初始蛇长

// ========== 状态ID表 ==========
#define STA_END 0
#define STA_LOBBY 1
#define STA_SELECT 2
#define STA_PAUSE 3
#define STA_OVER 4
#define STA_ACCOUNT 5//竞技模式结算画面
#define STA_DEATH 6
#define STA_PU_GAME 7 //游戏模式放最后方便扩展方便
#define STA_AT_GAME 8
#define STA_EX_GAME 9




#include <queue>//为了队列
#include <stack>//为了栈
#include <iostream>//数据流
#include <vector>//动态数组
#include <algorithm>//快速清空数组
#include <functional>
//#include <map>
#include <typeindex>
#include <easyx.h>
#include <windows.h>  // 为了 Sleep
#include <chrono>
#include <thread>
#include <cstdlib>  // rand(), srand()

#pragma comment(lib, "winmm.lib")   // 添加这一行



struct Position {
    int x = 0;
    int y = 0;

    // 成员函数形式重载 ==
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

class Stata;
class Logic;
class Renderer;
class Msg;
class Time;

struct Context {
    Stata& stata;
    Logic& logic;
    Renderer& renderer;

    Msg& msg;

    Time& gameTime;//记录游戏时间
    Time& backTime;//空格按下时间
    Time& enterTime;//回车按下时间
};

RECT Larea = { 362, 402, 458, 448 };//左按钮
RECT Marea = { 492, 402, 588, 448 };//中按钮
RECT Rarea = { 622, 402, 718, 448 };//右按钮
RECT Sarea = { 492, 502, 588, 548 };//开始按钮

void InitLobby(Context& ctx);   void Lobby(Context& ctx);
void InitSelect(Context& ctx);  void Select(Context& ctx);

void InitPause(Context& ctx);   void Pause(Context& ctx);
void InitOver(Context& ctx);    void Over(Context& ctx);
void InitAccount(Context& ctx); void Account(Context& ctx);

void InitDeathAnim(Context& ctx); void DeathAnim(Context& ctx);

void InitPuGameData(Context& ctx); void PuGame(Context& ctx);
void InitATGameData(Context& ctx); void ATGame(Context& ctx);
void InitEXGameData(Context& ctx); void EXGame(Context& ctx);

void returnToLobby(Context& ctx);
void clearPastData(Context& ctx);
void MoveMsg(Context& ctx, int p1, int p2);
void MirrorMoveMsg(Context& ctx, int p1, int p2);

// =========== 时间管理器 ===========
class Time {
private:
    long long start_ = 0;
    long long curr_ = 0;
    long long sum_ = 0;
    long long item_ = 0;

public:
    //Time() : start(0), curr(0), sum(0), item(0) {}

    void start() {
        start_ = clock();
    }

    void updata() {
        curr_ = clock();
        sum_ = curr_ - start_;
    }

    void pause() {
        curr_ = clock();
        sum_ = curr_ - start_;
        item_ += sum_;
        start_ = 0;
    }

    void resume() {
        start_ = clock();
    }

    long long getSum() const {
        return sum_ + item_;
    }

    void clear() {
        sum_ = 0;
        item_ = 0;
        start_ = 0;
    }

    bool empty() const {
        return start_ == 0;
    }
};

// ========== 状态栈管理器 ==========
class Stata {
private:
    std::stack<std::function<void(Context&)>>stack;//状态栈
    std::stack<int>StaID;//状态ID

public:
    // 压栈
    void push(std::function<void(Context&)> func, int id) {
        stack.push(func);
        StaID.push(id);
    }

    //弹出
    void pop() {
        if (!stack.empty()) {
            stack.pop();
            StaID.pop();
        }
    }

    //（更新栈顶）运行该模块
    void update(Context& cxt) {
        if (!stack.empty()) {
            stack.top()(cxt);
        }
    }

    //是否为空
    bool empty() {
        return stack.empty();
    }

    //获取状态id
    int getStaID() const {
        return stack.empty() ? STA_END : StaID.top();
    }

    // 获取栈深度
    size_t getStaDepth() const {
        return stack.size();
    }
};

// ========== 玩家管理器 ===========
class Player {
private:
    //下一格
    Position NextPos;

    //方向参数
    int dir_ = 0;//考虑用dydx合成速度
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
    //蛇头前一格相关
    const Position& getNextPos() const {
        return NextPos;
    }

    void initNextPos(int x, int y) {
        NextPos = { x,y };
    }

    void setNextPos(int x, int y) {
        const Position& item = { x,y };
        NextPos = item;
    }

    //存活状态相关
    bool getLive() const {
        return Live_;
    }

    void setLive(bool item) {
        Live_ = item;
    }

    //移动输入方面
    void up() {
        if (purrDir_ != 2) dir_ = 0;
    }

    void right() {
        if (purrDir_ != 3) dir_ = 1;
    }

    void down() {
        if (purrDir_ != 0) dir_ = 2;
    }

    void left() {
        if (purrDir_ != 1) dir_ = 3;
    }

    void updataDir() {
        dir_ = 0;
    }

    void updataPurrDir() {//防止向后转
        purrDir_ = dir_;
    }

    int getDir() const {
        return dir_;
    }

    void setDir(int dir) {
        dir_ = dir;
    }

    //队列方面
    void pushQ(int x, int y) {
        posQueue.push({ x,y });
    }

    void popQ() {
        posQueue.pop();
    }

    void clearQ() {
        while (!posQueue.empty()) {
            posQueue.pop();
        }
    }

    bool emptyQ() {
        return posQueue.empty();
    }

    size_t sizeQ() const {
        return posQueue.size();
    }

    const Position& getFront()const {
        return posQueue.front();
    }

    const Position& getBack()const {
        return posQueue.back();
    }

    const std::queue<Position>& getQueue()const {
        return posQueue;
    }

    //速度方面
    void setV(long long item) {
        if (LOW_DOWN_TIME <= item && item <= 4 * LOW_DOWN_TIME) {
            V = (SNAKE_UPDATE * 600) / (item + 400);
        }
        else if (4 * LOW_DOWN_TIME <= item) {
            V = 4 + 400 * (SNAKE_UPDATE - 8) / item;
        }
        else {
            V = SNAKE_UPDATE;
        }
    }

    void setFrame(int F) {
        Frame = F;
    }

    int getV() const {
        return V;
    }

    void FrameAdd() {
        Frame++;
    }

    void clearFrame() {
        Frame = 0;
    }

    int getFrame() const {
        return Frame;
    }

    //能量方面
    int getE() const {
        return E;
    }

    void setE(int item) {
        E = item;
    }

    void initE() {
        E = 0;
    }
};

// ========== 监听层 ===========
class Msg {
private:
    int keyStates[256] = { 0 };//<0松开 >0按下 +-2有一个可消费脉冲，+-1表示无脉冲
    bool leftButtonDown_ = false;

    ExMessage msg_;
    POINT mousePos_;



public:
    void GetMsg() {
        while (peekmessage(&msg_, EX_MOUSE | EX_KEY)) {
            if (msg_.message == WM_KEYDOWN) {
                if (msg_.vkcode >= 0 && msg_.vkcode < 256) {
                    keyStates[msg_.vkcode] = 2;
                }
            }
            else if (msg_.message == WM_KEYUP) {
                if (msg_.vkcode >= 0 && msg_.vkcode < 256) {
                    keyStates[msg_.vkcode] = -2;
                }
            }
            else if (msg_.message == WM_LBUTTONDOWN) {
                leftButtonDown_ = true;
                mousePos_ = { msg_.x, msg_.y };
            }
            else if (msg_.message == WM_LBUTTONUP) {
                leftButtonDown_ = false;
                mousePos_ = { msg_.x, msg_.y };
            }
        }

    }

    //键盘按下情况相关
    bool getKeySta(int keyCode) const {
        if (keyCode > 0 && keyCode <= 256 && keyStates[keyCode] > 0) {
            return true;
        }
        return false;
    }

    bool getKeyDown(int keyCode) {
        if (keyCode > 0 && keyCode <= 256 && keyStates[keyCode] == 2) {
            keyStates[keyCode] = 1;
            return true;
        }
        return false;
    }

    bool getKeyUp(int keyCode) {
        if (keyCode > 0 && keyCode <= 256 && keyStates[keyCode] == -2) {
            keyStates[keyCode] = -1;
            return true;
        }
        return false;
    }

    void clearKey() {
        std::fill_n(keyStates, 256, 0);
    }

    //鼠标按下情况相关
    const POINT& getMousePos()const {
        return mousePos_;
    }

    bool getMouse()const {
        return leftButtonDown_;
    }

    void clearMouse() {
        leftButtonDown_ = false;
    }
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

    //上右下左
    const int dx[4] = { 0,1,0,-1 };
    const int dy[4] = { 1,0,-1,0 };

public:
    //极限模式控制主体相关
    void Mirror() {
        mirror = mirror == 0 ? 1 : 0;
    }

    int getMirror()const {
        return mirror;
    }

    void initMirror() {
        mirror = 0;
    }

    //玩家列表相关
    void addPlayer(int x, int y, int dir) {
        int pos[3][2];
        pos[0][0] = x;
        pos[0][1] = y;

        for (int i = 1; i < SNAKE_LEN; i++) {
            pos[i][0] = pos[i - 1][0] - dx[dir];
            pos[i][1] = pos[i - 1][1] - dy[dir];
        }

        Player p;

        for (int i = SNAKE_LEN - 1; i >= 0; i--) {
            p.pushQ(pos[i][0], pos[i][1]);
        }
        p.initNextPos(x + dx[dir], y + dy[dir]);
        p.setDir(dir);
        p.initE();
        PlayerList.push_back(p);
    }

    void clearPlayerList() {
        while (!PlayerList.empty()) {
            PlayerList.pop_back();
        }
    }

    const std::vector<Player>& getPlayerList() const {
        return PlayerList;
    }

    Player& getPlayer(size_t index) {
        return PlayerList.at(index);
    }

    const Player& getPlayer(size_t index) const {
        return PlayerList.at(index);
    }

    //地图相关
    int getMapData(int x, int y) const {//输入坐标 返回状态
        return MapData[x + 9][-y + 9];
    }

    void clearMapData() {
        for (int i = 0; i < 19; i++) {
            for (int j = 0; j < 19; j++) {
                MapData[i][j] = 0;
            }
        }
    }

    void initMapData(int mode) {
        //存入地图
        if (mode == STA_PU_GAME) {
            MapData[9][9] = 4; //（0，0）
            MapData[9][10] = 1;//（0，-1）
            MapData[9][11] = 1;//（0，-2）
        }
        else if (mode == STA_AT_GAME) {
            MapData[4][9] = 4; //（-5，0）
            MapData[4][10] = 1;//（-5，-1）
            MapData[4][11] = 1;//（-5，-2）
            MapData[14][9] = 4; //（5，0）
            MapData[14][10] = 1;//（5，-1）
            MapData[14][11] = 1;//（5，-2）
        }
    }

    //果子相关
    void initDotData() {
        for (int i = 0; i < 3; i++) {
            DotData[i] = { 10,10 };
            CreatDot(0, 0, i);
        }
    }

    const Position& getDotData(int i) const {//输入坐标 返回状态
        return DotData[i];
    }

    void CreatDot(int nx, int ny, int i) {//生成一个果子
        //获取随机数
        int x = Random() * (Random() > 4 ? 1 : -1);
        int y = Random() * (Random() > 4 ? 1 : -1);
        while (MapData[x + 9][-y + 9] != 0 || (x == nx && y == ny) || (x == 0 && y == 0)) {
            x = Random() * (Random() > 4 ? 1 : -1);
            y = Random() * (Random() > 4 ? 1 : -1);
        }

        //放到数组
        DotData[i] = { x,y };
    }

    void clearDot(int i) {
        DotData[i] = { 10,10 };
    }

    //判断移动请求
    void judgeMoveRequest(Context& ctx, Player& player) {
        //获取下一格坐标
        const Position& bpos = player.getBack();
        int dir = player.getDir();
        int nx = bpos.x + dx[dir];
        int ny = bpos.y + dy[dir];
        player.setNextPos(nx, ny);

        //判断下一格
        int ItemMapData = MapData[nx + 9][-ny + 9];
        if (nx < -9 || ny < -9 || nx>9 || ny>9 || ItemMapData == 4) {
            //gameover
            player.setLive(false);
        }
        else if (ItemMapData == 1) {
            bool isBoby = { true };
            for (const Player& player : PlayerList) {// 遍历所有玩家进行绘制
                const Position& fpos = player.getFront();
                if (fpos.x == nx && fpos.y == ny) {
                    isBoby = false;
                }
            }
            if (isBoby) {
                //gameover
                player.setLive(false);
            }
        }
    }

    //更新渲染数据数据
    void updataData(Context& ctx, Player& player) {
        //万一呢
        if (!player.getLive()) return;

        //获取下一格坐标
        const Position& pos = player.getBack();
        int dir = player.getDir();
        int nx = pos.x + dx[dir];
        int ny = pos.y + dy[dir];

        //检测果子是否被吃
        bool EatDot = { false };
        for (int i = 0; i < 3; i++) {
            if (DotData[i].x == nx && DotData[i].y == ny) {
                EatDot = true;
                CreatDot(nx, ny, i);
                break;
            }
        }
        if (!EatDot) {//消去尾巴
            const Position& pos = player.getFront();
            int px = pos.x;
            int py = pos.y;
            MapData[px + 9][-py + 9] = 0;

            player.popQ();
        }

        //载入地图、队列、更新方向参数
        MapData[pos.x + 9][-pos.y + 9] = 1;
        MapData[nx + 9][-ny + 9] = 4;
        player.pushQ(nx, ny);
        player.setNextPos(nx, ny);
        player.updataPurrDir();

        if (ctx.stata.getStaID() == STA_EX_GAME) {
            int x = player.sizeQ();
            int y = (-5 * x * x + 205 * x - 500) / 2;
            player.setV(y);
            player.setE(y);
            //printf("%d\n", y);
        }

        /*
        // 函数1
        if (g_GameSta.getStaID() == STA_EX_GAME) {
            int x = player.sizeQ();
            int y = -x * x + 65 * x - 100;
            player.setV(y);
        }

        // 函数2
        if (g_GameSta.getStaID() == STA_EX_GAME) {
            int x = player.sizeQ();
            int y = -2 * x * x + 90 * x - 200;
            player.setV(y);
        }

        // 函数3
        if (g_GameSta.getStaID() == STA_EX_GAME) {
            int x = player.sizeQ();
            int y = (-5 * x * x + 205 * x - 500) / 2;
            player.setV(y);
        }
        */
    }

    //获取失败者
    int getLoser(Context& ctx) {
        //头撞头判定
        if (ctx.stata.getStaID() == STA_AT_GAME || ctx.stata.getStaID() == STA_EX_GAME) {
            if (PlayerList[0].getNextPos() == PlayerList[1].getNextPos()) {
                //updataData(ctx, ctx.logic.getPlayer(0));
                //updataData(ctx, ctx.logic.getPlayer(1));
                PlayerList[0].setLive(false);
                PlayerList[1].setLive(false);
            }
        }

        int cut = 1;
        int res = 0;
        for (const Player& player : PlayerList) {// 遍历所有玩家进行绘制
            //printf("player%d,:%d ", cut, (player.getLive() ? 1 : 0));
            res += (player.getLive() ? 0 : cut);
            cut++;
        }
        //printf("\n");
        //printf("res=%d\n", res);
        return res;//1p1死 2p2死 3一起死
    }

    int Random() {
        return rand() % 10;
    }
};

// ========== 渲染层 ===========
class Renderer {
private:
    bool Death_[2] = { false };//用于结算模式计算哪条蛇闪烁
    int Frame_ = 0;//计算频闪帧率
    int RenderCnt_ = 9;//计算频闪次数

    COLORREF colors[8] = {
        //0路 1蛇 2果子 4蛇头 5原点
        RGB(255,255,255),  // 白0
        RGB(0, 255, 0),    // 绿1
        RGB(255, 255, 0),  // 黄2
        RGB(0, 243, 255),  // 蓝3
        RGB(255, 0, 0),    // 红4
        RGB(128, 0, 255), // 紫5
        RGB(240, 240, 240),//灰6
        RGB(255, 128, 0)  // 橙7
    };

public:
    void renderOver(Context& ctx) {
        //printf("Frame_/RenderCnt_: %d / %d\n", Frame_, RenderCnt_);

        BeginBatchDraw();   // 开始批量绘图（双缓冲）

        if (Frame_ >= SNAKE_UPDATE) {
            Frame_ -= SNAKE_UPDATE;

            setfillcolor(WHITE);
            solidrectangle(75, 75, 645, 650);

            // 画棋盘表格
            //setfillcolor(colors[6]);
            //solidrectangle(75, 75, 645, 645);   

            setlinecolor(colors[6]);             // 线条颜色
            setlinestyle(PS_SOLID, 6);     // 线型：实线，宽度3像素
            for (int i = 0; i < 20; i++) {//30,75
                line(80, 80 + 30 * i, 650, 80 + 30 * i);
                line(80 + 30 * i, 80, 80 + 30 * i, 650);
            }
            setlinecolor(BLUE);             // 线条颜色
            setlinestyle(PS_SOLID, 3);     // 线型：实线，宽度3像素
            for (int i = 0; i < 20; i++) {
                line(75, 75 + 30 * i, 645, 75 + 30 * i);
                line(75 + 30 * i, 75, 75 + 30 * i, 645);
            }

            //打印原点
            Print(0, 0, 5);

            //打印果子
            for (int i = 0; i < 3; i++) {
                const Position& pos = ctx.logic.getDotData(i);
                Print(pos.x, pos.y, 2);
            }

            //打印蛇
            const std::vector<Player>& players = ctx.logic.getPlayerList();
            int playerID = -1;
            for (const Player& player : players) {// 遍历所有玩家进行绘制
                playerID++;

                if (Death_[playerID] == true) {
                    const Position& npos = player.getNextPos();
                    printX(npos.x, npos.y);
                }

                if (Death_[playerID] == true && RenderCnt_ % 2 == 0) { //闪烁
                    printf("跳过P%d ", playerID + 1);;
                    continue;
                }

                printf("渲染P%d ", playerID + 1);
                std::queue<Position> temp = player.getQueue();
                int x = 0;
                int y = 0;
                for (int i = temp.size(); i > 0; i--) {
                    const Position& pos = temp.front();
                    x = pos.x;
                    y = pos.y;
                    Print(x, y, (i == 1 ? 4 : (playerID == ctx.logic.getMirror() ? 1 : 3)));
                    temp.pop();
                }

                if (Death_[playerID] == true) {
                    const Position& npos = player.getNextPos();
                    printX(npos.x, npos.y);
                }
            }
            printf("\n");

            //叉叉
            playerID = -1;
            for (const Player& player : players) {// 遍历所有玩家进行绘制
                playerID++;

                if (Death_[playerID] == true) {
                    const Position& npos = player.getNextPos();
                    printX(npos.x, npos.y);
                }
            }

            RenderCnt_--;
        }
        else {
            Frame_++;
        }

        EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
    }

    void initRenderOver(const bool(&Death)[2]) {
        Death_[0] = Death[0];
        Death_[1] = Death[1];
        std::cout << Death_[0] << "/" << Death_[1] << std::endl;
        Frame_ = SNAKE_UPDATE;
        RenderCnt_ = 9;
    }

    int getRenderCnt() {
        return RenderCnt_;
    }

    void renderGame(Context& ctx) {
        BeginBatchDraw();   // 开始批量绘图（双缓冲）

        cleardevice();

        // 画棋盘表格
        //setfillcolor(colors[6]);
        //solidrectangle(75, 75, 645, 645);   

        setlinecolor(colors[6]);             // 线条颜色
        setlinestyle(PS_SOLID, 6);     // 线型：实线，宽度3像素
        for (int i = 0; i < 20; i++) {//30,75
            line(80, 80 + 30 * i, 650, 80 + 30 * i);
            line(80 + 30 * i, 80, 80 + 30 * i, 650);
        }
        setlinecolor(BLUE);             // 线条颜色
        setlinestyle(PS_SOLID, 3);     // 线型：实线，宽度3像素
        for (int i = 0; i < 20; i++) {
            line(75, 75 + 30 * i, 645, 75 + 30 * i);
            line(75 + 30 * i, 75, 75 + 30 * i, 645);
        }

        //打印原点
        Print(0, 0, 5);

        //打印果子
        for (int i = 0; i < 3; i++) {
            const Position& pos = ctx.logic.getDotData(i);
            Print(pos.x, pos.y, 2);
        }

        //打印蛇
        const std::vector<Player>& players = ctx.logic.getPlayerList();
        int playerID = 0;
        for (const Player& player : players) {// 遍历所有玩家进行绘制
            std::queue<Position> temp = player.getQueue();
            int x = 0;
            int y = 0;
            for (int i = temp.size(); i > 0; i--) {
                const Position& pos = temp.front();
                x = pos.x;
                y = pos.y;
                Print(x, y, (i == 1 ? 4 : (playerID == ctx.logic.getMirror() ? 1 : 3)));//第563行
                temp.pop();
            }
            playerID++;
        }

        //显示title标语
        settextcolor(BLACK);
        outtextxy(75, 55, L"Hello, HungerSnake!");

        //打印游玩时长
        wchar_t buffer[256];
        swprintf(buffer, 100, L"游戏时间：%llds               ", ctx.gameTime.getSum() / 1000);
        outtextxy(800, 360, buffer);

        //打印速度
        if (ctx.stata.getStaID() == STA_PU_GAME) {
            swprintf(buffer, 256, L"当前速度：%d / %lld                                                                      ",
                1000 / ctx.logic.getPlayer(0).getV(), ctx.backTime.getSum());
            outtextxy(800, 340, buffer);
        }
        else if (ctx.stata.getStaID() == STA_AT_GAME) {
            swprintf(buffer, 256, L"当前速度：%d / %d / %lld / %lld                                                                      ",
                1000 / ctx.logic.getPlayer(0).getV(), 1000 / ctx.logic.getPlayer(1).getV(), ctx.backTime.getSum(), ctx.enterTime.getSum());
            outtextxy(800, 340, buffer);
        }
        else if (ctx.stata.getStaID() == STA_EX_GAME) {
            swprintf(buffer, 256, L"当前速度：%d / %d / %d / %d                                                                      ",
                1000 / ctx.logic.getPlayer(0).getV(), 1000 / ctx.logic.getPlayer(1).getV(), ctx.logic.getPlayer(0).getE(), ctx.logic.getPlayer(1).getE());
            outtextxy(800, 340, buffer);
        }


        //打印长度提示
        if (ctx.stata.getStaID() == STA_PU_GAME) {
            swprintf(buffer, 256, L"当前长度：%d                                          ", (int)ctx.logic.getPlayer(0).sizeQ());
            outtextxy(800, 380, buffer);
        }
        else if (ctx.stata.getStaID() == STA_AT_GAME || ctx.stata.getStaID() == STA_EX_GAME) {
            swprintf(buffer, 256, L"当前长度：%d / %d                                          ", (int)ctx.logic.getPlayer(0).sizeQ(), (int)ctx.logic.getPlayer(1).sizeQ());
            outtextxy(800, 380, buffer);
        }

        setbkmode(TRANSPARENT);

        // 定义颜色常量
        COLORREF normal = colors[6];              // 灰（未按下）
        COLORREF pressedGreen = colors[1];        // 亮绿（按下）
        COLORREF pressedBlue = colors[3];         // 亮蓝（按下）

        // ----- 玩家1（WASD + 空格） -----
        // 1. W 键（上）
        if (ctx.msg.getKeySta('W'))
            setfillcolor(pressedGreen);
        else
            setfillcolor(normal);
        solidrectangle(800, 575, 825, 600);   // 上

        // 2. A 键（左）
        if (ctx.msg.getKeySta('A'))
            setfillcolor(pressedGreen);
        else
            setfillcolor(normal);
        solidrectangle(765, 610, 790, 635);   // 左

        // 3. S 键（下）
        if (ctx.msg.getKeySta('S'))
            setfillcolor(pressedGreen);
        else
            setfillcolor(normal);
        solidrectangle(800, 610, 825, 635);   // 下

        // 4. D 键（右）
        if (ctx.msg.getKeySta('D'))
            setfillcolor(pressedGreen);
        else
            setfillcolor(normal);
        solidrectangle(835, 610, 860, 635);   // 右

        // 5. 空格键（中心方块）
        if (ctx.msg.getKeySta(VK_SPACE))
            setfillcolor(pressedGreen);
        else
            setfillcolor(normal);
        solidrectangle(775, 640, 850, 657);   // 中心

        // ----- 玩家2（方向键 + 回车） -----
        // 6. 上方向键
        if (ctx.msg.getKeySta(VK_UP))
            setfillcolor(pressedBlue);
        else
            setfillcolor(normal);
        solidrectangle(950, 575, 975, 600);   // 上

        // 7. 左方向键
        if (ctx.msg.getKeySta(VK_LEFT))
            setfillcolor(pressedBlue);
        else
            setfillcolor(normal);
        solidrectangle(915, 610, 940, 635);   // 左

        // 8. 下方向键
        if (ctx.msg.getKeySta(VK_DOWN))
            setfillcolor(pressedBlue);
        else
            setfillcolor(normal);
        solidrectangle(950, 610, 975, 635);   // 下

        // 9. 右方向键
        if (ctx.msg.getKeySta(VK_RIGHT))
            setfillcolor(pressedBlue);
        else
            setfillcolor(normal);
        solidrectangle(985, 610, 1010, 635);  // 右

        // 10. 回车键（中心方块）
        if (ctx.msg.getKeySta(VK_RETURN))
            setfillcolor(pressedBlue);
        else
            setfillcolor(normal);
        solidrectangle(925, 640, 1000, 657);  // 中心

        setbkmode(OPAQUE);//TRANSPARENT

        EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
    }

    void UpdateMenu(Context& ctx) {
        printf("menu更新，当前Sta=%d\n", ctx.stata.getStaID());
        BeginBatchDraw();   // 开始批量绘图（双缓冲）

        //画按钮和提示框
        if (ctx.stata.getStaID() == STA_LOBBY) {//大厅
            cleardevice();
            //画中按钮
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(490, 500, 590, 500);
            line(490, 550, 590, 550);
            line(490, 500, 490, 550);
            line(590, 500, 590, 550);
            setfillcolor(RGB(255, 220, 75));//0Xc0c15e//250, 252, 156
            solidrectangle(492, 502, 588, 548);
        }
        else if (ctx.stata.getStaID() == STA_SELECT) {
            // 画后表格
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(310, 240, 770, 240);//上
            line(310, 480, 770, 480);//下
            line(310, 240, 310, 480);//左
            line(770, 240, 770, 480);//右
            setfillcolor(RGB(250, 250, 156));//0Xc0c15e//250, 252, 156
            solidrectangle(312, 242, 768, 478);
            //画左按钮
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(360, 400, 460, 400);
            line(360, 450, 460, 450);
            line(360, 400, 360, 450);
            line(460, 400, 460, 450);
            setfillcolor(RGB(255, 39, 0));//0Xc0c15e//250, 252, 156//50, 200, 200
            solidrectangle(362, 402, 458, 448);
            //画中按钮
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(490, 400, 590, 400);
            line(490, 450, 590, 450);
            line(490, 400, 490, 450);
            line(590, 400, 590, 450);
            setfillcolor(RGB(255, 220, 75));//0Xc0c15e//250, 252, 156
            solidrectangle(492, 402, 588, 448);
            //画右按钮
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(620, 400, 720, 400);
            line(620, 450, 720, 450);
            line(620, 400, 620, 450);
            line(720, 400, 720, 450);
            setfillcolor(RGB(124, 254, 86));//0Xc0c15e//250, 252, 156
            solidrectangle(622, 402, 718, 448);
        }
        else if (ctx.stata.getStaID() == STA_PAUSE) {
            // 画后表格
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(310, 240, 770, 240);//上
            line(310, 480, 770, 480);//下
            line(310, 240, 310, 480);//左
            line(770, 240, 770, 480);//右
            setfillcolor(RGB(250, 250, 156));//0Xc0c15e//250, 252, 156
            solidrectangle(312, 242, 768, 478);
            //画左按钮
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(360, 400, 460, 400);
            line(360, 450, 460, 450);
            line(360, 400, 360, 450);
            line(460, 400, 460, 450);
            setfillcolor(RGB(50, 200, 200));//0Xc0c15e//250, 252, 156//50, 200, 200
            solidrectangle(362, 402, 458, 448);
            //画中按钮
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(490, 400, 590, 400);
            line(490, 450, 590, 450);
            line(490, 400, 490, 450);
            line(590, 400, 590, 450);
            setfillcolor(RGB(255, 220, 75));//0Xc0c15e//250, 252, 156
            solidrectangle(492, 402, 588, 448);
            //画右按钮
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(620, 400, 720, 400);
            line(620, 450, 720, 450);
            line(620, 400, 620, 450);
            line(720, 400, 720, 450);
            setfillcolor(RGB(124, 254, 86));//0Xc0c15e//250, 252, 156
            solidrectangle(622, 402, 718, 448);
        }
        else if (ctx.stata.getStaID() == STA_OVER || ctx.stata.getStaID() == STA_ACCOUNT) {
            // 画后表格
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(310, 240, 770, 240);//上
            line(310, 480, 770, 480);//下
            line(310, 240, 310, 480);//左
            line(770, 240, 770, 480);//右
            setfillcolor(RGB(250, 250, 156));//0Xc0c15e//250, 252, 156
            solidrectangle(312, 242, 768, 478);
            //画左按钮
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(360, 400, 460, 400);
            line(360, 450, 460, 450);
            line(360, 400, 360, 450);
            line(460, 400, 460, 450);
            setfillcolor(RGB(50, 200, 200));//0Xc0c15e//250, 252, 156//50, 200, 200
            solidrectangle(362, 402, 458, 448);
            //画右按钮
            setlinecolor(RGB(0, 0, 200));
            setlinestyle(PS_SOLID, 3);
            line(620, 400, 720, 400);
            line(620, 450, 720, 450);
            line(620, 400, 620, 450);
            line(720, 400, 720, 450);
            setfillcolor(RGB(255, 220, 75));//0Xc0c15e//250, 252, 156
            solidrectangle(622, 402, 718, 448);
        }

        //文字
        setbkmode(TRANSPARENT);
        if (ctx.stata.getStaID() == STA_LOBBY) {
            settextcolor(BLACK);
            outtextxy(510, 518, L"开始游戏");
        }
        if (ctx.stata.getStaID() == STA_SELECT) {
            //画提示框
            settextcolor(BLACK);
            outtextxy(395, 418, L"极限");
            outtextxy(525, 418, L"竞速");
            outtextxy(655, 418, L"普通");
            outtextxy(510, 310, L"选择模式");
            //清空，填充背景色
            setfillcolor(colors[0]);
            solidrectangle(488, 498, 592, 552);
        }
        else if (ctx.stata.getStaID() == STA_PAUSE) {
            settextcolor(BLACK);
            outtextxy(380, 418, L"返回开始");
            outtextxy(510, 418, L"重新开始");
            outtextxy(640, 418, L"继续游戏");
            outtextxy(527, 310, L"暂停");
        }
        else if (ctx.stata.getStaID() == STA_OVER) {
            settextcolor(BLACK);
            outtextxy(380, 418, L"返回开始");
            outtextxy(640, 418, L"重新开始");
            outtextxy(500, 310, L"GAME OVER !!");
        }
        else if (ctx.stata.getStaID() == STA_ACCOUNT) {
            settextcolor(BLACK);
            outtextxy(380, 418, L"返回开始");
            outtextxy(640, 418, L"重新开始");

            wchar_t buffer[100];
            int item = ctx.logic.getLoser(ctx);
            if (item == 3) {
                swprintf(buffer, 100, L"真是一对苦命鸳鸯");
                outtextxy(480, 310, buffer);//??
            }
            else {
                swprintf(buffer, 100, L"Player%d VICTORY !!!", (item == 1 ? 2 : 1));
                outtextxy(490, 310, buffer);//??
            }

        }
        setbkmode(OPAQUE);//TRANSPARENT

        EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
    }

    void Print(int x, int y, int color) {
        setfillcolor(colors[color]);
        solidrectangle(X_CODE_PIONE + x * 30, X_CODE_PIONE - y * 30, Y_CODE_PIONE + x * 30, Y_CODE_PIONE - y * 30);
    }

    void printX(int x, int y) {
        setlinecolor(RED);
        line(X_CODE_PIONE + x * 30, X_CODE_PIONE - y * 30, Y_CODE_PIONE + x * 30, Y_CODE_PIONE - y * 30);
        line(Y_CODE_PIONE + x * 30, X_CODE_PIONE - y * 30, X_CODE_PIONE + x * 30, Y_CODE_PIONE - y * 30);
    }
};


// ================== 主函数 =====================
int main()
{
    timeBeginPeriod(1);  // // 提高定时器精度,全局设置，程序结束时记得 timeEndPeriod(1)

    Time gameTime, backTime, enterTime, limitFPS;//第四个限制帧率
    Stata stata;
    Logic logic;
    Msg input;
    Renderer renderer;

    Context ctx{ stata, logic, renderer, input, gameTime, backTime, enterTime };


    //生成画布
    initgraph(1080, 720);
    setbkcolor(WHITE);             //设置背景（白）
    cleardevice();
    //初始化必备数据
    setbkmode(OPAQUE);//TRANSPARENT
    srand((unsigned int)time(NULL));//加载随机数
    ctx.stata.push(InitLobby, STA_LOBBY);//初始化栈


    //加载帧率计数器
    const double FRAME_TIME_MS = 1000.0 / FPS_LOGIC;  // 16.666... ms
    using namespace std::chrono;
    auto nextFrameTime = steady_clock::now();//第0帧时间节点


    //程序主循环
    while (TRUE) {
        if (ctx.stata.empty())break;//退出逻辑

        ctx.msg.GetMsg();//监听键盘
        ctx.stata.update(ctx);//运行栈顶


        //等待
        nextFrameTime += milliseconds((long long)FRAME_TIME_MS);
        std::this_thread::sleep_until(nextFrameTime);

        // 防追帧
        if (steady_clock::now() > nextFrameTime) {
            // 重置时间基准，防止疯狂追帧
            nextFrameTime = steady_clock::now();
        }
    }

    timeEndPeriod(1);
    closegraph();
    return 0;
}

void InitLobby(Context& ctx) {
    //绘画大厅按键
    ctx.renderer.UpdateMenu(ctx);
    ctx.stata.pop();
    ctx.stata.push([](Context& c) {Lobby(c); }, STA_LOBBY);
}

void Lobby(Context& ctx) {
    //大厅检测
    if (ctx.msg.getMouse()) {
        POINT pt = ctx.msg.getMousePos();
        if (pt.x >= Sarea.left && pt.x <= Sarea.right && pt.y >= Sarea.top && pt.y <= Sarea.bottom) {
            //开始
            ctx.stata.push(InitSelect, STA_SELECT);
            printf("按下开始按钮\n");
        }
    }

    //ESC
    if (ctx.msg.getKeyDown(VK_ESCAPE)) {
        printf("键盘按下，Sta=%d\n", ctx.stata.getStaID());
        ctx.stata.pop();
        ctx.renderer.UpdateMenu(ctx);
    }
}

void InitSelect(Context& ctx) {
    //绘画难度选择按键
    ctx.renderer.UpdateMenu(ctx);
    ctx.stata.pop();
    ctx.stata.push(Select, STA_SELECT);
}

void Select(Context& ctx) {
    //选择难度检测
    if (ctx.msg.getMouse()) {
        POINT pt = ctx.msg.getMousePos();
        if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
            //左
            //极限模式
            ctx.stata.push(InitEXGameData, STA_EX_GAME);
            printf("左\n");
        }
        else if (pt.x >= Marea.left && pt.x <= Marea.right && pt.y >= Marea.top && pt.y <= Marea.bottom) {
            //中
            //竞技模式
            ctx.stata.push(InitATGameData, STA_AT_GAME);
            printf("中\n");
        }
        else if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
            //右
            //简单模式
            ctx.stata.push(InitPuGameData, STA_PU_GAME);
            printf("右\n");
        }
    }

    //ESC
    if (ctx.msg.getKeyDown(VK_ESCAPE)) {
        printf("键盘按下，Sta=%d\n", ctx.stata.getStaID());
        ctx.stata.pop();
        ctx.renderer.UpdateMenu(ctx);
    }
}

void InitPause(Context& ctx) {
    //绘画暂停按键
    ctx.renderer.UpdateMenu(ctx);
    ctx.stata.pop();
    ctx.stata.push(Pause, STA_PAUSE);
}

void Pause(Context& ctx) {
    //暂停界面
    if (ctx.msg.getMouse()) {
        POINT pt = ctx.msg.getMousePos();
        if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
            //左
            //返回大厅
            ctx.logic.clearPlayerList();
            for (int i = 0; i < 3; i++) {
                ctx.stata.pop();
            }
            ctx.renderer.UpdateMenu(ctx);
            printf("左\n");
        }
        if (pt.x >= Marea.left && pt.x <= Marea.right && pt.y >= Marea.top && pt.y <= Marea.bottom) {
            //中
            //重开（我应该使用StaID？）
            ctx.stata.pop();
            ctx.logic.clearPlayerList();
            if (ctx.stata.getStaID() == STA_PU_GAME) {
                ctx.stata.pop();
                ctx.stata.push(InitPuGameData, STA_PU_GAME);
            }
            else if (ctx.stata.getStaID() == STA_AT_GAME) {
                ctx.stata.pop();
                ctx.stata.push(InitATGameData, STA_AT_GAME);
            }
            else if (ctx.stata.getStaID() == STA_EX_GAME) {
                ctx.stata.pop();
                ctx.stata.push(InitEXGameData, STA_EX_GAME);
            }
            printf("中\n");
        }
        if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
            //右
            //继续游戏
            ctx.stata.pop();
            ctx.gameTime.resume();
            ctx.renderer.renderGame(ctx);
            printf("右\n");
        }
    }

    //ESC
    if (ctx.msg.getKeyDown(VK_ESCAPE)) {
        printf("键盘按下，Sta=%d\n", ctx.stata.getStaID());
        ctx.stata.pop();
        ctx.gameTime.resume();
        ctx.renderer.renderGame(ctx);
    }
}

void InitOver(Context& ctx) {
    //绘画暂停按键
    ctx.renderer.UpdateMenu(ctx);
    ctx.gameTime.pause();
    ctx.backTime.clear();
    ctx.enterTime.clear();

    ctx.stata.pop();
    ctx.stata.push(Over, STA_OVER);
}

void Over(Context& ctx) {
    //游戏结束界面
    if (ctx.msg.getMouse()) {
        POINT pt = ctx.msg.getMousePos();
        if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
            //左
            //返回大厅
            ctx.logic.clearPlayerList();
            returnToLobby(ctx);
            ctx.renderer.UpdateMenu(ctx);
            printf("左\n");
        }
        else if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
            //右
            //重开（我应该使用StaID？）
            ctx.stata.pop();
            ctx.logic.clearPlayerList();
            if (ctx.stata.getStaID() == STA_PU_GAME) {
                ctx.stata.pop();
                ctx.stata.push(InitPuGameData, STA_PU_GAME);
            }
            else if (ctx.stata.getStaID() == STA_AT_GAME) {
                ctx.stata.pop();
                ctx.stata.push(InitATGameData, STA_AT_GAME);
            }
            else if (ctx.stata.getStaID() == STA_EX_GAME) {
                ctx.stata.pop();
                ctx.stata.push(InitEXGameData, STA_EX_GAME);
            }
            printf("右\n");
        }
    }
}

void InitAccount(Context& ctx) {
    //绘画结算画面
    ctx.renderer.UpdateMenu(ctx);
    ctx.gameTime.pause();
    ctx.backTime.clear();
    ctx.enterTime.clear();

    ctx.stata.pop();
    ctx.stata.push(Account, STA_ACCOUNT);
}

void Account(Context& ctx) {
    //结算界面
    if (ctx.msg.getMouse()) {
        POINT pt = ctx.msg.getMousePos();
        if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
            //左
            //返回大厅
            ctx.logic.clearPlayerList();
            returnToLobby(ctx);
            ctx.renderer.UpdateMenu(ctx);
            printf("左\n");
        }
        else if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
            //右
            //重开（我应该使用StaID？）
            ctx.stata.pop();
            ctx.logic.clearPlayerList();
            if (ctx.stata.getStaID() == STA_PU_GAME) {
                ctx.stata.pop();
                ctx.stata.push(InitPuGameData, STA_PU_GAME);
            }
            else if (ctx.stata.getStaID() == STA_AT_GAME) {
                ctx.stata.pop();
                ctx.stata.push(InitATGameData, STA_AT_GAME);
            }
            else if (ctx.stata.getStaID() == STA_EX_GAME) {
                ctx.stata.pop();
                ctx.stata.push(InitEXGameData, STA_EX_GAME);
            }
            printf("右\n");
        }
    }
}

void InitDeathAnim(Context& ctx) {
    bool Death[2] = { false };
    const std::vector<Player>& players = ctx.logic.getPlayerList();
    int playerID = 0;
    for (const Player& player : players) {// 遍历所有玩家进行绘制
        Death[playerID] = !player.getLive();
        playerID++;
    }
    ctx.renderer.initRenderOver(Death);
    ctx.stata.pop();
    ctx.stata.push(DeathAnim, STA_DEATH);
}

void DeathAnim(Context& ctx) {
    if (!ctx.msg.getKeyDown(VK_ESCAPE) && ctx.renderer.getRenderCnt() > 0) {
        ctx.renderer.renderOver(ctx);
    }
    else {
        ctx.stata.pop();
        if (ctx.stata.getStaID() == STA_PU_GAME || ctx.stata.getStaID() == STA_EX_GAME) {
            ctx.stata.push(InitOver, STA_OVER);
        }
        else if (ctx.stata.getStaID() == STA_AT_GAME) {
            ctx.stata.push(InitAccount, STA_ACCOUNT);
        }
    }
}

void InitPuGameData(Context& ctx) {
    clearPastData(ctx);//清空上一把残留数据

    ctx.logic.addPlayer(0, 0, 0);//队列初始化
    ctx.logic.initMapData(STA_PU_GAME);//地图初始化和果子初始化
    ctx.renderer.renderGame(ctx);//渲染一帧
    ctx.gameTime.start();//开始计时

    ctx.stata.pop();
    ctx.stata.push(PuGame, STA_PU_GAME);
}

void PuGame(Context& ctx) {
    ctx.gameTime.updata(); //游玩时间更新
    if (!ctx.backTime.empty())ctx.backTime.updata(); //空格长按时间更新

    //键盘监听
    if (ctx.msg.getKeyDown(VK_ESCAPE)) {//ESC
        printf("键盘按下，Sta=%d\n", ctx.stata.getStaID());
        ctx.stata.push(InitPause, STA_PAUSE);
        ctx.backTime.clear();
        ctx.gameTime.pause();
    }
    if (ctx.msg.getKeyDown(VK_SPACE)) {//空格
        if (ctx.backTime.getSum() == 0) {
            ctx.backTime.start();
        }
    }
    if (ctx.msg.getKeyUp(VK_SPACE)) {//空格
        ctx.backTime.clear(); // 清零
    }
    MoveMsg(ctx, 0, 0);

    //长按空格
    ctx.logic.getPlayer(0).setV(ctx.backTime.getSum());

    //刷新
    ctx.logic.getPlayer(0).FrameAdd();

    bool p1 = (ctx.logic.getPlayer(0).getV() <= ctx.logic.getPlayer(0).getFrame());

    if (p1) {
        //清空帧数计数
        ctx.logic.getPlayer(0).clearFrame();
        ctx.logic.judgeMoveRequest(ctx, ctx.logic.getPlayer(0));
    }

    //渲染
    if (ctx.logic.getLoser(ctx) == 0) {
        if (p1)ctx.logic.updataData(ctx, ctx.logic.getPlayer(0));
        ctx.renderer.renderGame(ctx);
    }
    else {
        ctx.stata.push(InitDeathAnim, STA_DEATH);
    }
}

void InitATGameData(Context& ctx) {
    //清空上一把残留数据
    clearPastData(ctx);

    ctx.logic.addPlayer(-5, 0, 0);//队列初始化
    ctx.logic.addPlayer(5, 0, 0);
    ctx.logic.initMapData(STA_AT_GAME);//地图初始化和果子初始化
    ctx.renderer.renderGame(ctx);//渲染一帧
    ctx.gameTime.start();//开始计时

    ctx.stata.pop();
    ctx.stata.push(ATGame, STA_AT_GAME);
}

void ATGame(Context& ctx) {
    ctx.gameTime.updata(); //游玩时间更新
    if (!ctx.backTime.empty())ctx.backTime.updata(); //空格长按时间更新
    if (!ctx.enterTime.empty())ctx.enterTime.updata(); //回车长按时间更新

    //键盘监听
    if (ctx.msg.getKeyDown(VK_ESCAPE)) {//ESC
        printf("键盘按下，Sta=%d\n", ctx.stata.getStaID());
        ctx.stata.push(InitPause, STA_PAUSE);
        ctx.backTime.clear();
        ctx.enterTime.clear();
        ctx.gameTime.pause();
    }
    if (ctx.msg.getKeyDown(VK_SPACE)) {//空格
        if (ctx.backTime.getSum() == 0) {
            ctx.backTime.start();
        }
    }
    if (ctx.msg.getKeyUp(VK_SPACE)) {//空格
        ctx.backTime.clear(); // 清零
        ctx.logic.getPlayer(0).setFrame(ctx.logic.getPlayer(1).getFrame());
    }
    if (ctx.msg.getKeyDown(VK_RETURN)) {//回车
        if (ctx.enterTime.getSum() == 0) {
            ctx.enterTime.start();
        }
    }
    if (ctx.msg.getKeyUp(VK_RETURN)) {//回车
        ctx.enterTime.clear(); // 清零
        ctx.logic.getPlayer(1).setFrame(ctx.logic.getPlayer(0).getFrame());
    }
    MoveMsg(ctx, 0, 1);


    ctx.logic.getPlayer(0).setV(ctx.backTime.getSum());//长按空格
    ctx.logic.getPlayer(1).setV(ctx.enterTime.getSum());//长按回车

    //刷新
    ctx.logic.getPlayer(0).FrameAdd();
    ctx.logic.getPlayer(1).FrameAdd();

    //计算移动请求
    std::vector<int> toMove; // 每帧新建，自动析构
    if (ctx.logic.getPlayer(0).getV() <= ctx.logic.getPlayer(0).getFrame()) toMove.push_back(0);
    if (ctx.logic.getPlayer(1).getV() <= ctx.logic.getPlayer(1).getFrame()) toMove.push_back(1);

    for (int i : toMove) {
        //清空帧数计数
        ctx.logic.getPlayer(i).clearFrame();
        ctx.logic.judgeMoveRequest(ctx, ctx.logic.getPlayer(i));
    }

    //是否失败跳出
    if (ctx.logic.getLoser(ctx) != 0) {
        ctx.stata.push(InitDeathAnim, STA_DEATH);
        return;
    }

    //更新数据
    for (int i : toMove) {
        //清空帧数计数
        ctx.logic.updataData(ctx, ctx.logic.getPlayer(i));
    }

    ctx.renderer.renderGame(ctx);
}

void InitEXGameData(Context& ctx) {
    //清空上一把残留数据
    clearPastData(ctx);

    ctx.logic.addPlayer(5, -5, 0);//队列初始化
    ctx.logic.addPlayer(-5, 5, 2);
    ctx.logic.initMapData(STA_EX_GAME);//地图初始化和果子初始化
    ctx.renderer.renderGame(ctx);//渲染一帧
    ctx.gameTime.start();//开始计时

    ctx.stata.pop();
    ctx.stata.push(EXGame, STA_EX_GAME);
}

void EXGame(Context& ctx) {
    ctx.gameTime.updata(); //游玩时间更新

    //键盘监听
    if (ctx.msg.getKeyDown(VK_ESCAPE)) {//ESC
        printf("键盘按下，Sta=%d\n", ctx.stata.getStaID());
        ctx.stata.push(InitPause, STA_PAUSE);
        ctx.gameTime.pause();
    }
    if (ctx.msg.getKeyUp(VK_SPACE)) {//空格
        ctx.logic.Mirror();
    }
    bool mirror = ctx.logic.getMirror(); // 注意：原代码中用 mirror 变量，但 Logic 中 mirror 是私有，需要提供 getMirror() 已存在
    int mainIdx = mirror ? 1 : 0;
    int subIdx = 1 - mainIdx;
    MirrorMoveMsg(ctx, mainIdx, subIdx);

    //刷新
    ctx.logic.getPlayer(0).FrameAdd();
    ctx.logic.getPlayer(1).FrameAdd();

    //计算移动请求
    std::vector<int> toMove; // 每帧新建，自动析构
    if (ctx.logic.getPlayer(0).getV() <= ctx.logic.getPlayer(0).getFrame()) toMove.push_back(0);
    if (ctx.logic.getPlayer(1).getV() <= ctx.logic.getPlayer(1).getFrame()) toMove.push_back(1);

    for (int i : toMove) {
        //清空帧数计数
        ctx.logic.getPlayer(i).clearFrame();
        ctx.logic.judgeMoveRequest(ctx, ctx.logic.getPlayer(i));
    }

    //是否失败跳出
    if (ctx.logic.getLoser(ctx) != 0) {
        ctx.stata.push(InitDeathAnim, STA_DEATH);
        return;
    }

    //更新数据
    for (int i : toMove) {
        //清空帧数计数
        ctx.logic.updataData(ctx, ctx.logic.getPlayer(i));
    }

    ctx.renderer.renderGame(ctx);
}

//====== 辅助函数 =====
void returnToLobby(Context& ctx) {
    while (ctx.stata.getStaDepth() > 1) {
        ctx.stata.pop();
    }
}

void clearPastData(Context& ctx) {
    ctx.gameTime.clear();
    ctx.logic.initDotData();
    ctx.logic.clearMapData();
    ctx.logic.clearPlayerList();
    ctx.logic.initMirror();
}

void MoveMsg(Context& ctx, int p1, int p2) {
    //p1
    if (ctx.msg.getKeyDown('W')) ctx.logic.getPlayer(p1).up();
    if (ctx.msg.getKeyDown('D')) ctx.logic.getPlayer(p1).right();
    if (ctx.msg.getKeyDown('S')) ctx.logic.getPlayer(p1).down();
    if (ctx.msg.getKeyDown('A')) ctx.logic.getPlayer(p1).left();

    //p2
    if (ctx.msg.getKeyDown(VK_UP))    ctx.logic.getPlayer(p2).up();
    if (ctx.msg.getKeyDown(VK_RIGHT)) ctx.logic.getPlayer(p2).right();
    if (ctx.msg.getKeyDown(VK_DOWN))  ctx.logic.getPlayer(p2).down();
    if (ctx.msg.getKeyDown(VK_LEFT))  ctx.logic.getPlayer(p2).left();
}

void MirrorMoveMsg(Context& ctx, int p1, int p2) {
    if (ctx.msg.getKeyDown('W') || ctx.msg.getKeyDown(VK_UP)) { ctx.logic.getPlayer(p1).up();     ctx.logic.getPlayer(p2).down(); }
    if (ctx.msg.getKeyDown('D') || ctx.msg.getKeyDown(VK_RIGHT)) { ctx.logic.getPlayer(p1).right();  ctx.logic.getPlayer(p2).left(); }
    if (ctx.msg.getKeyDown('S') || ctx.msg.getKeyDown(VK_DOWN)) { ctx.logic.getPlayer(p1).down();   ctx.logic.getPlayer(p2).up(); }
    if (ctx.msg.getKeyDown('A') || ctx.msg.getKeyDown(VK_LEFT)) { ctx.logic.getPlayer(p1).left();   ctx.logic.getPlayer(p2).right(); }
}

