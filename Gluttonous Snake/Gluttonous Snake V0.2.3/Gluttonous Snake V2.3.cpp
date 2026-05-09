/*
26.4.13V2.3版贪吃蛇

采用状态栈架构，封装在Stata类里并且需要手动把状态ID push到类里（方便其他函数比较当前状态）
并且提供一个获取栈深度的接口在某些场合化简比较多个栈状态（便于后续扩展游戏模式）
同时，关于游戏时间，检测空格长按和限制刷新率也封装在了类里面

相比2.2，把固定数量的全局玩家对象改成动态玩家列表形式，对于双死判定做出补丁，整体变得更加
屎山代码
*/


#define SNAKE_UPDATE 20 //初始蛇蛇刷新
#define FPS_LOGIC 60.0  //逻辑刷新率
#define LOW_DOWN_TIME 200 //最短按下时间
#define X_CODE_PIONE  347   //(0,0)左上坐标
#define Y_CODE_PIONE  373   //(0,0)右下坐标

// ========== 状态ID表 ==========
#define STA_END 0
#define STA_LOBBY 1
#define STA_SELECT 2
#define STA_PAUSE 3
#define STA_OVER 4
#define STA_ACCOUNT 5//竞技模式结算画面
#define STA_PU_GAME 6 //游戏模式放最后方便扩展方便
#define STA_AT_GAME 7



#include <queue>//为了队列
#include <stack>//为了栈
#include <iostream>//数据流
#include <vector>//动态数组
#include <functional>
//#include <map>
#include <typeindex>
#include <easyx.h>
#include <windows.h>  // 为了 Sleep
#include <chrono>
#include <thread>
#include <cstdlib>  // rand(), srand()

#pragma comment(lib, "winmm.lib")   // 添加这一行


void InitLobby();
void Lobby();

void InitSelect();
void Select();

void InitPause();
void Pause();

void InitOver();
void Over();

void InitAccount();
void Account();

void InitPuGameData();
void PuGame();

void InitATGameData();
void ATGame();


void GetMsg();

int Random();

struct Position {
    int x = 0;
    int y = 0;

    // 成员函数形式重载 ==
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

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

Time g_GameTime;//记录游戏时间
Time g_BackTime;//空格按下时间
Time g_EnterTime;//回车按下时间
Time g_LimitFPS;//限制帧率

// ========== 状态栈管理器 ==========
class Stata {
private:
    std::stack<std::function<void()>>stack;//状态栈
    std::stack<int>StaID;//状态ID

public:
    // 压栈
    void push(std::function<void()> func, int id) {
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
    void update() {
        if (!stack.empty()) {
            stack.top()();
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

Stata g_GameSta;//游戏状态对象

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

    // 蛇坐标队列
    std::queue<Position> posQueue;

    //存活状态
    bool Live_ = { true };

public:
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
};

// ========== 逻辑层===========
class Logic {
private:
    //地图用于判断是否是蛇身，判断果子用数组
    int MapData[19][19] = { 0 };//0路 1蛇 2果子 4蛇头 5原点
    struct Position DotData[3] = { 0 };//果子坐标

    std::vector<Player>PlayerList;//玩家列表

public:
    //玩家列表相关
    void addPlayer(int x, int y) {
        Player p;
        p.pushQ(x, y - 2);
        p.pushQ(x, y - 1);
        p.pushQ(x, y);
        p.initNextPos(x, y + 1);
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
        while (MapData[x + 9][-y + 9] != 0 && (x != nx && y != ny)) {
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
    void updata(Player& player) {
        //上右下左
        const int dx[4] = { 0,1,0,-1 };
        const int dy[4] = { 1,0,-1,0 };

        //获取当前蛇头坐标
        const Position& pos = player.getBack();
        int x = pos.x;
        int y = pos.y;

        //获取下一格坐标
        int dir = player.getDir();
        int nx = x + dx[dir];
        int ny = y + dy[dir];

        //判断下一格
        int ItemMapData = MapData[nx + 9][-ny + 9];
        if (nx < -9 || ny < -9 || nx>9 || ny>9 || ItemMapData == 1 || ItemMapData == 4) {
            //gameover
            player.setLive(false);
            player.setNextPos(nx, ny);
            return;
        }

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
        MapData[x + 9][-y + 9] = 1;
        MapData[nx + 9][-ny + 9] = 4;
        player.pushQ(nx, ny);
        player.setNextPos(nx, ny);
        player.updataPurrDir();
    }

    void BothOut() {
        if (PlayerList[0].getNextPos() == PlayerList[1].getNextPos()) {
            for (Player& player : PlayerList) {// 遍历所有玩家进行绘制
                player.setLive(false);
            }
        }
    }

    int getLoser() {
        if (g_GameSta.getStaID() == STA_AT_GAME) BothOut();
        int cut = 1;
        int res = 0;
        for (const Player& player : PlayerList) {// 遍历所有玩家进行绘制
            res += (player.getLive() ? 0 : cut);
            cut++;
        }
        //printf("res=%d\n", res);
        return res;//1p1死 2p2死 3一起死
    }
};

Logic g_GameLogic;//逻辑层

// ========== 渲染层 ===========
class Rander {
private:
    int DownMode = 4;

    COLORREF colors[6] = {
        //0路 1蛇 2果子 4蛇头 5原点
        RGB(255,255,255),  // 白0
        RGB(0, 255, 0),    // 绿1
        RGB(255, 255, 0),  // 黄2
        RGB(255, 128, 0),  // 橙3
        RGB(255, 0, 0),    // 红4
        RGB(128, 0, 255)  // 紫5
    };

public:
    int getDownMode()const {
        return DownMode;
    }

    void setDownMode(int i) {
        DownMode = i;
    }

    void renderGame(const Logic& logic) {
        BeginBatchDraw();   // 开始批量绘图（双缓冲）

        cleardevice();

        // 画棋盘表格
        setlinecolor(BLUE);             // 线条颜色
        setlinestyle(PS_SOLID, 3);     // 线型：实线，宽度3像素
        for (int i = 0; i < 20; i++) {//30,75
            line(75, 75 + 30 * i, 645, 75 + 30 * i);
            line(75 + 30 * i, 75, 75 + 30 * i, 645);
        }

        //打印原点
        Print(0, 0, 5);

        //打印果子
        for (int i = 0; i < 3; i++) {
            const Position& pos = logic.getDotData(i);
            Print(pos.x, pos.y, 2);
        }

        //打印蛇
        const std::vector<Player>& players = logic.getPlayerList();
        for (const Player& player : players) {// 遍历所有玩家进行绘制
            std::queue<Position> temp = player.getQueue();
            int x = 0;
            int y = 0;
            for (int i = temp.size(); i > 0; i--) {
                const Position& pos = temp.front();
                x = pos.x;
                y = pos.y;
                Print(x, y, (i == 1 ? 4 : 1));
                temp.pop();
            }
        }

        //显示title标语
        settextcolor(BLACK);
        outtextxy(75, 55, L"Hello, HungerSnake!");

        //打印游玩时长
        wchar_t buffer[100];
        swprintf(buffer, 100, L"游戏时间：%llds               ", g_GameTime.getSum() / 1000);
        outtextxy(800, 360, buffer);

        //打印速度
        if (g_GameSta.getStaID() == STA_PU_GAME && g_GameSta.getStaDepth() >= 3) {
            swprintf(buffer, 100, L"当前速度：%d / %lld                                                                      ", 1000 / g_GameLogic.getPlayer(0).getV(), g_BackTime.getSum());
            outtextxy(800, 340, buffer);
        }
        else if (g_GameSta.getStaID() == STA_AT_GAME && g_GameSta.getStaDepth() >= 3) {
            swprintf(buffer, 100, L"当前速度：%d / %d / %lld / %lld                                                                      ", 1000 / g_GameLogic.getPlayer(0).getV(), 1000 / g_GameLogic.getPlayer(1).getV(), g_BackTime.getSum(), g_EnterTime.getSum());
            outtextxy(800, 340, buffer);
        }


        //打印长度提示
        if (g_GameSta.getStaID() == STA_PU_GAME) {
            swprintf(buffer, 100, L"当前长度：%d                                          ", (int)g_GameLogic.getPlayer(0).sizeQ());
            outtextxy(800, 380, buffer);
        }
        else if (g_GameSta.getStaID() == STA_AT_GAME) {
            swprintf(buffer, 100, L"当前长度：%d / %d                                          ", (int)g_GameLogic.getPlayer(0).sizeQ(), (int)g_GameLogic.getPlayer(1).sizeQ());
            outtextxy(800, 380, buffer);
        }


        //打印操作提示
        if (DownMode == 0) {
            swprintf(buffer, 100, L"当前按下：↑              ");
        }
        else if (DownMode == 1) {
            swprintf(buffer, 100, L"当前按下：→              ");
        }
        else if (DownMode == 2) {
            swprintf(buffer, 100, L"当前按下：↓              ");
        }
        else if (DownMode == 3) {
            swprintf(buffer, 100, L"当前按下：←              ");
        }
        else if (DownMode == -1) {
            swprintf(buffer, 100, L"当前按下：ESC             ");
        }
        else if (DownMode == -2) {
            swprintf(buffer, 100, L"当前按下：BaclSpace");
        }
        else {
            swprintf(buffer, 100, L"当前按下：None             ");
        }
        outtextxy(800, 400, buffer);

        EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
    }

    void UpdateMenu() {
        printf("menu更新，当前Sta=%d\n", g_GameSta.getStaID());
        BeginBatchDraw();   // 开始批量绘图（双缓冲）

        //画按钮和提示框
        if (g_GameSta.getStaID() == STA_LOBBY) {//大厅
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
        else if (g_GameSta.getStaID() == STA_SELECT) {
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
        else if (g_GameSta.getStaID() == STA_PAUSE) {
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
        else if (g_GameSta.getStaID() == STA_OVER || g_GameSta.getStaID() == STA_ACCOUNT) {
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
        if (g_GameSta.getStaID() == STA_LOBBY) {
            settextcolor(BLACK);
            outtextxy(510, 518, L"开始游戏");
        }
        if (g_GameSta.getStaID() == STA_SELECT) {
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
        else if (g_GameSta.getStaID() == STA_PAUSE) {
            settextcolor(BLACK);
            outtextxy(380, 418, L"返回开始");
            outtextxy(510, 418, L"重新开始");
            outtextxy(640, 418, L"继续游戏");
            outtextxy(527, 310, L"暂停");
        }
        else if (g_GameSta.getStaID() == STA_OVER) {
            settextcolor(BLACK);
            outtextxy(380, 418, L"返回开始");
            outtextxy(640, 418, L"重新开始");
            outtextxy(500, 310, L"GAME OVER !!");
        }
        else if (g_GameSta.getStaID() == STA_ACCOUNT) {
            settextcolor(BLACK);
            outtextxy(380, 418, L"返回开始");
            outtextxy(640, 418, L"重新开始");

            wchar_t buffer[100];
            if (g_GameLogic.getLoser() == 3) {
                swprintf(buffer, 100, L"真是一对苦命鸳鸯");
                outtextxy(480, 310, buffer);//??
            }
            else {
                swprintf(buffer, 100, L"Player%d VICTORY !!!", (g_GameLogic.getLoser() == 1 ? 2 : 1));
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

Rander g_GameRander;//渲染层

ExMessage msg;
RECT Larea = { 362, 402, 458, 448 };//左按钮
RECT Marea = { 492, 402, 588, 448 };//中按钮
RECT Rarea = { 622, 402, 718, 448 };//右按钮
RECT Sarea = { 492, 502, 588, 548 };//开始按钮


// ================== 主函数 =====================
int main()
{
    //生成画布
    initgraph(1080, 720);
    setbkcolor(WHITE);             //设置背景（白）
    cleardevice();
    setbkmode(OPAQUE);//TRANSPARENT

    //加载随机数
    srand((unsigned int)time(NULL));

    //初始化栈
    g_GameSta.push(InitLobby, STA_LOBBY);

    //加载帧率计数器
    const double FRAME_TIME_MS = 1000.0 / FPS_LOGIC;  // 16.666... ms
    using namespace std::chrono;

    // 提高定时器精度（Windows 必需，否则 Sleep 最小 15ms）
    timeBeginPeriod(1);  // 全局设置，程序结束时记得 timeEndPeriod(1)

    auto nextFrameTime = steady_clock::now();//第0帧时间节点

    //程序主循环
    while (TRUE) {
        if (g_GameSta.empty())break;//退出逻辑

        GetMsg();//监听键盘
        g_GameSta.update();//运行栈顶


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

void InitLobby() {
    //绘画大厅按键
    g_GameRander.UpdateMenu();
    g_GameSta.pop();
    g_GameSta.push(Lobby, STA_LOBBY);
}

void Lobby() {
    //大厅检测
    //已经有GegMsg()了
}

void InitSelect() {
    //绘画难度选择按键
    g_GameRander.UpdateMenu();
    g_GameSta.pop();
    g_GameSta.push(Select, STA_SELECT);
}

void Select() {
    //选择难度检测
    //已经有GegMsg()了
}

void InitPause() {
    //绘画暂停按键
    g_GameRander.UpdateMenu();
    g_GameSta.pop();
    g_GameSta.push(Pause, STA_PAUSE);
}

void Pause() {
    //暂停界面
    //已经有GegMsg()了
}

void InitOver() {
    //绘画暂停按键
    g_GameRander.UpdateMenu();
    g_GameTime.pause();
    g_BackTime.clear();
    g_EnterTime.clear();

    g_GameSta.pop();
    g_GameSta.push(Over, STA_OVER);
}

void Over() {
    //游戏结束界面
    //已经有GegMsg()了
}

void InitAccount() {
    //绘画结算画面
    g_GameRander.UpdateMenu();
    g_GameTime.pause();
    g_BackTime.clear();
    g_EnterTime.clear();

    g_GameSta.pop();
    g_GameSta.push(Account, STA_ACCOUNT);
}

void Account() {
    //结算界面
    //已经有GegMsg()了
}

void InitPuGameData() {
    //清空上一把残留数据
    g_GameTime.clear();
    g_GameLogic.initDotData();
    g_GameLogic.clearMapData();
    g_GameLogic.clearMapData();
    g_GameLogic.clearPlayerList();

    //队列初始化
    g_GameLogic.addPlayer(0, 0);

    //地图初始化和果子初始化
    g_GameLogic.initMapData(STA_PU_GAME);

    //更新?

    g_GameRander.renderGame(g_GameLogic);


    //三秒倒计时（新状态入栈

    //开始计时
    g_GameTime.start();

    g_GameSta.pop();
    g_GameSta.push(PuGame, STA_PU_GAME);
}

void PuGame() {
    g_GameTime.updata(); //游玩时间更新
    if (!g_BackTime.empty())g_BackTime.updata(); //空格长按时间更新

    //长按空格
    g_GameLogic.getPlayer(0).setV(g_BackTime.getSum());

    //蛇吃果子关键帧判定
    //g_GameLogic.updataMove(g_Player1.getBack('x'), g_Player1.getBack('y'), g_Player1.getDir(), 1);

    //刷新
    g_GameLogic.getPlayer(0).FrameAdd();
    if (g_GameLogic.getPlayer(0).getV() <= g_GameLogic.getPlayer(0).getFrame()) {
        //清空帧数计数
        g_GameLogic.getPlayer(0).clearFrame();
        g_GameLogic.updata(g_GameLogic.getPlayer(0));
    }

    //渲染
    if (g_GameLogic.getLoser() == 0) {
        g_GameRander.renderGame(g_GameLogic);
    }
    else {
        g_GameSta.push(InitOver, STA_OVER);
    }
}

void InitATGameData() {
    //清空上一把残留数据
    g_GameTime.clear();
    g_GameLogic.initDotData();
    g_GameLogic.clearMapData();
    g_GameLogic.clearMapData();
    g_GameLogic.clearPlayerList();//清空队列

    //队列初始化
    g_GameLogic.addPlayer(-5, 0);
    g_GameLogic.addPlayer(5, 0);

    //地图初始化和果子初始化
    g_GameLogic.initMapData(STA_AT_GAME);

    //更新?
    g_GameRander.renderGame(g_GameLogic);

    //三秒倒计时（新状态入栈

    //开始计时
    g_GameTime.start();

    g_GameSta.pop();
    g_GameSta.push(ATGame, STA_AT_GAME);
}

void ATGame() {
    g_GameTime.updata(); //游玩时间更新
    if (!g_BackTime.empty())g_BackTime.updata(); //空格长按时间更新
    if (!g_EnterTime.empty())g_EnterTime.updata(); //回车长按时间更新

    //各自刷新率
    g_GameLogic.getPlayer(0).setV(g_BackTime.getSum());//长按空格
    g_GameLogic.getPlayer(1).setV(g_EnterTime.getSum());//长按回车

    //刷新
    g_GameLogic.getPlayer(0).FrameAdd();
    g_GameLogic.getPlayer(1).FrameAdd();

    if (g_GameLogic.getPlayer(0).getV() <= g_GameLogic.getPlayer(0).getFrame()) {
        //清空帧数计数
        g_GameLogic.getPlayer(0).clearFrame();
        g_GameLogic.updata(g_GameLogic.getPlayer(0));
    }
    if (g_GameLogic.getPlayer(1).getV() <= g_GameLogic.getPlayer(1).getFrame()) {
        //清空帧数计数
        g_GameLogic.getPlayer(1).clearFrame();
        g_GameLogic.updata(g_GameLogic.getPlayer(1));
    }



    //渲染
    if (g_GameLogic.getLoser() == 0) {
        g_GameRander.renderGame(g_GameLogic);
    }
    else {
        g_GameSta.push(InitAccount, STA_ACCOUNT);
    }
}

void GetMsg() {
    while (peekmessage(&msg, EX_MOUSE | EX_KEY)) {

        //鼠标放开
        if (msg.message == WM_LBUTTONUP) {
            printf("鼠标松开，Sta=%d/%d\n", g_GameSta.getStaID(), (int)g_GameSta.getStaDepth());
            for (const Player& player : g_GameLogic.getPlayerList()) {
                std::cout << player.sizeQ() << std::endl;
            }
            //深度：1大厅 2选择难度 3游戏中 4暂停
            if (msg.x >= Larea.left && msg.x <= Larea.right && msg.y >= Larea.top && msg.y <= Larea.bottom) {
                //左
                if (g_GameSta.getStaDepth() == 2) {

                }
                if (g_GameSta.getStaDepth() == 4) {
                    //返回大厅
                    g_GameLogic.clearPlayerList();
                    for (int i = 0; i < 3; i++) {
                        g_GameSta.pop();
                    }
                    g_GameRander.UpdateMenu();
                }
                printf("左\n");
            }
            if (msg.x >= Marea.left && msg.x <= Marea.right && msg.y >= Marea.top && msg.y <= Marea.bottom) {
                //中
                if (g_GameSta.getStaDepth() == 2) {
                    //竞技模式
                    g_GameSta.push(InitATGameData, STA_AT_GAME);
                }
                else if (g_GameSta.getStaID() == STA_PAUSE) {
                    //重开（我应该使用StaID？）
                    g_GameSta.pop();
                    g_GameLogic.clearPlayerList();
                    if (g_GameSta.getStaID() == STA_PU_GAME) {
                        g_GameSta.pop();
                        g_GameSta.push(InitPuGameData, STA_PU_GAME);
                    }
                    else if (g_GameSta.getStaID() == STA_AT_GAME) {
                        g_GameSta.pop();
                        g_GameSta.push(InitATGameData, STA_AT_GAME);
                    }

                }
                printf("中\n");
            }
            if (msg.x >= Rarea.left && msg.x <= Rarea.right && msg.y >= Rarea.top && msg.y <= Rarea.bottom) {
                //右
                if (g_GameSta.getStaDepth() == 2) {
                    //简单模式
                    g_GameSta.push(InitPuGameData, STA_PU_GAME);
                }
                else if (g_GameSta.getStaID() == STA_PAUSE) {
                    //继续游戏
                    g_GameSta.pop();
                    g_GameTime.resume();
                    g_GameRander.renderGame(g_GameLogic);
                }
                else if (g_GameSta.getStaID() == STA_OVER || g_GameSta.getStaID() == STA_ACCOUNT) {
                    //重开（我应该使用StaID？）
                    g_GameSta.pop();
                    g_GameLogic.clearPlayerList();
                    if (g_GameSta.getStaID() == STA_PU_GAME) {
                        g_GameSta.pop();
                        g_GameSta.push(InitPuGameData, STA_PU_GAME);
                    }
                    else if (g_GameSta.getStaID() == STA_AT_GAME) {
                        g_GameSta.pop();
                        g_GameSta.push(InitATGameData, STA_AT_GAME);
                    }
                }
                printf("右\n");
            }
            if (msg.x >= Sarea.left && msg.x <= Sarea.right && msg.y >= Sarea.top && msg.y <= Sarea.bottom) {
                //开始
                if (g_GameSta.getStaDepth() == 1) {
                    g_GameSta.push(InitSelect, STA_SELECT);
                }
                //UpdateMenu();
                printf("按下开始按钮\n");
            }
        }

        // 按键按下
        if (msg.message == WM_KEYDOWN) {
            printf("键盘按下，Sta=%d\n", g_GameSta.getStaID());
            if (g_GameSta.getStaDepth() == 1) {
                switch (msg.vkcode) {
                case VK_ESCAPE:g_GameSta.pop(); g_GameRander.UpdateMenu(); break;//ESC
                }
            }
            else if (g_GameSta.getStaDepth() == 2) {
                switch (msg.vkcode) {
                case VK_ESCAPE:g_GameSta.pop(); g_GameRander.UpdateMenu(); break;//ESC
                }
            }
            else if (g_GameSta.getStaID() == STA_PU_GAME) {
                switch (msg.vkcode) {
                    //上右下左
                case VK_UP:    case 'W': g_GameLogic.getPlayer(0).up();    g_GameRander.setDownMode(0); break;
                case VK_RIGHT: case 'D': g_GameLogic.getPlayer(0).right(); g_GameRander.setDownMode(1); break;
                case VK_DOWN:  case 'S': g_GameLogic.getPlayer(0).down();  g_GameRander.setDownMode(2); break;
                case VK_LEFT:  case 'A': g_GameLogic.getPlayer(0).left();  g_GameRander.setDownMode(3); break;

                    //特殊按键
                case VK_ESCAPE://ESC
                    g_GameSta.push(InitPause, STA_PAUSE);
                    g_BackTime.clear();
                    g_GameTime.pause();
                    g_GameRander.setDownMode(-1);
                    break;

                case VK_SPACE://空格
                    if (g_BackTime.getSum() == 0) {
                        g_BackTime.start();
                    }
                    g_GameRander.setDownMode(-2);
                    break;
                }
            }
            else if (g_GameSta.getStaID() == STA_AT_GAME) {
                switch (msg.vkcode) {
                    //上右下左2
                case VK_UP:    g_GameLogic.getPlayer(1).up();    g_GameRander.setDownMode(0); break;
                case VK_RIGHT: g_GameLogic.getPlayer(1).right(); g_GameRander.setDownMode(1); break;
                case VK_DOWN:  g_GameLogic.getPlayer(1).down();  g_GameRander.setDownMode(2); break;
                case VK_LEFT:  g_GameLogic.getPlayer(1).left();  g_GameRander.setDownMode(3); break;
                    //wasd1
                case 'W': g_GameLogic.getPlayer(0).up();    g_GameRander.setDownMode(0); break;
                case 'D': g_GameLogic.getPlayer(0).right(); g_GameRander.setDownMode(1); break;
                case 'S': g_GameLogic.getPlayer(0).down();  g_GameRander.setDownMode(2); break;
                case 'A': g_GameLogic.getPlayer(0).left();  g_GameRander.setDownMode(3); break;

                    //特殊按键
                case VK_ESCAPE://ESC
                    g_GameSta.push(InitPause, STA_PAUSE);
                    g_BackTime.clear();
                    g_GameLogic.getPlayer(0).setFrame(g_GameLogic.getPlayer(1).getFrame());
                    g_GameTime.pause();
                    g_GameRander.setDownMode(-1);
                    break;

                case VK_SPACE://空格
                    if (g_BackTime.getSum() == 0) {
                        g_BackTime.start();
                    }
                    g_GameRander.setDownMode(-2);
                    break;
                case VK_RETURN://Enter
                    if (g_EnterTime.getSum() == 0) {
                        g_EnterTime.start();
                    }
                    g_GameRander.setDownMode(-2);
                    break;
                }
            }
            else if (g_GameSta.getStaID() == STA_PAUSE) {//这里本身就是第四层，直接用StaID处理
                switch (msg.vkcode) {
                case VK_ESCAPE://ESC
                    g_GameSta.pop();
                    g_GameTime.resume();
                    g_GameRander.renderGame(g_GameLogic);
                    break;
                }
            }
            else if (g_GameSta.getStaID() == STA_OVER) {//这里本身就是第四层，直接用StaID处理
                switch (msg.vkcode) {
                case VK_ESCAPE://ESC
                    //
                    break;
                }
            }
        }

        // 按键松开
        if (msg.message == WM_KEYUP) {
            printf("键盘松开，Sta=%d\n", g_GameSta.getStaID());
            //清空空格长按时间
            switch (msg.vkcode) {
            case VK_SPACE:
                g_BackTime.clear(); // 清零
                if (g_GameSta.getStaID() == STA_AT_GAME) g_GameLogic.getPlayer(0).setFrame(g_GameLogic.getPlayer(1).getFrame());
                break;
            case VK_RETURN:
                if (g_GameSta.getStaID() == STA_AT_GAME) g_EnterTime.clear(); // 清零
                if (g_GameSta.getStaID() == STA_AT_GAME) g_GameLogic.getPlayer(1).setFrame(g_GameLogic.getPlayer(0).getFrame());
                break;
            }
            //刷新按键码
            g_GameRander.setDownMode(4);
        }
    }

}

int Random() {
    return rand() % 10;
}



