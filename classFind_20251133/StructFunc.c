#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "StructFunc.hpp"
#include "Log.hpp"

//添加
struct StuNode* Add(struct StuNode* head, char buffer[256], int cnt) {
    //分配内存
    struct StuNode* newnode = NULL;
    newnode = (struct StuNode*)malloc(sizeof(struct StuNode));
    if (newnode == NULL) {
        printf("申请内存失败\n");
        writeLog("内存申请失败");
        return head;
    }
    newnode->next = NULL;

    //拆开缓冲区，逐个写入
    char* token = strtok(buffer, "\t");
    int tokenStr = 0;   //第几份数据
    while (token != NULL) {
        switch (tokenStr) {
        case 0:
            newnode->num = atoi(token);
            break;
        case 1:
            strcpy(newnode->name, token);
            break;
        case 2:
            strcpy(newnode->NumDorm, token);
            break;
        case 3:
            strcpy(newnode->NumPhone, token);
            break;
        case 4:
            strcpy(newnode->NumCard, token);
            break;
        }
        tokenStr++;
        token = strtok(NULL, "\t");
    }
    if (tokenStr != 5 && cnt==-2) {   //=1添加链尾，-2读取
        printf("读取失败！在读取序列%d时失败\n", newnode->num);
        writeLog("文件读取失败");
        return head;
    }
    else if (tokenStr != 5 && cnt != -2) {
        printf("添加失败！\n");
        writeLog("添加失败");
        return head;
    }

    //printf("读取成功！\n");

    //先判断创建（head==0）
    if (head == NULL) {
        writeLog("创建头节点");
        return newnode;   //直接返回新节点地址做head
    }

    struct StuNode* curr = head; //当前节点
    struct StuNode* prev = NULL;//上一节点

    //链接
    if (cnt == -1 || cnt == -2) {  //创建
        //查找到链尾
        while (curr->next != NULL) {
            prev = curr;
            curr = curr->next;
        }
        curr->next = newnode;
        if (cnt != -2) {
            writeLog("创建到尾部");
        }
        return head;
    }
    else {            //插入
        //插入表头
        if (cnt <= curr->num) {
            newnode->next = head;
            writeLog("链接到头部");
            return newnode;
        }
        else {
            //查找到链尾或序号前
            while (curr->next != NULL && curr->num < cnt) {
                prev = curr;
                curr = curr->next;
            }
            newnode->next = curr;
            prev->next = newnode;
            writeLog("链接到序列？");
            return head;
        }
    }

    printf("添加失败\n");
    free(newnode);
    return head;
}

//删除
struct StuNode* Delete(struct StuNode* head, int num) {
    //空链表
    if (head == NULL) {
        printf("链表不存在！\n");
        return NULL;
    }

    struct StuNode* curr = head; //当前节点
    struct StuNode* prev = NULL;//上一节点

    //查找
    while (curr != NULL && curr->num != num) {
        prev = curr;
        curr = curr->next;
    }

    //不存在
    if (curr == NULL) {
        printf("数据不存在！\n");
        return head;
    }

    //删除
    if (prev == NULL) {  //链头
        head = curr->next;
    }
    else {   //链尾和中间  
        prev->next = curr->next;
    }
    free(curr);
    return head;
}

//修改\打印
void Find(struct StuNode* head, char* ch, int choice) {
    //空链表
    if (head == NULL) {
        printf("链表不存在！\n");
        return;
    }

    struct StuNode* curr = head; //当前节点
    struct StuNode* prev = NULL;//上一节点

    int cnt_bu = 0;
    struct StuNode* res_bu[256] = { NULL };

    //printf("序号\t姓名\t宿舍号\t手机号\t身份证号\n");
    if (choice == -1) {
        //全打印
        printf("序号\t姓名\t宿舍号\t\t手机号\t\t身份证号\n");
        while (curr != NULL) {
            printf("%02d\t%s\t%s\t%s\t%s\n", curr->num, curr->name, curr->NumDorm, curr->NumPhone, curr->NumCard);
            prev = curr;
            curr = curr->next;
        }
        return;
    }
    else {
        if (ch[0] == '#') {
            // 查找名字单项（跳过#号）
            char* search_name = ch + 1;  // 跳过开头的'#'

            while (curr != NULL) {
                // 名字
                if (strstr(curr->name, search_name) != NULL) {
                    res_bu[cnt_bu] = curr;
                    cnt_bu++;
                }
                curr = curr->next;
            }
        }
        else if (ch[0] == '@') {
            // 查找序号单项（跳过@号）
            int num_bu = 0;
            int len = strlen(ch);
            for (int i = 0; i < len; i++) {
                if (ch[i] >= '0' && ch[i] <= '9') {
                    num_bu *= 10;
                    num_bu += ch[i] - '0';
                }
            }
            printf("%d\n", num_bu);
            while (curr != NULL) {
                // 序号
                if (curr->num == num_bu) {
                    res_bu[cnt_bu] = curr;
                    cnt_bu++;
                    break;
                }
                curr = curr->next;
            }
        }
        else {
            //宿舍、手机号、身份证号
            while (curr != NULL) {
                if (strstr(curr->NumDorm, ch) != NULL || strstr(curr->NumPhone, ch) != NULL || strstr(curr->NumCard, ch) != NULL) {
                    res_bu[cnt_bu] = curr;
                    cnt_bu++;
                }
                curr = curr->next;
            }
        }

        if (cnt_bu == 0) {
            printf("没有该数据！\n");
            return;
        }
        else {
            printf("序号\t姓名\t宿舍号\t\t手机号\t\t身份证号\n");
            for (int i = 0; i < cnt_bu; i++) {
                printf("%02d\t%s\t%s\t%s\t%s\n", res_bu[i]->num, res_bu[i]->name, res_bu[i]->NumDorm, res_bu[i]->NumPhone, res_bu[i]->NumCard);
            }
        }
    
        if (choice == 1 && cnt_bu == 1) {
            while (choice != 0) {
                printf("请输入要修改的内容：\n");
                printf("1序号 2名字 3宿舍号 4手机号 5身份证号 0退出\n");

                scanf("%1d", &choice);
                clean_input_buffer();

                if (choice == 0)return;

                printf("请输入修改后的内容：\n");
                switch (choice) {
                    int num = -3;
                    char name[16];
                    char NumDorm[16];
                    char NumPhone[12];
                    char NumCard[19];
                case 1:
                    scanf("%3d", &num);
                    clean_input_buffer();
                    curr->num = num;
                    break;
                case 2:
                    scanf("%15s", &name);
                    clean_input_buffer();
                    strcpy(curr->name, name);
                    break;
                case 3:
                    scanf("%15s", &NumDorm);
                    clean_input_buffer();
                    strcpy(curr->NumDorm, NumDorm);
                    break;
                case 4:
                    scanf("%11s", &NumPhone);
                    clean_input_buffer();
                    strcpy(curr->NumPhone, NumPhone);
                    break;
                case 5:
                    scanf("%18s", &NumCard);
                    clean_input_buffer();
                    strcpy(curr->NumCard, NumCard);
                    break;
                }
                printf("修改成功！\n");
            }
        }
        if (choice == 1 && cnt_bu != 1)printf("非唯一项无法修改！\n");
    }
    return;
}

//保存文件
void saveFile(struct StuNode* head, FILE* dataFile) {
    //空链表
    if (head == NULL) {
        printf("链表不存在！\n");
        return;
    }
    //修改文件打开模式？？？？？
    if (dataFile) {
        writeLog("修改文件打开模式");
        fclose(dataFile);
        dataFile = NULL;
    }
    dataFile = fopen("classdata.txt", "w");
    if (dataFile == NULL) {
        printf("文件打开失败！\n");
        writeLog("文件打开方式修改失败");
        return;
    }

    struct StuNode* curr = head; //当前节点
    struct StuNode* prev = NULL;//上一节点

    //遍历写入
    while (curr != NULL) {
        fprintf(dataFile, "%02d\t%s\t%s\t%s\t%s\n", curr->num, curr->name, curr->NumDorm, curr->NumPhone, curr->NumCard);
        prev = curr;
        curr = curr->next;
    }
    writeLog("文件写入完成");

    //修改文件打开模式
    if (dataFile) {
        writeLog("修改文件打开模式");
        fclose(dataFile);
        dataFile = NULL;
    }
    dataFile = fopen("classdata.txt", "r");
    if (dataFile == NULL) {
        printf("文件打开失败！\n");
        writeLog("文件打开方式修改失败");
        return;
    }
    return;
}

// 清理缓冲区
void clean_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}