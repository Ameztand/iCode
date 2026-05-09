
#include "logic.h"
#include "gameState.h"



// ========== 玩家管理器 ===========

//初始化
void Player::initPlayerData(int x, int y, int dir) {

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
const Position& Player::getNextPos() const {
    return NextPos;
}

void Player::setNextPos(int x, int y) {
    NextPos = { x,y };
}

//存活状态相关
bool Player::getLive() const {
    return Live_;
}

void Player::setLive(bool item) {
    Live_ = item;
}

//移动输入方面
void Player::up() {
    if (purrDir_ != 2) dir_ = 0;
}

void Player::right() {
    if (purrDir_ != 3) dir_ = 1;
}

void Player::down() {
    if (purrDir_ != 0) dir_ = 2;
}

void Player::left() {
    if (purrDir_ != 1) dir_ = 3;
}

void Player::setDir(int item) {
    dir_ = item;
}

void Player::updataPurrDir() {//防止向后转
    purrDir_ = dir_;
}

int Player::getDir() const {
    return dir_;
}

//队列方面
void Player::pushQ(int x, int y) {
    posQueue.push({ x,y });
}

void Player::popQ() {
    posQueue.pop();
}

void Player::clearQ() {
    while (!posQueue.empty()) {
        posQueue.pop();
    }
}

bool Player::emptyQ() {
    return posQueue.empty();
}

size_t Player::sizeQ() const {
    return posQueue.size();
}

const Position& Player::getFront()const {
    return posQueue.front();
}

const Position& Player::getBack()const {
    return posQueue.back();
}

const std::queue<Position>& Player::getQueue()const {
    return posQueue;
}

//速度方面
void Player::setV(long long item) {
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

int Player::getV() const {
    return V;
}

void Player::setFrame(int F) {
    Frame = F;
}

void Player::FrameAdd() {
    Frame++;
}

int Player::getFrame() const {
    return Frame;
}

//能量方面
int Player::getE() const {
    return E;
}

void Player::setE(int item) {
    E = item;
}

// ========== 逻辑层===========
//极限模式控制主体相关
void Logic:: Mirror() {
    mirror = mirror == 0 ? 1 : 0;
}

int Logic::getMirror()const {
    return mirror;
}

void Logic::initMirror() {
    mirror = 0;
}

//玩家列表相关
void Logic::addPlayer(int x, int y, int dir) {
    Player p;
    p.initPlayerData(x, y, dir);
    PlayerList.push_back(p);
}

void Logic::clearPlayerList() {
    while (!PlayerList.empty()) {
        PlayerList.pop_back();
    }
}

const std::vector<Player>& Logic::getPlayerList() const {
    return PlayerList;
}

Player& Logic::getPlayer(size_t index) {
    return PlayerList.at(index);
}

const Player& Logic::getPlayer(size_t index) const {
    return PlayerList.at(index);
}

//地图相关
int Logic::getMapData(int x, int y) const {//输入坐标 返回状态
    return MapData[x + 9][-y + 9];
}

void Logic::clearMapData() {
    for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 19; j++) {
            MapData[i][j] = 0;
        }
    }
}

void Logic::initMapData(int mode) {
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
void Logic::initDotData() {
    for (int i = 0; i < 3; i++) {
        DotData[i] = { 10,10 };
        CreatDot(0, 0, i);
    }
}

const Position& Logic::getDotData(int i) const {//输入坐标 返回状态
    return DotData[i];
}

void Logic::CreatDot(int nx, int ny, int i) {//生成一个果子
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

void Logic::clearDot(int i) {
    DotData[i] = { 10,10 };
}

//判断移动请求
void Logic::judgeMoveRequest(Context& ctx, Player& player) {
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
void Logic::updataData(Context& ctx, Player& player) {
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

    if (ctx.state.getStaID() == STA_EX_GAME) {
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
int Logic::getLoser(Context& ctx) {
    //头撞头判定
    if (ctx.state.getStaID() == STA_AT_GAME || ctx.state.getStaID() == STA_EX_GAME) {
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

int Logic::Random() {
    return rand() % 10;
}