#pragma once
#include <cstdio>
#include <cstring>
#include <iostream>
#include <queue>

#define MAXLINELENGTH 1000 /* 机器指令的最大长度 */
#define MEMSIZE 10000      /* 内存的最大容量 */
#define NUMREGS 32         /* 寄存器数量 */

/*
 * 操作码和功能码定义
 */

#define regRegALU 0 /* 寄存器-寄存器的ALU运算的操作码为0 */
#define LW 35
#define SW 43
#define ADDI 8
#define ANDI 12
#define BEQZ 4
#define J 2
#define HALT 1
#define NOOP 3
#define addFunc 32 /* ALU运算的功能码 */
#define subFunc 34
#define andFunc 36

#define NOOPINSTRUCTION 0x0c000000;

/*
 * 执行单元
 */
#define LOAD1 1
#define LOAD2 2
#define STORE1 3
#define STORE2 4
#define INT1 5
#define INT2 6

#define NUMUNITS 6 /* 执行单元数量 */
const char *unitname[NUMUNITS] = {
    "LOAD1",    "LOAD2", "STORE1",
    "STORE2", "INT1",    "INT2"
}; /* 执行单元的名称 */

/*
 * 不同操作所需要的周期数
 */
#define BRANCHEXEC 3 /* 分支操作 */
#define LDEXEC 2     /* Load */
#define STEXEC 2     /* Store */
#define INTEXEC 1    /* 整数运算 */

/*
 * 指令状态
 */
#define ISSUING 0       /* 发射 */
#define EXECUTING 1     /* 执行 */
#define WRITINGRESULT 2 /* 写结果 */
#define COMMITTING 3    /* 提交 */
const char *statename[4] = {
    "ISSUING", "EXECUTING", "WRITINGRESULT", "COMMTITTING"
}; /*        状态名称 */

#define RBSIZE 16 /* ROB有16个单元 */
#define BTBSIZE 8 /* 分支预测缓冲栈有8个单元 */

/*
 * 2 bit 分支预测状态
 */
#define STRONGNOT 0
#define WEAKTAKEN 1
#define WEAKNOT 2
#define STRONGTAKEN 3

/*
 * 分支跳转结果
 */
#define NOTTAKEN 0
#define TAKEN 1

struct resStation { /* 保留栈的数据结构 */
    int instr;      /* 指令 */
    int busy;       /* 空闲标志位 */
    int Vj;         /* Vj, Vk 存放操作数 */
    int Vk;
    int Qj;         /* Qj, Qk 存放将会生成结果的执行单元编号 */
    int Qk;         /* 为零则表示对应的V有效 */
    int exTimeLeft; /* 指令执行的剩余时间 */
    int reorderNum; /* 该指令对应的ROB项编号 */
};

struct reorderEntry { /* ROB项的数据结构 */
    int busy;         /* 空闲标志位 */
    int instr;        /* 指令 */
    int execUnit;     /* 执行单元编号 */
    int instrStatus;  /* 指令的当前状态 */
    int valid;        /* 表明结果是否有效的标志位 */
    int result;       /* 在提交之前临时存放结果 */
    int storeAddress; /* store指令的内存地址 */
    int pc;
    int prediction;
};

struct regResultEntry { /* 寄存器状态的数据结构 */
    int valid;          /* 1表示寄存器值有效, 否则0 */
    int reorderNum;     /* 如果值无效, 记录ROB中哪个项目会提交结果 */
};

struct btbEntry {     /* 分支预测缓冲栈的数据结构 */
    int valid;        /* 有效位 */
    int branchPC;     /* 分支指令的PC值 */
    int branchTarget; /* when predict taken, update PC with target */
    int branchPred;   /* 预测：2-bit分支历史 */
};
