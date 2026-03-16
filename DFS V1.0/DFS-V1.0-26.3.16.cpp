/*
这是是我自己琢磨出来的DFS寻路算法，不算高效，并且还有一部分bug，思路也有些混乱
整体框架已经搭建好，填充色块上可以加更多的if语句

思路上，先调用Direction函数判断方向，然后一直向前走，直到正前方无路，然后再次判
断方向（改向），直到走到死胡同（Direction函数返回-1）然后开始回溯，回溯每次都会
判断方向，直到找到岔口，回到main函数是要len++，否则会出错（少记录一个格子）这是
值得修改的方向寻路
以及，判断终点的方式是要走到死胡同才判断，这是一个隐患。

后续可能会加入从文件读取地图，设置起点终点和地图大小等功能。不过当下最紧要的是优
化核心寻路算法
*/

#include<stdio.h>
#include <graphics.h>
#include <conio.h>
#include <windows.h>  // 为了 Sleep

void initEaxyX();
void Print(int x, int y, int sign);
int Recall(int x, int y);
int Direction(int x, int y);

//恒量表
int X = 1100, Y = 20, V = 100, len = 0;//V是速度调节，值越大越慢

int dx[4] = { 0,1,0,-1 };//上右下左
int dy[4] = { -1,0,1,0 };

int inn[100][2];
int buMap[10][10];
int mapData[10][10] = {  //0为路1为墙
        {0,1,0,0,0,1,0,0,0,1},
        {0,0,0,1,0,1,1,1,0,1},
        {0,1,0,1,0,1,0,0,0,1},
        {0,1,0,1,0,1,0,1,0,1},
        {0,1,0,1,0,0,0,1,0,1},
        {0,0,0,1,0,1,1,1,0,1},
        {0,1,0,1,0,1,0,1,0,1},
        {0,1,0,1,0,1,0,0,0,1},
        {0,1,0,1,0,0,0,1,0,1},
        {0,1,0,0,0,1,0,0,0,0}
};


int main()
{
    //加载图像
    initEaxyX();

    //寻路主循环
    int x = 0, y = 0;//当前格子
    int nx, ny;      //检测格子
    int ii = 0;      //方向参数
    while (1) {
        //方向参数
        ii = Direction(x, y);
        printf("当前方向：%d\n", ii);

        //回溯
        if (ii == -1) {
            ii = Recall(x, y);//返回上一个岔口的方向参数
            //取栈
            x = inn[len][0];
            y = inn[len][1];
            len++;//很关键
            Print(x, y, 1);
            printf("回溯至(%d,%d)当前方向参数%d\n", x, y, ii);
        }

        //向前走循环
        nx = x + dx[ii];
        ny = y + dy[ii];
        while (nx >= 0 && ny >= 0 && nx <= 9 && ny <= 9 && buMap[ny][nx] == 0) {
            //移动中心点
            x = nx;
            y = ny;

            //打印
            Print(x, y, 1);

            //入栈
            inn[len][0] = x;
            inn[len][1] = y;
            len++;

            //检测点向前移动一格
            nx = x + dx[ii];
            ny = y + dy[ii];

            Sleep(V);
        }

        printf("走到尽头\n");

        //结束判断
        if (x == 9 && y == 9) {
            setfillcolor(YELLOW);
            solidrectangle(569, 569, 618, 618);
            break;
        }
    }

    for (int i = 0; i < len; i++) {
        printf("(%d,%d)\n", inn[i][0], inn[i][1]);
    }

    _getch();
    closegraph();
    return 0;
}

void initEaxyX()
{
    initgraph(1280, 720);          //生成画布
    setbkcolor(WHITE);             //设置背景（白）
    cleardevice();                 //清空，填充背景色

    // 画表格
    setlinecolor(BLUE);             // 线条颜色
    setlinestyle(PS_SOLID, 3);     // 线型：实线，宽度3像素

    for (int i = 0; i <= 10; i++) {
        moveto(100, 100 + 52 * i);
        lineto(620, 100 + 52 * i);
    }
    for (int i = 0; i <= 10; i++) {
        moveto(100 + 52 * i, 100);
        lineto(100 + 52 * i, 620);
    }

    //显示墙
    settextcolor(BLACK);
    outtextxy(100, 80, L"Hello, EasyX!");

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (mapData[i][j] == 1) {
                outtextxy(122 + 52 * j, 120 + 52 * i, L"#");
            }
        }
    }

    //起点与路径
    setfillcolor(GREEN);
    /*
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (mapData[i][j] == 1) {
                solidrectangle(101 + 52 * i, 101 + 52 * j, 150 + 52 * i, 150 + 52 * j);
            }
        }
    }
    */
    setfillcolor(RED);
    solidrectangle(101, 101, 150, 150);
    setfillcolor(YELLOW);
    solidrectangle(569, 569, 618, 618);

    //复制地图
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            buMap[i][j] = mapData[i][j];
        }
    }
    buMap[0][0] == 2;//2就是第一次走过的路
}

void Print(int x, int y, int sign) {
    //打印画布右侧
    wchar_t buffer[100];
    if (sign == 1) {
        swprintf(buffer, 100, L"坐标: (%d, %d)", x, y);
        setfillcolor(GREEN);//设置填充色块颜色
    }
    else {
        swprintf(buffer, 100, L"                        ");
        setfillcolor(CYAN);
    }
    outtextxy(X, Y + 20 * len, buffer);
    solidrectangle(101 + 52 * x, 101 + 52 * y, 150 + 52 * x, 150 + 52 * y);
    //更新地图
    buMap[y][x] += 2;
}

int Recall(int x, int y) {
    printf("当前(%d,%d)开始回溯\n", x, y);

    //打印中心格子
    Print(x, y, 0);

    //出栈
    len--;
    x = inn[len][0];
    y = inn[len][1];

    Sleep(V);

    int ii = Direction(x, y);
    if (ii == -1) {
        return Recall(x, y);
    }
    return ii;
}

int Direction(int x, int y) {
    printf("开始遍历方向参数(%d,%d)\n", x, y);

    int nx, ny;
    for (int i = 0; i < 4; i++) {
        nx = x + dx[i];
        ny = y + dy[i];
        printf("(%d,%d)", nx, ny);
        if (nx >= 0 && ny >= 0 && nx <= 9 && ny <= 9 && buMap[ny][nx] == 0) {
            printf("jem\n");
            return i;
        }
        printf("none\n");
    }
    return -1;//死路
}

