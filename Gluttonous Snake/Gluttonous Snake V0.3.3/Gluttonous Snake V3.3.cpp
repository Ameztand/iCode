/*
26.4.2V3.3版贪吃蛇

改成状态模式，映入状态基和游戏基，目前只做了迁移工作，尚未清楚目前bug
*/


#define SNAKE_UPDATE 20 //初始蛇蛇刷新
#define FPS_LOGIC 60.0  //逻辑刷新率
#define LOW_DOWN_TIME 200 //最短按下时间
#define X_CODE_PIONE  346   //(0,0)左上坐标
#define Y_CODE_PIONE  373   //(0,0)右下坐标
#define SNAKE_LEN 3     //初始蛇长

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
const int dx[4] = { 0,1,0,-1 };
const int dy[4] = { 1,0,-1,0 };


#include <queue>//为了队列
#include <stack>//为了栈
#include <iostream>//数据流
#include <vector>//动态数组
#include <algorithm>//快速清空数组
#include <memory>     // std::unique_ptr 智能指针
#include <typeindex>  // std::type_index 用于运行时类型信息
//#include <functional>
#include <unordered_map>
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

class IGameState;
class BaseGame;

class PUGame;
class ATGame;
class EXGame;

class Stata;
class Logic;
class Renderer;
class Msg;
class Time;

class Lobby;
class Select;
class Pause;
class Over;
class Death;


struct Context {
    Stata& stata;
    Logic& logic;
    Renderer& renderer;

    Msg& msg;

    Time& gameTime;//记录游戏时间
};

RECT Larea = { 362, 402, 458, 448 };//左按钮
RECT Marea = { 492, 402, 588, 448 };//中按钮
RECT Rarea = { 622, 402, 718, 448 };//右按钮
RECT Sarea = { 492, 502, 588, 548 };//开始按钮


void ESCpop(Context& ctx);
void returnToLobby(Context& ctx);
void clearPastData(Context& ctx);
void MoveMsg(Context& ctx, int p1, int p2);
void MirrorMoveMsg(Context& ctx, int p1, int p2);

//========== 状态基 ==========
class IGameState {
public:
    virtual ~IGameState() = default;
    virtual void onEnter(Context& ctx) = 0;
    virtual void onExit(Context& ctx) = 0;
    virtual void tick(Context& ctx) = 0;
    virtual int getID() const = 0;
};

// ========== 状态机管理器 ==========
class Stata {
private:
    std::stack<std::unique_ptr<IGameState>> stack;

public:
    // 压栈
    void pushSta(Context& ctx, std::unique_ptr<IGameState> newState) {
        if (newState) {
            newState->onEnter(ctx);           // 自动调用具体状态的 onEnter
            stack.push(std::move(newState));
        }
    }

    //弹出
    void popSta(Context& ctx) {
        if (!stack.empty()) {
            stack.top()->onExit(ctx);          // 自动调用具体状态的 onExit
            stack.pop();
        }
    }

    //替换
    void changeSta(Context& ctx, std::unique_ptr<IGameState> newState) {
        if (!stack.empty()) {
            stack.top()->onExit(ctx);          // 自动调用具体状态的 onExit
            stack.pop();
        }
        if (newState) {
            newState->onEnter(ctx);           // 自动调用具体状态的 onEnter
            stack.push(std::move(newState));
        }
    }

    //获取栈顶对象
    IGameState* getState() {
        return stack.empty() ? nullptr : stack.top().get();
    }

    //是否为空
    bool empty() {
        return stack.empty();
    }

    // 获取栈深度
    int getStaDepth() const {
        return (int)stack.size();
    }

    //获取栈顶id
    int getStaID()const {
        return (stack.empty()) ? 0 : stack.top()->getID();
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
    void initPlayerData(int x, int y, int dir) {

        Live_ = true;

        dir_ = dir;//方向参数
        purrDir_ = dir;//刷新时修改，记录上一个方向防止往回走
        V = SNAKE_UPDATE;//蛇（移动）刷新速度？
        Frame = 0;//限制蛇刷新速度（移动速度）
        E = 0;//极限模式加速能量参考

        //初始位置
        Position pos[3];
        pos[0] = { x,y };

        for (int i = 1; i < SNAKE_LEN; i++) {
            pos[i] = { pos[i - 1].x - dx[dir],pos[i - 1].y - dy[dir] };
        }

        for (int i = SNAKE_LEN - 1; i >= 0; i--) {
            posQueue.push(pos[i]);
        }

        NextPos = { x + dx[dir],y + dy[dir] };
    }

    //蛇头前一格相关
    const Position& getNextPos() const {
        return NextPos;
    }

    void setNextPos(int x, int y) {
        NextPos = { x,y };
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

    void setDir(int item) {
        dir_ = item;
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

    int getV() const {
        return V;
    }

    void setFrame(int F) {
        Frame = F;
    }

    void FrameAdd() {
        Frame++;
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
};

// ========== 监听层 ===========
class Msg {
private:
    struct keySta {
        std::chrono::steady_clock::time_point pressTime;//按下时间ms
        bool pressSta = { true };//T短按状态,F长按状态
        bool pressPls = { true };//T短按脉冲
        bool keyUp = { false };//松开标识（清理标识）
    };

    std::unordered_map<int, keySta> keys;//键盘按下情况表
    bool leftButtonDown_ = false;//鼠标按下情况

    ExMessage msg_;
    POINT mousePos_;

public:
    void GetMsg() {
        while (peekmessage(&msg_, EX_MOUSE | EX_KEY)) {
            if (msg_.message == WM_KEYDOWN) {
                if (msg_.vkcode >= 0 && msg_.vkcode < 256) {
                    auto time = getPressTime(msg_.vkcode);
                    if (time == 0) {
                        keys[msg_.vkcode] = { std::chrono::steady_clock::now(), true, true, false };//键盘按下
                    }
                    else if (time >= LOW_DOWN_TIME) {
                        keys.find(msg_.vkcode)->second.pressSta = false; // 长按状态
                    }
                }
            }
            else if (msg_.message == WM_KEYUP) {
                if (msg_.vkcode >= 0 && msg_.vkcode < 256) {
                    auto time = getPressTime(msg_.vkcode);
                    if (time != 0) {
                        if (time < LOW_DOWN_TIME) {
                            keys.find(msg_.vkcode)->second.pressSta = true; // 短按状态
                        }
                        keys.find(msg_.vkcode)->second.keyUp = true; // 松开
                    }
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
    //获取按下时间
    long long getPressTime(int key) {
        auto it = keys.find(key);
        if (it != keys.end()) {
            auto now = std::chrono::steady_clock::now();//当下的时间戳
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.pressTime).count();//计算按下时长
            return duration;
        }
        else {
            return 0;
        }
    }

    //消费短按脉冲
    bool pressPlsS(int key) {
        if (key > 0 && key <= 256) {
            auto it = keys.find(key);
            if (it != keys.end() && it->second.pressPls) {//能找到并且存在脉冲
                it->second.pressPls = false;
                return true;
            }
        }
        return false;
    }

    //键盘按下情况相关//0为没按 1为短按 2为长按
    int getKeySta(int keyCode) const {
        if (keyCode > 0 && keyCode <= 256) {
            auto it = keys.find(keyCode);
            if (it != keys.end()) {//能找到
                return (it->second.pressSta ? 1 : 2);
            }
        }
        return 0;
    }

    void clearKey() {
        keys.clear();
    }

    void updataKey() {//每帧更新
        for (auto it = keys.begin(); it != keys.end();) {
            if (it->second.keyUp) {
                it = keys.erase(it);   // 删除后迭代器自动指向下一个
            }
            else {
                ++it;
            }
        }
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
        Player p;
        p.initPlayerData(x, y, dir);
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
        if (nx < -9 || ny < -9 || nx>9 || ny>9 || ItemMapData == 4) {//头胀头youbug
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

    COLORREF colors[9] = {//124, 229, 255 淡蓝蓝
        //0路 1蛇 2果子 4蛇头 5原点
        RGB(255,255,255),  // 白0
        RGB(0, 255, 0),    // 绿1
        RGB(255, 255, 0),  // 黄2
        RGB(0, 243, 255),  // 蓝3//0, 143, 255//100, 100, 240
        RGB(255, 0, 0),    // 红4
        RGB(128, 0, 255), // 紫5
        RGB(240, 240, 240),//灰6
        RGB(80, 80 ,230), //深灰7
        RGB(255, 128, 0)  // 橙8
    };

public:
    void renderDeath(Context& ctx) {
        //printf("Frame_/RenderCnt_: %d / %d\n", Frame_, RenderCnt_);

        BeginBatchDraw();   // 开始批量绘图（双缓冲）

        if (Frame_ >= SNAKE_UPDATE) {
            Frame_ -= SNAKE_UPDATE;

            setfillcolor(WHITE);
            solidrectangle(75, 75, 645, 650);

            // 画棋盘表格底层阴影
            setlinecolor(colors[6]);             // 线条颜色
            setlinestyle(PS_SOLID, 5);     // 线型：实线，宽度3像素
            for (int i = 0; i < 20; i++) {//30,75
                line(78, 78 + 30 * i, 648, 78 + 30 * i);
                line(78 + 30 * i, 78, 78 + 30 * i, 648);
            }

            //打印原点
            Print(0, 0, 8);

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

            //蓝框
            setlinecolor(colors[7]);             // 线条颜色
            setlinestyle(PS_SOLID, 2);     // 线型：实线，宽度3像素
            for (int i = 0; i < 20; i++) {
                line(75, 75 + 30 * i, 645, 75 + 30 * i);
                line(75 + 30 * i, 75, 75 + 30 * i, 645);
            }
            setlinecolor(BLUE);             // 线条颜色
            setlinestyle(PS_SOLID, 4);     // 线型：实线，宽度3像素
            line(75, 75, 75, 645);
            line(645, 75, 645, 645);
            line(75, 75, 645, 75);
            line(75, 645, 645, 645);

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

    void initRenderDeath(const bool(&Death)[2]) {
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

        // 画棋盘表格底层阴影
        setlinecolor(colors[6]);             // 线条颜色
        setlinestyle(PS_SOLID, 5);     // 线型：实线，宽度3像素
        for (int i = 0; i < 20; i++) {//30,75
            line(78, 78 + 30 * i, 648, 78 + 30 * i);
            line(78 + 30 * i, 78, 78 + 30 * i, 648);
        }

        //打印原点
        Print(0, 0, 8);

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

        //蓝框
        setlinecolor(colors[7]);             // 线条颜色
        setlinestyle(PS_SOLID, 2);     // 线型：实线，宽度3像素
        for (int i = 0; i < 20; i++) {
            line(75, 75 + 30 * i, 645, 75 + 30 * i);
            line(75 + 30 * i, 75, 75 + 30 * i, 645);
        }
        setlinecolor(BLUE);             // 线条颜色
        setlinestyle(PS_SOLID, 4);     // 线型：实线，宽度3像素
        line(75, 75, 75, 645);
        line(645, 75, 645, 645);
        line(75, 75, 645, 75);
        line(75, 645, 645, 645);

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
                1000 / ctx.logic.getPlayer(0).getV(), ctx.msg.getPressTime(VK_SPACE));
            outtextxy(800, 340, buffer);
        }
        else if (ctx.stata.getStaID() == STA_AT_GAME) {
            swprintf(buffer, 256, L"当前速度：%d / %d / %lld / %lld                                                                      ",
                1000 / ctx.logic.getPlayer(0).getV(), 1000 / ctx.logic.getPlayer(1).getV(), ctx.msg.getPressTime(VK_SPACE), ctx.msg.getPressTime(VK_RETURN));
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

    void renderLobby(Context& ctx) {
        BeginBatchDraw();   // 开始批量绘图（双缓冲）

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
        //文字
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);
        outtextxy(510, 518, L"开始游戏");
        setbkmode(OPAQUE);//TRANSPARENT

        EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
    }

    void renderSelect(Context& ctx) {
        BeginBatchDraw();   // 开始批量绘图（双缓冲）

        cleardevice();
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
        //文字
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);
        outtextxy(395, 418, L"极限");
        outtextxy(525, 418, L"竞速");
        outtextxy(655, 418, L"普通");
        outtextxy(510, 310, L"选择模式");
        setbkmode(OPAQUE);//TRANSPARENT

        EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
    }

    void renderPuase(Context& ctx) {
        BeginBatchDraw();   // 开始批量绘图（双缓冲）

        //cleardevice();
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
        //文字
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);
        outtextxy(380, 418, L"返回开始");
        outtextxy(510, 418, L"重新开始");
        outtextxy(640, 418, L"继续游戏");
        outtextxy(527, 310, L"暂停");
        setbkmode(OPAQUE);//TRANSPARENT

        EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
    }

    void renderOver(Context& ctx) {
        BeginBatchDraw();   // 开始批量绘图（双缓冲）

        //cleardevice();
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
        //文字
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);
        outtextxy(380, 418, L"返回开始");
        outtextxy(640, 418, L"重新开始");
        outtextxy(500, 310, L"GAME OVER !!");
        setbkmode(OPAQUE);//TRANSPARENT

        EndBatchDraw();     // 结束批量绘图，一次性显示所有内容
    }

    /*
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
        */

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



//暂停
class Pause :public IGameState {
private:
    int ID = STA_PAUSE;

public:
    void onEnter(Context& ctx)override {
        //绘画暂停按键
        ctx.renderer.renderPuase(ctx);
        ctx.gameTime.pause();
    }

    void onExit(Context& ctx)override {}

    void tick(Context& ctx)override {
        //暂停界面
        if (ctx.msg.getMouse()) {
            POINT pt = ctx.msg.getMousePos();
            if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
                //左
                //返回大厅
                ctx.logic.clearPlayerList();
                for (int i = 0; i < 3; i++) {
                    ctx.stata.popSta(ctx);
                }
                ctx.renderer.renderLobby(ctx);
                printf("左\n");
            }
            if (pt.x >= Marea.left && pt.x <= Marea.right && pt.y >= Marea.top && pt.y <= Marea.bottom) {
                //中
                //重开（我应该使用StaID？）
                ctx.stata.popSta(ctx);
                ctx.logic.clearPlayerList();
                if (auto* cur = ctx.stata.getState()) cur->onEnter(ctx);//初始化
                //ctx.stata.changeSta(ctx, std::make_unique<Lobby>());
                printf("中\n");
            }
            if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
                //右
                //继续游戏
                ctx.stata.popSta(ctx);
                ctx.gameTime.resume();
                ctx.renderer.renderGame(ctx);
                printf("右\n");
            }
        }

        //ESC
        /*
        ESCpop(ctx);
        */
        // 处理 ESC 暂停（通用）
        if (ctx.msg.pressPlsS(VK_ESCAPE)) {
            printf("键盘按下，Sta=%d\n", ctx.stata.getStaID());
            ctx.stata.popSta(ctx);
            ctx.gameTime.resume();
        }
    }

    int getID() const override {
        return ID;
    }
};

//结算
class Over :public IGameState {
private:
    int ID = STA_OVER;

public:
    void onEnter(Context& ctx)override {
        //绘画暂停按键
        ctx.renderer.renderOver(ctx);

        ctx.gameTime.pause();
        ctx.msg.clearKey();
    }

    void onExit(Context& ctx)override {}

    void tick(Context& ctx)override {
        //游戏结束界面
        if (ctx.msg.getMouse()) {
            POINT pt = ctx.msg.getMousePos();
            if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
                //左
                //返回大厅
                ctx.logic.clearPlayerList();
                returnToLobby(ctx);
                ctx.renderer.renderLobby(ctx);
                printf("左\n");
            }
            else if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
                //右
                //重开（我应该使用StaID？）
                ctx.stata.popSta(ctx);
                ctx.logic.clearPlayerList();
                if (auto* cur = ctx.stata.getState()) cur->onEnter(ctx);//初始化
                printf("右\n");
            }
        }
    }

    int getID() const override {
        return ID;
    }
};

//死亡动画
class Death :public IGameState {
private:
    int ID = STA_DEATH;

public:
    void onEnter(Context& ctx)override {
        bool Death[2] = { false };
        const std::vector<Player>& players = ctx.logic.getPlayerList();
        int playerID = 0;
        for (const Player& player : players) {// 遍历所有玩家进行绘制
            Death[playerID] = !player.getLive();
            playerID++;
        }
        ctx.renderer.initRenderDeath(Death);
    }

    void onExit(Context& ctx)override {
        /*
        if (ctx.stata.getStaID() == STA_PU_GAME || ctx.stata.getStaID() == STA_EX_GAME) {
            ctx.stata.pushSta(ctx, std::make_unique<Over>());
        }
        else if (ctx.stata.getStaID() == STA_AT_GAME) {
            ctx.stata.pushSta(ctx, std::make_unique<Over>());
        }
        */
    }

    void tick(Context& ctx)override {
        if ((!(ctx.msg.getKeySta(VK_ESCAPE) >= 1)) && ctx.renderer.getRenderCnt() > 0) {
            ctx.renderer.renderDeath(ctx);
        }
        else {
            ctx.stata.changeSta(ctx, std::make_unique<Over>());
        }
    }

    int getID() const override {
        return ID;
    }
};


//========== 游戏基 ==========
class BaseGame :public IGameState {
public:
    // 每个子类必须返回自己的模式ID（用于渲染、速度计算等）
    virtual int getGameModeID() const = 0;

    // 返回该模式的玩家数量（1 或 2）
    virtual int getPlayerCount() const = 0;

    // 初始化玩家出生点（在 onEnter 中调用）
    virtual void initPlayers(Context& ctx) = 0;

    // 处理本模式特有的输入（如极限模式的镜像切换）
    virtual void ModeInput(Context& ctx) = 0;

    // 当玩家死亡时，决定切换到哪个结算状态（Over 或 Account）
    virtual void onGameOver(Context& ctx) = 0;

    void onEnter(Context& ctx) override final {
        clearPastData(ctx);                 // 通用清理
        initPlayers(ctx);                   // 子类提供初始位置
        ctx.logic.initMapData(getGameModeID()); // 初始化地图
        ctx.renderer.renderGame(ctx);       // 渲染一帧
        ctx.gameTime.start();               // 开始计时
    }

    void onExit(Context& ctx) override final {
        //ctx.gameTime.start();               // 暂停计时？？
    }

    void tick(Context& ctx) override final {
        ctx.gameTime.updata();

        // 处理 ESC 暂停（通用）
        if (ctx.msg.pressPlsS(VK_ESCAPE)) {
            ctx.stata.pushSta(ctx, std::make_unique<Pause>());
            return;
        }

        // 调用子类特有的输入处理（如镜像模式）
        ModeInput(ctx);

        // 对所有玩家增加帧计数
        for (int i = 0; i < getPlayerCount(); ++i) {
            ctx.logic.getPlayer(i).FrameAdd();
        }

        // 收集需要移动的玩家
        std::vector<int> toMove;
        for (int i = 0; i < getPlayerCount(); ++i) {
            if (ctx.logic.getPlayer(i).getV() <= ctx.logic.getPlayer(i).getFrame()) {
                toMove.push_back(i);
            }
        }

        // 移动前预测碰撞（judgeMoveRequest）
        for (int i : toMove) {
            ctx.logic.getPlayer(i).setFrame(0);
            ctx.logic.judgeMoveRequest(ctx, ctx.logic.getPlayer(i));
        }

        // 检查是否有玩家死亡
        int loser = ctx.logic.getLoser(ctx);
        if (loser != 0) {
            onGameOver(ctx);   // 子类决定进入 Death 还是直接结算
            return;
        }

        // 真正更新移动数据
        for (int i : toMove) {
            ctx.logic.updataData(ctx, ctx.logic.getPlayer(i));
        }

        // 渲染
        ctx.renderer.renderGame(ctx);
    }

    // 基类提供通用的 getID（返回游戏模式ID）
    int getID() const override final {
        return getGameModeID();
    }
};

//PU
class PUGame :public BaseGame {
public:
    int getGameModeID() const override {
        return STA_PU_GAME;
    }

    int getPlayerCount() const override {
        return 1;
    }

    void initPlayers(Context& ctx) override {
        ctx.logic.addPlayer(0, 0, 0);//队列初始化
    }

    void ModeInput(Context& ctx) override {
        ctx.logic.getPlayer(0).setV(ctx.msg.getPressTime(VK_SPACE));
        MoveMsg(ctx, 0, 0);
    }

    void onGameOver(Context& ctx) override {
        ctx.stata.pushSta(ctx, std::make_unique<Death>());
    }
};

//AT
class ATGame :public BaseGame {
public:
    int getGameModeID() const override {
        return STA_AT_GAME;
    }

    int getPlayerCount() const override {
        return 2;
    }

    void initPlayers(Context& ctx) override {
        ctx.logic.addPlayer(-5, 0, 0);//队列初始化
        ctx.logic.addPlayer(5, 0, 0);
    }

    void ModeInput(Context& ctx) override {
        ctx.logic.getPlayer(0).setV(ctx.msg.getPressTime(VK_SPACE));
        ctx.logic.getPlayer(1).setV(ctx.msg.getPressTime(VK_RETURN));
        MoveMsg(ctx, 0, 1);
    }

    void onGameOver(Context& ctx) override {
        ctx.stata.pushSta(ctx, std::make_unique<Death>());
    }
};

//EX
class EXGame :public BaseGame {
public:
    int getGameModeID() const override {
        return STA_EX_GAME;
    }

    int getPlayerCount() const override {
        return 2;
    }

    void initPlayers(Context& ctx) override {
        ctx.logic.addPlayer(5, -5, 0);//队列初始化
        ctx.logic.addPlayer(-5, 5, 2);
    }

    void ModeInput(Context& ctx) override {
        if (ctx.msg.pressPlsS(VK_SPACE))ctx.logic.Mirror();//空格
        bool mirror = ctx.logic.getMirror(); // 注意：原代码中用 mirror 变量，但 Logic 中 mirror 是私有，需要提供 getMirror() 已存在
        int mainIdx = mirror ? 1 : 0;
        int subIdx = 1 - mainIdx;
        MirrorMoveMsg(ctx, mainIdx, subIdx);
    }

    void onGameOver(Context& ctx) override {
        ctx.stata.pushSta(ctx, std::make_unique<Death>());
    }
};




//选择难度
class Select :public IGameState {
private:
    int ID = STA_SELECT;

public:
    void onEnter(Context& ctx)override {
        //绘画难度选择按键
        ctx.renderer.renderSelect(ctx);
    }

    void onExit(Context& ctx)override {}

    void tick(Context& ctx)override {
        //大厅检测
        if (ctx.msg.getMouse()) {
            POINT pt = ctx.msg.getMousePos();
            if (pt.x >= Larea.left && pt.x <= Larea.right && pt.y >= Larea.top && pt.y <= Larea.bottom) {
                //左
                //极限模式
                ctx.stata.pushSta(ctx, std::make_unique<EXGame>());
                printf("左\n");
            }
            else if (pt.x >= Marea.left && pt.x <= Marea.right && pt.y >= Marea.top && pt.y <= Marea.bottom) {
                //中
                //竞技模式
                ctx.stata.pushSta(ctx, std::make_unique<ATGame>());
                printf("中\n");
            }
            else if (pt.x >= Rarea.left && pt.x <= Rarea.right && pt.y >= Rarea.top && pt.y <= Rarea.bottom) {
                //右
                //简单模式
                ctx.stata.pushSta(ctx, std::make_unique<PUGame>());
                printf("右\n");
            }
        }

        //ESC
        ESCpop(ctx);
    }

    int getID() const override {
        return ID;
    }
};

//大厅
class Lobby :public IGameState {
private:
    int ID = STA_LOBBY;

public:
    void onEnter(Context& ctx)override {
        //绘画大厅按键
        ctx.renderer.renderLobby(ctx);
    }

    void onExit(Context& ctx)override {}

    void tick(Context& ctx)override {
        //大厅检测
        if (ctx.msg.getMouse()) {
            POINT pt = ctx.msg.getMousePos();
            if (pt.x >= Sarea.left && pt.x <= Sarea.right && pt.y >= Sarea.top && pt.y <= Sarea.bottom) {
                //开始
                ctx.stata.pushSta(ctx, std::make_unique<Select>());
                printf("按下开始按钮\n");
            }
        }

        //ESC
        ESCpop(ctx);
    }

    int getID() const override {
        return ID;
    }
};




// ================== 主函数 =====================
int main()
{
    timeBeginPeriod(1);  // // 提高定时器精度,全局设置，程序结束时记得 timeEndPeriod(1)

    Time gameTime;
    Stata stata;
    Logic logic;
    Msg input;
    Renderer renderer;

    Context ctx{ stata, logic, renderer, input, gameTime };


    //生成画布
    initgraph(1080, 720);
    setbkcolor(WHITE);             //设置背景（白）
    cleardevice();
    //初始化必备数据
    setbkmode(OPAQUE);//TRANSPARENT
    srand((unsigned int)time(NULL));//加载随机数

    ctx.stata.pushSta(ctx, std::make_unique<Lobby>());//初始化栈


    //加载帧率计数器
    const double FRAME_TIME_MS = 1000.0 / FPS_LOGIC;  // 16.666... ms
    using namespace std::chrono;
    auto nextFrameTime = steady_clock::now();//第0帧时间节点


    //程序主循环
    while (TRUE) {

        if (ctx.stata.empty())break;//退出逻辑

        ctx.msg.updataKey();//清空松开的键盘
        ctx.msg.GetMsg();//监听键盘

        if (auto* cur = ctx.stata.getState()) cur->tick(ctx);//运行栈顶


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



//====== 辅助函数 =====
void ESCpop(Context& ctx) {
    if (ctx.msg.pressPlsS(VK_ESCAPE)) {
        ctx.stata.popSta(ctx);
    }
}

void returnToLobby(Context& ctx) {
    while (ctx.stata.getStaDepth() > 1) {
        ctx.stata.popSta(ctx);
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
    if (ctx.msg.getKeySta('W') != 0) ctx.logic.getPlayer(p1).up();
    if (ctx.msg.getKeySta('D') != 0) ctx.logic.getPlayer(p1).right();
    if (ctx.msg.getKeySta('S') != 0) ctx.logic.getPlayer(p1).down();
    if (ctx.msg.getKeySta('A') != 0) ctx.logic.getPlayer(p1).left();

    //p2
    if (ctx.msg.getKeySta(VK_UP) != 0)    ctx.logic.getPlayer(p2).up();
    if (ctx.msg.getKeySta(VK_RIGHT) != 0) ctx.logic.getPlayer(p2).right();
    if (ctx.msg.getKeySta(VK_DOWN) != 0)  ctx.logic.getPlayer(p2).down();
    if (ctx.msg.getKeySta(VK_LEFT) != 0)  ctx.logic.getPlayer(p2).left();
}

void MirrorMoveMsg(Context& ctx, int p1, int p2) {
    if ((ctx.msg.getKeySta('W') != 0) || (ctx.msg.getKeySta(VK_UP) != 0)) { ctx.logic.getPlayer(p1).up();     ctx.logic.getPlayer(p2).down(); }
    if ((ctx.msg.getKeySta('D') != 0) || (ctx.msg.getKeySta(VK_RIGHT) != 0)) { ctx.logic.getPlayer(p1).right();  ctx.logic.getPlayer(p2).left(); }
    if ((ctx.msg.getKeySta('S') != 0) || (ctx.msg.getKeySta(VK_DOWN) != 0)) { ctx.logic.getPlayer(p1).down();   ctx.logic.getPlayer(p2).up(); }
    if ((ctx.msg.getKeySta('A') != 0) || (ctx.msg.getKeySta(VK_LEFT) != 0)) { ctx.logic.getPlayer(p1).left();   ctx.logic.getPlayer(p2).right(); }
}

