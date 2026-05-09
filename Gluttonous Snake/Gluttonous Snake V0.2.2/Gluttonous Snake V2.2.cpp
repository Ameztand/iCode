/*
26.4.6V2.2版贪吃蛇

采用状态栈架构，封装在Stata类里并且需要手动把状态ID push到类里（方便其他函数比较当前状态）
并且提供一个获取栈深度的接口在某些场合化简比较多个栈状态（便于后续扩展游戏模式）
同时，关于游戏时间，检测空格长按和限制刷新率也封装在了类里面

相比2.1，我把逻辑判断和刷新固定为PFS_LOGIC，并且把2.0的吃果子bug和加速后刷新异步解决（加速完
设置成对方的速度不就好了）以及UI刷新也改了一下，这是加入双人模式后最好的一次了
并且，加速函数更新了
但是还是会存在初始果子刷新在初始蛇的身上bug但是遇到概率极低
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
#include <functional>
#include <map>
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
    //方向参数
    int dir_ = 0;//考虑用dydx合成速度
    int purrDir_ = 0;//刷新时修改，记录上一个方向防止往回走

    //帧率计数器
    int V = SNAKE_UPDATE;//蛇（移动）刷新速度？
    int Frame = 0;//限制蛇刷新速度（移动速度）

    // 蛇坐标队列
    std::queue<int> X_;
    std::queue<int> Y_;

public:
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

    void updataDir() {//防止向后转
        dir_ = 0;
    }

    void updataPurrDir() {
        purrDir_ = dir_;
    }

    int getDir() const {
        return dir_;
    }

    //队列方面
    void pushQ(int x, int y) {
        X_.push(x);
        Y_.push(y);
    }

    void popQ() {
        X_.pop();
        Y_.pop();
    }

    void clearQ() {
        while (!X_.empty()) {
            X_.pop();
            Y_.pop();
        }
    }

    bool emptyQ() {
        return X_.empty();
    }

    size_t sizeQ() const {
        return X_.size();
    }

    int getFront(char ch)const {
        return ch == 'x' ? X_.front() : Y_.front();
    }

    int getBack(char ch)const {
        return ch == 'x' ? X_.back() : Y_.back();
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

Player g_Player1;//wasd和back键
Player g_Player2;//箭头和Enter键

// ========== 逻辑层===========
class Logic {
private:
    //地图用于判断是否是蛇身，判断果子用数组
    int MapData[19][19] = { 0 };//0路 1蛇 2果子 4蛇头 5原点
    int DotData[3][4] = { 10 };//果子坐标，替换可能被吃掉的果子坐标的新果子坐标

    int NextMove[2][2] = { 0 };//可能的下一步的坐标

    bool PlayerLive[2] = { true };//是否存活
    bool BothLoser = { false };//是否苦命鸳鸯
    bool EatDot[2] = { false };//是否吃到果子

public:
    int getLoser() {
        if (BothLoser) {
            return 3;
        }
        else if (!PlayerLive[1]) {
            return 2;
        }
        else if (!PlayerLive[0]) {
            return 1;
        }
    }

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
            NextMove[0][0] = 0;
            NextMove[0][1] = 1;
        }
        else if (mode == STA_AT_GAME) {
            MapData[4][9] = 4; //（-5，0）
            MapData[4][10] = 1;//（-5，-1）
            MapData[4][11] = 1;//（-5，-2）
            MapData[14][9] = 4; //（5，0）
            MapData[14][10] = 1;//（5，-1）
            MapData[14][11] = 1;//（5，-2）
            NextMove[0][0] = -5;
            NextMove[0][1] = 1;
            NextMove[1][0] = 5;
            NextMove[1][1] = 1;
        }


        //果子坐标初始化
        for (int i = 0; i < 3; i++) {
            CreatDot(0, 0, 0, 0, i);
            DotData[i][0] = DotData[i][2];
            DotData[i][1] = DotData[i][3];
        }
    }

    int getDotData(int i, int j) const {//输入坐标 返回状态
        return DotData[i][j];
    }

    //生成一个果子
    void CreatDot(int nx, int ny, int px, int py, int i) {
        //获取随机数
        int x = Random() * (Random() > 4 ? 1 : -1);
        int y = Random() * (Random() > 4 ? 1 : -1);
        while (MapData[x + 9][-y + 9] != 0 && (x != nx && y != ny) && (x != px && y != py)) {
            x = Random() * (Random() > 4 ? 1 : -1);
            y = Random() * (Random() > 4 ? 1 : -1);
        }

        //放到缓冲区
        DotData[i][2] = x;
        DotData[i][3] = y;
    }

    void clearDot(int i) {
        DotData[i][2] = DotData[i][0];
        DotData[i][3] = DotData[i][1];
    }

    void updataDot() {
        for (int i = 0; i < 3; i++) {
            DotData[i][0] = DotData[i][2];
            DotData[i][1] = DotData[i][3];
        }
    }

    //判断移动请求
    void updataMove(int x, int y, int dir, int id) {
        id -= 1;

        int dx[4] = { 0,1,0,-1 };//上右下左
        int dy[4] = { 1,0,-1,0 };

        //获取上一个移动请求坐标
        int nx = x + dx[dir];
        int ny = y + dy[dir];
        int px = NextMove[id][0];
        int py = NextMove[id][1];

        //检查果子（恢复）
        for (int i = 0; i < 3; i++) {
            if (DotData[i][0] == px && DotData[i][1] == py) {
                DotData[i][2] = DotData[i][0];
                DotData[i][3] = DotData[i][1];
                EatDot[id] = false;
                printf("恢复（%d,%d）\n", px, py);
            }
        }

        //判断下一格
        int ItemMapData = MapData[nx + 9][-ny + 9];
        if (nx < -9 || ny < -9 || nx>9 || ny>9 || ItemMapData == 1 || ItemMapData == 4) {
            //gameover
            PlayerLive[id] = false;
        }
        else {
            PlayerLive[id] = true;
            //检测果子是否被吃
            for (int i = 0; i < 3; i++) {
                if (DotData[i][0] == nx && DotData[i][1] == ny) {
                    CreatDot(nx, ny, px, py, i);
                    EatDot[id] = true;
                    break;
                }
            }
        }

        if (g_GameSta.getStaID() == STA_AT_GAME) {
            //检查是否头撞头(抢一个格子)
            int oid = (id == 0 ? 1 : 0);
            if (nx == NextMove[oid][0] && ny == NextMove[oid][1]) {
                PlayerLive[0] = false;
                PlayerLive[1] = false;
            }
        }

        BothLoser = ((!PlayerLive[0] && !PlayerLive[1]) ? true : false);//检查是否一起死
        NextMove[id][0] = nx;//更新预测下一格坐标
        NextMove[id][1] = ny;
    }

    void BeforeRander1(int mode) {
        if ((g_GameSta.getStaID() == STA_AT_GAME && PlayerLive[0] && PlayerLive[1]) || (g_GameSta.getStaID() == STA_PU_GAME && PlayerLive[0])) {
            //更新蛇头
            MapData[g_Player1.getBack('x') + 9][-g_Player1.getBack('y') + 9] = 1;
            g_Player1.pushQ(NextMove[0][0], NextMove[0][1]);
            MapData[g_Player1.getBack('x') + 9][-g_Player1.getBack('y') + 9] = 4;

            //消去尾巴
            if (EatDot[0]) {
                EatDot[0] = false;//吃到果子
            }
            else {
                MapData[g_Player1.getFront('x') + 9][-g_Player1.getFront('y') + 9] = 0;
                g_Player1.popQ();//没吃到
            }

            //更新方向参数
            g_Player1.updataPurrDir();

            //更新果子
            for (int i = 0; i < 3; i++) {
                MapData[DotData[i][2] + 9][-DotData[i][3] + 9] = 2;
                DotData[i][0] = DotData[i][2];
                DotData[i][1] = DotData[i][3];
            }
        }
        else {
            if (g_GameSta.getStaID() == STA_PU_GAME) g_GameSta.push(InitOver, STA_OVER);
            if (g_GameSta.getStaDepth() == 3) g_GameSta.push(InitAccount, STA_ACCOUNT);
        }

    }

    void BeforeRander2(int mode) {
        if ((g_GameSta.getStaID() == STA_AT_GAME && PlayerLive[0] && PlayerLive[1])) {
            //更新蛇头
            MapData[g_Player2.getBack('x') + 9][-g_Player2.getBack('y') + 9] = 1;
            g_Player2.pushQ(NextMove[1][0], NextMove[1][1]);
            MapData[g_Player2.getBack('x') + 9][-g_Player2.getBack('y') + 9] = 4;

            //消去尾巴
            if (EatDot[1]) {
                EatDot[1] = false;//吃到果子
            }
            else {
                MapData[g_Player2.getFront('x') + 9][-g_Player2.getFront('y') + 9] = 0;
                g_Player2.popQ();//没吃到
            }

            //更新方向参数
            g_Player2.updataPurrDir();

            //更新果子
            for (int i = 0; i < 3; i++) {
                MapData[DotData[i][2] + 9][-DotData[i][3] + 9] = 2;
                DotData[i][0] = DotData[i][2];
                DotData[i][1] = DotData[i][3];
            }
        }
        else {
            if (g_GameSta.getStaDepth() == 3) g_GameSta.push(InitAccount, STA_ACCOUNT);
        }

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

    void renderGame() {
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
            Print(g_GameLogic.getDotData(i, 0), g_GameLogic.getDotData(i, 1), 2);
        }

        //打印蛇
        int len = g_Player1.sizeQ();
        int x = 0;
        int y = 0;
        for (int i = len; i > 0; i--) {
            x = g_Player1.getFront('x');
            y = g_Player1.getFront('y');
            Print(x, y, (i == 1 ? 4 : 1));
            g_Player1.pushQ(x, y);
            g_Player1.popQ();
        }
        len = g_Player2.sizeQ();
        for (int i = len; i > 0; i--) {
            x = g_Player2.getFront('x');
            y = g_Player2.getFront('y');
            Print(x, y, (i == 1 ? 4 : 1));
            g_Player2.pushQ(x, y);
            g_Player2.popQ();
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
            swprintf(buffer, 100, L"当前速度：%d / %lld                                                                      ", 1000 / g_Player1.getV(), g_BackTime.getSum());
            outtextxy(800, 340, buffer);
        }
        else if (g_GameSta.getStaID() == STA_AT_GAME && g_GameSta.getStaDepth() >= 3) {
            swprintf(buffer, 100, L"当前速度：%d / %d / %lld / %lld                                                                      ", 1000 / g_Player1.getV(), 1000 / g_Player2.getV(), g_BackTime.getSum(), g_EnterTime.getSum());
            outtextxy(800, 340, buffer);
        }


        //打印长度提示
        if (g_GameSta.getStaID() == STA_PU_GAME) {
            swprintf(buffer, 100, L"当前长度：%d                                          ", (int)g_Player1.sizeQ());
            outtextxy(800, 380, buffer);
        }
        else if (g_GameSta.getStaID() == STA_AT_GAME) {
            swprintf(buffer, 100, L"当前长度：%d / %d                                          ", (int)g_Player1.sizeQ(), (int)g_Player2.sizeQ());
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
    g_Player1.clearFrame();
    g_Player2.clearFrame();
    g_Player1.updataDir();
    g_Player2.updataDir();
    g_GameTime.clear();
    g_GameLogic.clearMapData();
    g_Player1.clearQ();//清空队列
    g_Player2.clearQ();//清空队列

    //队列初始化
    g_Player1.pushQ(0, -2);
    g_Player1.pushQ(0, -1);
    g_Player1.pushQ(0, 0);

    //地图初始化和果子初始化
    g_GameLogic.initMapData(STA_PU_GAME);

    //更新?
    //g_GameLogic.BeforeRander();
    g_GameRander.renderGame();


    //三秒倒计时（新状态入栈

    //开始计时
    //g_GameTime.clear();
    g_GameTime.start();

    g_GameSta.pop();
    g_GameSta.push(PuGame, STA_PU_GAME);
}

void PuGame() {
    g_GameTime.updata(); //游玩时间更新
    if (!g_BackTime.empty())g_BackTime.updata(); //空格长按时间更新

    //长按空格
    g_Player1.setV(g_BackTime.getSum());

    //蛇吃果子关键帧判定
    g_GameLogic.updataMove(g_Player1.getBack('x'), g_Player1.getBack('y'), g_Player1.getDir(), 1);

    //刷新
    g_Player1.FrameAdd();
    if (g_Player1.getV() <= g_Player1.getFrame()) {
        //清空帧数计数
        g_Player1.clearFrame();
        g_GameLogic.BeforeRander1(STA_PU_GAME);//??? 
    }
    g_GameRander.renderGame();
}

void InitATGameData() {
    //清空上一把残留数据
    g_Player1.clearFrame();
    g_Player2.clearFrame();
    g_Player1.updataDir();
    g_Player2.updataDir();
    g_GameTime.clear();
    g_GameLogic.clearMapData();
    g_Player1.clearQ();//清空队列
    g_Player2.clearQ();//清空队列

    //队列初始化
    g_Player1.pushQ(-5, -2);
    g_Player1.pushQ(-5, -1);
    g_Player1.pushQ(-5, 0);
    g_Player2.pushQ(5, -2);
    g_Player2.pushQ(5, -1);
    g_Player2.pushQ(5, 0);

    //地图初始化和果子初始化
    g_GameLogic.initMapData(STA_AT_GAME);

    //更新?
    //g_GameLogic.BeforeRander();
    g_GameRander.renderGame();

    //三秒倒计时（新状态入栈

    //开始计时
    //g_GameTime.clear();
    g_GameTime.start();

    g_GameSta.pop();
    g_GameSta.push(ATGame, STA_AT_GAME);
}

void ATGame() {
    g_GameTime.updata(); //游玩时间更新
    if (!g_BackTime.empty())g_BackTime.updata(); //空格长按时间更新
    if (!g_EnterTime.empty())g_EnterTime.updata(); //回车长按时间更新

    //各自刷新率
    g_Player1.setV(g_BackTime.getSum());//长按空格
    g_Player2.setV(g_EnterTime.getSum());//长按回车


    //蛇吃果子关键帧判定
    g_GameLogic.updataMove(g_Player1.getBack('x'), g_Player1.getBack('y'), g_Player1.getDir(), 1);
    g_GameLogic.updataMove(g_Player2.getBack('x'), g_Player2.getBack('y'), g_Player2.getDir(), 2);

    //刷新
    g_Player1.FrameAdd();
    g_Player2.FrameAdd();
    if (g_Player1.getV() <= g_Player1.getFrame()) {
        //清空帧数计数
        g_Player1.clearFrame();
        g_GameLogic.BeforeRander1(STA_AT_GAME);
    }
    if (g_Player2.getV() <= g_Player2.getFrame()) {
        //清空帧数计数
        g_Player2.clearFrame();
        g_GameLogic.BeforeRander2(STA_AT_GAME);
    }
    g_GameRander.renderGame();
}

void GetMsg() {
    while (peekmessage(&msg, EX_MOUSE | EX_KEY)) {

        //鼠标放开
        if (msg.message == WM_LBUTTONUP) {
            printf("鼠标松开，Sta=%d/%d\n", g_GameSta.getStaID(), (int)g_GameSta.getStaDepth());
            //深度：1大厅 2选择难度 3游戏中 4暂停
            if (msg.x >= Larea.left && msg.x <= Larea.right && msg.y >= Larea.top && msg.y <= Larea.bottom) {
                //左
                if (g_GameSta.getStaDepth() == 2) {

                }
                if (g_GameSta.getStaDepth() == 4) {
                    //返回大厅
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
                    g_GameRander.renderGame();
                }
                else if (g_GameSta.getStaID() == STA_OVER || g_GameSta.getStaID() == STA_ACCOUNT) {
                    //重开（我应该使用StaID？）
                    g_GameSta.pop();
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
                case VK_UP:    case 'W': g_Player1.up();    g_GameRander.setDownMode(0); break;
                case VK_RIGHT: case 'D': g_Player1.right(); g_GameRander.setDownMode(1); break;
                case VK_DOWN:  case 'S': g_Player1.down();  g_GameRander.setDownMode(2); break;
                case VK_LEFT:  case 'A': g_Player1.left();  g_GameRander.setDownMode(3); break;

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
                case VK_UP:    g_Player2.up();    g_GameRander.setDownMode(0); break;
                case VK_RIGHT: g_Player2.right(); g_GameRander.setDownMode(1); break;
                case VK_DOWN:  g_Player2.down();  g_GameRander.setDownMode(2); break;
                case VK_LEFT:  g_Player2.left();  g_GameRander.setDownMode(3); break;
                    //wasd1
                case 'W': g_Player1.up();    g_GameRander.setDownMode(0); break;
                case 'D': g_Player1.right(); g_GameRander.setDownMode(1); break;
                case 'S': g_Player1.down();  g_GameRander.setDownMode(2); break;
                case 'A': g_Player1.left();  g_GameRander.setDownMode(3); break;

                    //特殊按键
                case VK_ESCAPE://ESC
                    g_GameSta.push(InitPause, STA_PAUSE);
                    g_BackTime.clear();
                    g_Player1.setFrame(g_Player2.getFrame());
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
                    g_GameRander.renderGame();
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
                g_Player1.setFrame(g_Player2.getFrame());
                break;
            case VK_RETURN:
                g_EnterTime.clear(); // 清零
                g_Player2.setFrame(g_Player1.getFrame());
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



