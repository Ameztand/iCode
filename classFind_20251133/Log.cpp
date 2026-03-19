#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Log.hpp"

static FILE* logFile = NULL;

//日志初始化
int initLog() {
    logFile = fopen("classlog.txt", "a");
    if (logFile == NULL) {
        printf("日志系统初始化失败！\n");
        return 1;
    }
    writeLog("日志初始化完成");
    return 0;
}

// 日志记录函数
void writeLog(const char* message) {
    if (logFile == NULL) return;

    time_t now;
    struct tm* timeinfo;
    char timeStr[100];

    time(&now);
    timeinfo = localtime(&now);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d  %H:%M:%S", timeinfo);

    fprintf(logFile, "[%s] %s\n", timeStr, message);
    fflush(logFile); // 立即写入磁盘，避免数据丢失
}

// 关闭日志
void closeLog() {
    if (logFile) {
        writeLog("日志关闭\n");
        fclose(logFile);
        logFile = NULL;
    }
}