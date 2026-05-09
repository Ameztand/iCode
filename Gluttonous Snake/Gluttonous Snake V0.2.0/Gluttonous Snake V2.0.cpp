/*
26.3.30第二版贪吃蛇

采用状态栈架构，封装在Stata类里并且需要手动把状态ID push到类里（方便其他函数比较当前状态）
并且提供一个获取栈深度的接口在某些场合化简比较多个栈状态（便于后续扩展游戏模式）
同时，关于游戏时间，检测空格长按和限制刷新率也封装在了类里面

在后续的扩展中，我把蛇头判定关键帧判定封装在来玩家类里，并且把蛇的移动速度、蛇身队列和方向参数也封装在这个类里面
简单实现双人对局（当然，一开始是想设计成单人脑裂玩两条蛇的），并简单设计一个结算界面
但是，原本的关键帧判定是针对单人游戏的，无法处理同时碰壁或身体，以及头撞头抢格子的关键帧判定，后续需要优化这部分
*/


#define SNAKE_UPDATE 20 //初始蛇蛇刷新
#define FRAME_DALAY 10 //刷新率延迟
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
#include <cstdlib>  // rand(), srand()
#include <ctime>    // time()


void initData();

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


void UpdateUI_resume();
void UpdateUI();
void UpdateMenu();

void Print(int x, int y, int color);
void CreatDot();
int Random();


int DMode = 4;//按下按键代码
int MapData[19][19] = { 0 };
//果子坐标
int EDot[3][2] = { 10 };
bool g_Key[256] = { false };


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
Time g_GameTime;//记录游戏时间
Time g_BackTime;//空格按下时间
Time g_EnterTime;//回车按下时间
Time g_LimitFPS;//限制帧率


// ========== 玩家管理器 ===========
class Player {
private:
    int dir_ = 0;//考虑用dydx合成速度
    int purrDir_ = 0;//刷新时修改，记录上一个方向防止往回走

    int V = SNAKE_UPDATE;//蛇（移动）刷新速度？
    //FPS计数器
    int Frame = 0;//限制蛇刷新速度（移动速度）

    bool loser = false;

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

    //蛇头关键帧
    void judgeSnakeHead() {  //无法处理同时碰壁或身体，以及头撞头抢格子的关键帧判定
        //蛇头关键帧判定
        int dx[4] = { 0,1,0,-1 };//上右下左
        int dy[4] = { 1,0,-1,0 };

        int nx = X_.back() + dx[dir_];
        int ny = Y_.back() + dy[dir_];
        if (nx < -9 || ny < -9 || nx>9 || ny>9 || MapData[nx + 9][-ny + 9] == 1) {
            //gameover
            if (g_GameSta.getStaDepth() == 3) {
                if (g_GameSta.getStaID() == STA_PU_GAME) {
                    g_GameSta.push(InitOver, STA_OVER);
                }
                else if (g_GameSta.getStaID() == STA_AT_GAME) {
                    loser = true;
                    g_GameSta.push(InitAccount, STA_ACCOUNT);
                }
            }
        }
        else {
            //消去尾巴//消去时机
            if (MapData[nx + 9][-ny + 9] != 2) {
                Print(X_.front(), Y_.front(), 0);
                MapData[X_.front() + 9][-Y_.front() + 9] = 0;
                popQ();
            }
            else {
                //吃到果子，立即刷新一个果子
                for (int i = 0; i < 3; i++) {
                    if (EDot[i][0] == nx && EDot[i][1] == ny) {
                        EDot[i][0] = 10;
                        EDot[i][1] = 10;
                        break;
                    }
                }
                CreatDot();
            }

            //添加蛇头
            MapData[nx + 9][-ny + 9] = 1;//蛇头
            pushQ(nx, ny);

            //更新蛇
            updataSnake();

            //防止向后转
            purrDir_ = dir_;
        }
    }

    void updataSnake() {
        // 打印蛇身 / 头
        int len = X_.size();
        for (int i = 0; i < len; i++) {
            Print(X_.front(), Y_.front(), (i == (len - 1) ? 1 : 4));
            WheelQueue(X_.front(), Y_.front());
        }

        //原点提示
        if (MapData[9][9] == 0) {
            Print(0, 0, 6);
        }
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

    void WheelQueue(int x, int y) {
        X_.push(x);
        Y_.push(y);
        X_.pop();
        Y_.pop();
    }

    //速度方面
    void setV(long long item) {
        if (item >= LOW_DOWN_TIME) {
            V = 5 + 3000 / item;//15-5////////???????????
        }
        else {
            V = SNAKE_UPDATE;
        }
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

    //失败判定方面
    bool getLoser() const {
        return loser == true;
    }

    void clearLoser() {
        loser = false;
    }
};

Player g_Player1;//wasd和back键
Player g_Player2;//箭头和Enter键


COLORREF colors[] = {
    RGB(255,255,255),  // 白0
    RGB(255, 0, 0),    // 红1
    RGB(255, 128, 0),  // 橙2
    RGB(255, 255, 0),  // 黄3
    RGB(0, 255, 0),    // 绿4
    RGB(0, 0, 255),    // 蓝5
    RGB(128, 0, 255),  // 紫6
};

ExMessage msg;
RECT Larea = { 362, 402, 458, 448 };//左按钮
RECT Marea = { 492, 402, 588, 448 };//中按钮
RECT Rarea = { 622, 402, 718, 448 };//右按钮
RECT Sarea = { 492, 502, 588, 548 };//开始按钮


// ================== 主函数 =====================
int main()
{
    //初始化
    initData();

    //程序主循环
    while (TRUE) {
        //帧率采集
        g_LimitFPS.start();

        if (g_GameSta.empty())break;//退出逻辑

        GetMsg();//监听键盘
        g_GameSta.update();//运行栈顶

        //限制帧率
        g_Player1.FrameAdd();
        g_Player2.FrameAdd();
        g_LimitFPS.updata();
        if (g_LimitFPS.getSum() < FRAME_DALAY)Sleep(FRAME_DALAY - g_LimitFPS.getSum());
        g_LimitFPS.clear();
    }

    closegraph();
    return 0;
}

//初始化数据
void initData() {
    //生成画布
    initgraph(1080, 720);
    setbkcolor(WHITE);             //设置背景（白）
    cleardevice();
    setbkmode(OPAQUE);//TRANSPARENT

    //加载随机数
    srand((unsigned int)time(NULL));

    //初始化栈
    g_GameSta.push(InitLobby, STA_LOBBY);
}

void InitLobby() {
    //绘画大厅按键
    UpdateMenu();
    g_GameSta.pop();
    g_GameSta.push(Lobby, STA_LOBBY);
}

void Lobby() {
    //大厅检测
    //已经有GegMsg()了
}

void InitSelect() {
    //绘画难度选择按键
    UpdateMenu();
    g_GameSta.pop();
    g_GameSta.push(Select, STA_SELECT);
}

void Select() {
    //选择难度检测
    //已经有GegMsg()了
}

void InitPause() {
    //绘画暂停按键
    UpdateMenu();
    g_GameSta.pop();
    g_GameSta.push(Pause, STA_PAUSE);
}

void Pause() {
    //暂停界面
    //已经有GegMsg()了
}

void InitOver() {
    //绘画暂停按键
    UpdateMenu();
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
    UpdateMenu();
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
    g_Player1.updataDir();
    g_GameTime.clear();
    for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 19; j++) {
            MapData[i][j] = 0;
        }
    }
    g_Player1.clearQ();//清空队列

    //队列初始化
    g_Player1.pushQ(0, -2);
    g_Player1.pushQ(0, -1);
    g_Player1.pushQ(0, 0);

    //地图初始化
    MapData[9][9] = 1; //（0，0）
    MapData[9][10] = 1;//（0，-1）
    MapData[9][11] = 1;//（0，-2）

    //果子坐标初始化
    for (int i = 0; i < 3; i++) {
        EDot[i][0] = 10;
        EDot[i][1] = 10;
    }

    //更新?
    UpdateUI_resume();
    g_Player1.updataSnake();//?
    UpdateUI();

    //添加3个果子
    for (int i = 0; i < 3; i++) {
        CreatDot();
    }

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
    if (g_Player1.getV() <= g_Player1.getFrame()) {
        //清空帧数计数
        g_Player1.clearFrame();

        g_Player1.judgeSnakeHead();
    }

    //刷新右侧标语
    UpdateUI();


}

void InitATGameData() {
    //清空上一把残留数据
    g_Player1.updataDir();
    g_Player2.updataDir();
    g_Player1.clearLoser();
    g_Player2.clearLoser();
    g_GameTime.clear();
    for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 19; j++) {
            MapData[i][j] = 0;
        }
    }
    g_Player1.clearQ();//清空队列
    g_Player2.clearQ();//清空队列

    //队列初始化
    g_Player1.pushQ(-5, -2);
    g_Player1.pushQ(-5, -1);
    g_Player1.pushQ(-5, 0);
    g_Player2.pushQ(5, -2);
    g_Player2.pushQ(5, -1);
    g_Player2.pushQ(5, 0);

    //地图初始化
    MapData[4][9] = 1; //（-5，0）
    MapData[4][10] = 1;//（-5，-1）
    MapData[4][11] = 1;//（-5，-2）
    MapData[14][9] = 1; //（5，0）
    MapData[14][10] = 1;//（5，-1）
    MapData[14][11] = 1;//（5，-2）

    //果子坐标初始化
    for (int i = 0; i < 3; i++) {
        EDot[i][0] = 10;
        EDot[i][1] = 10;
    }

    //更新?
    UpdateUI_resume();
    g_Player1.updataSnake();//?
    g_Player2.updataSnake();//?
    UpdateUI();

    //添加3个果子
    for (int i = 0; i < 3; i++) {
        CreatDot();
    }

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
    if (g_Player1.getV() <= g_Player1.getFrame()) {
        //清空帧数计数
        g_Player1.clearFrame();

        g_Player1.judgeSnakeHead();
    }
    if (g_Player2.getV() <= g_Player2.getFrame()) {
        //清空帧数计数
        g_Player2.clearFrame();

        g_Player2.judgeSnakeHead();
    }

    //刷新右侧标语
    UpdateUI();
}

void GetMsg() {
    if (peekmessage(&msg, EX_MOUSE | EX_KEY)) {

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
                    UpdateMenu();
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
                    UpdateUI_resume();
                    g_Player1.updataSnake();//?
                    g_Player2.updataSnake();//?
                    UpdateUI();
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
                case VK_ESCAPE:g_GameSta.pop(); UpdateMenu(); break;//ESC
                }
            }
            else if (g_GameSta.getStaDepth() == 2) {
                switch (msg.vkcode) {
                case VK_ESCAPE:g_GameSta.pop(); UpdateMenu(); break;//ESC
                }
            }
            else if (g_GameSta.getStaID() == STA_PU_GAME) {
                switch (msg.vkcode) {
                    //上右下左
                case VK_UP:    case 'W': g_Player1.up();    DMode = 0; break;
                case VK_RIGHT: case 'D': g_Player1.right(); DMode = 1; break;
                case VK_DOWN:  case 'S': g_Player1.down();  DMode = 2; break;
                case VK_LEFT:  case 'A': g_Player1.left();  DMode = 3; break;

                    //特殊按键
                case VK_ESCAPE://ESC
                    g_GameSta.push(InitPause, STA_PAUSE);
                    g_BackTime.clear();
                    g_GameTime.pause();
                    DMode = -1;
                    break;

                case VK_SPACE://空格
                    if (g_BackTime.getSum() == 0) {
                        g_BackTime.start();
                    }
                    DMode = -2;
                    break;
                }
            }
            else if (g_GameSta.getStaID() == STA_AT_GAME) {
                switch (msg.vkcode) {
                    //上右下左2
                case VK_UP:    g_Player2.up();    DMode = 0; break;
                case VK_RIGHT: g_Player2.right(); DMode = 1; break;
                case VK_DOWN:  g_Player2.down();  DMode = 2; break;
                case VK_LEFT:  g_Player2.left();  DMode = 3; break;
                    //wasd1
                case 'W': g_Player1.up();    DMode = 0; break;
                case 'D': g_Player1.right(); DMode = 1; break;
                case 'S': g_Player1.down();  DMode = 2; break;
                case 'A': g_Player1.left();  DMode = 3; break;

                    //特殊按键
                case VK_ESCAPE://ESC
                    g_GameSta.push(InitPause, STA_PAUSE);
                    g_BackTime.clear();
                    g_GameTime.pause();
                    DMode = -1;
                    break;

                case VK_SPACE://空格
                    if (g_BackTime.getSum() == 0) {
                        g_BackTime.start();
                    }
                    DMode = -2;
                    break;
                case VK_RETURN://Enter
                    if (g_EnterTime.getSum() == 0) {
                        g_EnterTime.start();
                    }
                    DMode = -2;
                    break;
                }
            }
            else if (g_GameSta.getStaID() == STA_PAUSE) {//这里本身就是第四层，直接用StaID处理
                switch (msg.vkcode) {
                case VK_ESCAPE://ESC
                    g_GameSta.pop();
                    g_GameTime.resume();
                    UpdateUI_resume();
                    g_Player1.updataSnake();//?
                    g_Player2.updataSnake();//?
                    UpdateUI();
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
                break;
            case VK_RETURN:
                g_EnterTime.clear(); // 清零
                break;
            }
            //刷新按键码
            DMode = 4;
        }
    }

}

void UpdateUI_resume() {
    //清空tip框，填充白色
    setfillcolor(colors[0]);
    solidrectangle(74, 74, 772, 646);//308, 238, 772, 482

    // 画棋盘表格
    setlinecolor(BLUE);             // 线条颜色
    setlinestyle(PS_SOLID, 3);     // 线型：实线，宽度3像素
    for (int i = 0; i < 20; i++) {//30,75
        line(75, 75 + 30 * i, 645, 75 + 30 * i);
        line(75 + 30 * i, 75, 75 + 30 * i, 645);
    }

    //显示title标语
    settextcolor(BLACK);
    outtextxy(75, 55, L"Hello, HungerSnake!");

    //打印果子
    for (int i = 0; i < 3; i++) {
        if (EDot[i][0] != 10) {
            Print(EDot[i][0], EDot[i][1], 3);
        }
    }

}

void UpdateUI() {
    //打印游玩时长
    wchar_t buffer[100];
    swprintf(buffer, 100, L"游戏时间：%llds               ", g_GameTime.getSum() / 1000);
    outtextxy(800, 360, buffer);

    //打印速度
    if (g_GameSta.getStaID() == STA_PU_GAME) {
        swprintf(buffer, 100, L"当前速度：%d / %lld                                                                      ", 1000 / g_Player1.getV(), g_BackTime.getSum());
        outtextxy(800, 340, buffer);
    }
    else if (g_GameSta.getStaID() == STA_AT_GAME) {
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
    if (DMode == 0) {
        swprintf(buffer, 100, L"当前按下：↑              ");
    }
    else if (DMode == 1) {
        swprintf(buffer, 100, L"当前按下：→              ");
    }
    else if (DMode == 2) {
        swprintf(buffer, 100, L"当前按下：↓              ");
    }
    else if (DMode == 3) {
        swprintf(buffer, 100, L"当前按下：←              ");
    }
    else if (DMode == -1) {
        swprintf(buffer, 100, L"当前按下：ESC             ");
    }
    else if (DMode == -2) {
        swprintf(buffer, 100, L"当前按下：BaclSpace");
    }
    else {
        swprintf(buffer, 100, L"当前按下：None             ");
    }
    outtextxy(800, 400, buffer);
}

void UpdateMenu() {
    printf("menu更新，当前Sta=%d\n", g_GameSta.getStaID());

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
        swprintf(buffer, 100, L"Player%d VICTORY !!!", (g_Player1.getLoser() ? 2 : 1));
        outtextxy(500, 310, buffer);//??

    }
    setbkmode(OPAQUE);//TRANSPARENT
}

void Print(int x, int y, int color) {
    setfillcolor(colors[color]);
    solidrectangle(X_CODE_PIONE + x * 30, X_CODE_PIONE - y * 30, Y_CODE_PIONE + x * 30, Y_CODE_PIONE - y * 30);
}

void CreatDot() {
    //获取随机数
    int x = Random() * (Random() > 4 ? 1 : -1);
    int y = Random() * (Random() > 4 ? 1 : -1);
    while (MapData[x + 9][-y + 9] != 0) {
        x = Random() * (Random() > 4 ? 1 : -1);
        y = Random() * (Random() > 4 ? 1 : -1);
    }

    //接入数组
    for (int i = 0; i < 3; i++) {
        if (EDot[i][0] == 10) {
            EDot[i][0] = x;
            EDot[i][1] = y;
            break;
        }
    }

    //接入MapData地图
    MapData[x + 9][-y + 9] = 2;

    //打印色块
    Print(x, y, 3);
}

int Random() {
    return rand() % 10;
}



