#include "defines.hpp"

int convertNum16(int num) {
    /* convert an 16 bit number into a 32-bit or 64-bit number */
    if (num & 0x8000) {
        num -= 65536;
    }
    return (num);
}

int convertNum26(int num) {
    /* convert an 26 bit number into a 32-bit or 64-bit number */
    if (num & 0x200000) {
        num -= 67108864;
    }
    return (num);
}

/*
 * 这里对指令进行解码，转换成程序可以识别的格式，需要根据指令格式来进行。
 * 可以考虑使用高级语言中的位和逻辑运算
 */
int field0(int instruction) {
    /*
     *返回指令的第一个寄存器RS1
     */
    return instruction >> 21 & 31;
}

int field1(int instruction) {
    /*
     *返回指令的第二个寄存器，RS2或者Rd
     */
    return instruction >> 16 & 31;
}

int field2(int instruction) {
    /*
     *返回指令的第三个寄存器，Rd
     */
    return instruction >> 11 & 31;
}

int immediate(int instruction) {
    /*
     *返回I型指令的立即数部分
     */
    return convertNum16(instruction & 65535);
}

int jumpAddr(int instruction) {
    /*
     *返回J型指令的跳转地址
     */
    return convertNum26(instruction & 67108863);
}

int opcode(int instruction) {
    /*
     *返回指令的操作码
     */
    return instruction >> 26 & 63;
}

int func(int instruction) {
    /*
     *返回R型指令的功能域
     */
    return instruction & 2047;
}

bool isR(int instruction) {
    return opcode(instruction) == regRegALU;
}

bool isI(int instruction) {
    return opcode(instruction) == SW
        || opcode(instruction) == LW
        || opcode(instruction) == ADDI
        || opcode(instruction) == ANDI
        || opcode(instruction) == BEQZ;
}

bool isL(int instruction) {
    return opcode(instruction) == LW;
}

bool isS(int instruction) {
    return opcode(instruction) == SW;
}

bool isB(int instruction) {
    return opcode(instruction) == BEQZ;
}

bool isJ(int instruction) {
    return opcode(instruction) == J;
}

int Next(int RB) {
    return RB + 1 >= RBSIZE ? RB + 1 - RBSIZE : RB + 1;
}