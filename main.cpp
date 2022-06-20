#include "machinestate.hpp"

void printInstruction(int instr) {
    char opcodeString[10];
    char funcString[11];
    int funcCode;
    int op;

    if (opcode(instr) == regRegALU) {
        funcCode = func(instr);
        if (funcCode == addFunc) {
            strcpy(opcodeString, "add");
        } else if (funcCode == subFunc) {
            strcpy(opcodeString, "sub");
        } else if (funcCode == andFunc) {
            strcpy(opcodeString, "and");
        } else {
            strcpy(opcodeString, "alu");
        }
        printf("%s %d %d %d \n", opcodeString, field0(instr), field1(instr),
                     field2(instr));
    } else if (opcode(instr) == LW) {
        strcpy(opcodeString, "lw");
        printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
                     immediate(instr));
    } else if (opcode(instr) == SW) {
        strcpy(opcodeString, "sw");
        printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
                     immediate(instr));
    } else if (opcode(instr) == ADDI) {
        strcpy(opcodeString, "addi");
        printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
                     immediate(instr));
    } else if (opcode(instr) == ANDI) {
        strcpy(opcodeString, "andi");
        printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
                     immediate(instr));
    } else if (opcode(instr) == BEQZ) {
        strcpy(opcodeString, "beqz");
        printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
                     immediate(instr));
    } else if (opcode(instr) == J) {
        strcpy(opcodeString, "j");
        printf("%s %d\n", opcodeString, jumpAddr(instr));
    } else if (opcode(instr) == HALT) {
        strcpy(opcodeString, "halt");
        printf("%s\n", opcodeString);
    } else if (opcode(instr) == NOOP) {
        strcpy(opcodeString, "noop");
        printf("%s\n", opcodeString);
    } else {
        strcpy(opcodeString, "data");
        printf("%s %d\n", opcodeString, instr);
    }
}

machineState::machineState() {
    /*
     * 分配数据结构空间
     */
    for (int i = 0; i < MEMSIZE; ++i) {
        memory[i] = 0;
    }

    /*
     * 状态初始化
     */
    halt = 0;
    pc = 16;
    cycles = 0;
    for (int i = 0; i < NUMREGS; ++i) {
        regFile[i] = 0;
    }
    for (int i = 0; i < NUMUNITS; ++i) {
        reservation[i].busy = 0;
    }
    for (int i = 0; i < RBSIZE; i++) {
        reorderBuf[i].busy = 0;
    }

    headRB = -1;
    tailRB = -1;

    for (int i = 0; i < NUMREGS; ++i) {
        regResult[i].valid = 1;
    }
    for (int i = 0; i < BTBSIZE; ++i) {
        btBuf[i].valid = 0;
    }
}

void machineState::read(FILE *filePtr) {
    /*
     * 将机器指令读入到内存中
     */
    int read_pc = 16, instr;
    char line[MAXLINELENGTH];
    bool done = 0;
    while (!done) {
        if (fgets(line, MAXLINELENGTH, filePtr) == NULL) {
            done = 1;
        } else {
            if (sscanf(line, "%d", &instr) != 1) {
                printf("error in reading address %d\n", read_pc);
                exit(1);
            }

            memory[read_pc] = instr;
            printf("memory[%d]=%d\n", read_pc, memory[read_pc]);
            read_pc = read_pc + 1;
        }
    }

    memorySize = read_pc;
}

void machineState::printState() {
    int i;

    printf("Cycles: %d\n", cycles);

    printf("\t pc=%d\n", pc);

    printf("\t Reservation stations:\n");
    for (i = 0; i < NUMUNITS; i++) {
        if (reservation[i].busy == 1) {
            printf("\t \t Reservation station %d: ", i);
            if (reservation[i].Qj == 0) {
                printf("Vj = %d ", reservation[i].Vj);
            } else {
                printf("Qj = '%s' ", unitname[reservation[i].Qj - 1]);
            }
            if (reservation[i].Qk == 0) {
                printf("Vk = %d ", reservation[i].Vk);
            } else {
                printf("Qk = '%s' ", unitname[reservation[i].Qk - 1]);
            }
            printf(" ExTimeLeft = %d        RBNum = %d\n",
                         reservation[i].exTimeLeft,
                         reservation[i].reorderNum);
        }
    }

    printf("\t Reorder buffers:\n");
    for (i = 0; i < RBSIZE; i++) {
        if (reorderBuf[i].busy == 1) {
            printf("\t \t Reorder buffer %d: ", i);
            printf(
                    "instr %d        executionUnit '%s'        state %s        valid %d        result %d "
                    "storeAddress %d\n",
                    reorderBuf[i].instr,
                    unitname[reorderBuf[i].execUnit - 1],
                    statename[reorderBuf[i].instrStatus],
                    reorderBuf[i].valid, reorderBuf[i].result,
                    reorderBuf[i].storeAddress);
        }
    }

    printf("\t Register result status:\n");
    for (i = 1; i < NUMREGS; i++) {
        if (!regResult[i].valid) {
            printf("\t \t Register %d: ", i);
            printf("waiting for reorder buffer number %d\n",
                         regResult[i].reorderNum);
        }
    }

    /*
     * 如果你实现了动态分支预测, 将这里的注释取消
     */

    printf("\t Branch target buffer:\n");
    for (i = 0; i < BTBSIZE; i++) {
        if (btBuf[i].valid) {
            printf(
                "\t \t Entry %d: PC=%d, Target=%d, Pred=%d\n",
                i, btBuf[i].branchPC,
                btBuf[i].branchTarget,
                btBuf[i].branchPred
            );
        }
    }

    printf("\t Memory:\n");
    for (i = 0; i < memorySize; i++) {
        printf("\t \t memory[%d] = %d\n", i, memory[i]);
    }

    printf("\t Registers:\n");
    for (i = 0; i < NUMREGS; i++) {
        printf("\t \t regFile[%d] = %d\n", i, regFile[i]);
    }
}

void machineState::updateRes(int unit, int value) {
    /*
    * 更新保留栈:
    * 将位于公共数据总线上的数据
    * 复制到正在等待它的其他保留栈中去
    */
    for (int i = 0; i < NUMUNITS; ++i) {
        if (reservation[i].Qj - 1 == unit) {
            reservation[i].Qj = 0;
            reservation[i].Vj = value;
        }
        if (reservation[i].Qk - 1 == unit) {
            reservation[i].Qk = 0;
            reservation[i].Vk = value;
        }
    }
    reservation[unit].busy = 0;
}

void machineState::issueInstr(int instr, int unit, int reorderNum) {
    /*
    * 发射指令:
    * 填写保留栈和ROB项的内容.
    * 注意, 要在所有的字段中写入正确的值.
    * 检查寄存器状态, 相应的在Vj,Vk和Qj,Qk字段中设置正确的值:
    * 对于I类型指令, 设置Qk=0,Vk=0;
    * 对于sw指令, 如果寄存器有效, 将寄存器中的内存基地址保存在Vj中;
    * 对于beqz和j指令, 将当前PC+1的值保存在Vk字段中.
    * 如果指令在提交时会修改寄存器的值, 还需要在这里更新寄存器状态数据结构.
    */
    resStation &resStat = reservation[unit];
    reorderEntry &robEntry = reorderBuf[reorderNum];
    resStat.instr = instr;
    resStat.busy = 1;
    resStat.reorderNum = reorderNum;
    robEntry.busy = 1;
    robEntry.instr = instr;
    robEntry.execUnit = unit + 1;
    robEntry.instrStatus = ISSUING;
    robEntry.valid = 0;
    robEntry.pc = pc;
    if (isR(instr)) {
        int rs1 = field0(instr), rs2 = field1(instr), rd = field2(instr);
        setQV(rs1, resStat.Qj, resStat.Vj);
        setQV(rs2, resStat.Qk, resStat.Vk);
        resStat.exTimeLeft = INTEXEC;
        setReg(rd, reorderNum);
    } else if (isI(instr)) {
        int rs1 = field0(instr), rd = field1(instr);
        setQV(rs1, resStat.Qj, resStat.Vj);
        if (isS(instr)) {
            setQV(rd, resStat.Qk, resStat.Vk);
            resStat.exTimeLeft = INTEXEC;
        } else if (isB(instr)) {
            resStat.Qk = 0;
            resStat.Vk = pc + 1;
            resStat.exTimeLeft = BRANCHEXEC;
        } else {
            resStat.Qk = resStat.Vk = 0;
            resStat.exTimeLeft = INTEXEC;
            if (isL(instr))
                resStat.exTimeLeft += LDEXEC;
            setReg(rd, reorderNum);
        }
    } else if (isJ(instr)) {
        resStat.Qj = resStat.Vj = 0;
        resStat.Qk = 0;
        resStat.Vk = pc + 1;
        resStat.exTimeLeft = INTEXEC;
    } else {
        resStat.Qj = resStat.Vj = 0;
        resStat.Qk = resStat.Vk = 0;
        resStat.exTimeLeft = INTEXEC;
    }
}

int machineState::checkReorder() {
    /*
     * 在ROB的队尾检查是否有空闲的空间, 如果有, 返回空闲项目的编号.
     * ROB是一个循环队列, 它可以容纳RBSIZE个项目.
     * 新的指令被添加到队列的末尾, 指令提交则是从队首进行的.
     * 当队列的首指针或尾指针到达数组中的最后一项时, 它应滚动到数组的第一项.
     */
    int next = Next(tailRB);
    if (reorderBuf[next].busy == 0)
        return tailRB = next;
    else
        return -1;
}

int machineState::getResult(int unit) {

    /*
     * 这个函数负责计算有输出的指令的结果.
     * 你需要完成下面的case语句....
     */
    resStation &rStation = reservation[unit];
    int op = opcode(rStation.instr);
    int immed = immediate(rStation.instr);

    switch (op) {
        case ANDI:
            return rStation.Vj & immed;
            break;
        case ADDI:
            return rStation.Vj + immed;
            break;
        case LW:
            return memory[rStation.Vj + immed];
            break;
        case SW:
            return rStation.Vj + immed;
            break;
        case BEQZ:
            return rStation.Vj == 0 ? rStation.Vk + immed : rStation.Vk;
            break;
        case J:
            return rStation.Vk + jumpAddr(rStation.instr);
        case regRegALU:
            switch (func(rStation.instr)) {
            case addFunc:
                return rStation.Vj + rStation.Vk;
                break;
            case subFunc:
                return rStation.Vj - rStation.Vk;
                break;
            case andFunc:
                return rStation.Vj & rStation.Vk;
                break;
            }
            break;
        default:
            break;
    }
    return 0;
}

/* 选作内容 */
int machineState::getPrediction(int pc) {
    /*
    * 对给定的PC, 检查分支预测缓冲栈中是否有历史记录
    * 如果有, 返回根据历史信息进行的预测, 否则返回-1
    */
    for (int i = 0; i < BTBSIZE; ++i) {
        if (btBuf[i].valid && btBuf[i].branchPC == pc)
            return i;
    }
    return -1;
}

/* 选作内容 */
void machineState::updateBTB(int branchPC, int targetPC, int outcome) {
    /*
     * 更新分支预测缓冲栈: 检查是否与缓冲栈中的项目匹配.
     * 如果是, 对2-bit的历史记录进行更新;
     * 如果不是, 将当前的分支语句添加到缓冲栈中去.
     * 如果缓冲栈已满，你需要选择一种替换算法将旧的记录替换出去.
     * 如果当前跳转成功, 将初始的历史状态设置为STRONGTAKEN;
     * 如果不成功, 将历史设置为STRONGNOT
     */
    int i = getPrediction(branchPC);
    if (i == -1) {
        btBuf[btPtr].valid = 1;
        btBuf[btPtr].branchPC = branchPC;
        btBuf[btPtr].branchTarget = targetPC;
        btBuf[btPtr].branchPred = outcome * 3;
        ++btPtr;
    } else {
        btBuf[i].branchTarget = targetPC;
        if (outcome)
            btBuf[i].branchPred = std::min(btBuf[i].branchPred + 1, 3);
        else
            btBuf[i].branchPred = std::max(btBuf[i].branchPred - 1, 0);
    }
}

/* 选作内容 */
int machineState::getTarget(int reorderNum) {
    /*
     * [TODO]
     * 检查分支指令是否已保存在分支预测缓冲栈中:
     * 如果不是, 返回当前pc+1, 这意味着我们预测分支跳转不会成功;
     * 如果在, 并且历史信息为STRONGTAKEN或WEAKTAKEN, 返回跳转的目标地址,
     * 如果历史信息为STRONGNOT或WEAKNOT, 返回当前pc+1.
     */
    int i = getPrediction(reorderBuf[reorderNum].pc);
    if (i == -1)
        return reorderBuf[reorderNum].pc + 1;
    else if (btBuf[i].branchPred <= 1)
        return reorderBuf[reorderNum].pc + 1;
    else
        return btBuf[i].branchTarget;
}

int getTargetPC(int instr, int base) {
    if (isB(instr))
        return base + immediate(instr);
    else
        return base + jumpAddr(instr);
}

bool machineState::commitAndCheck() {
    completeStore();
    int next = Next(headRB);
    if (reorderBuf[next].instrStatus != COMMITTING || headRB == tailRB)
        return 1;
    int unit = reorderBuf[next].execUnit - 1;
    int instr = reorderBuf[next].instr;
    if (isB(instr) || isJ(instr)) {
        int prediction = reorderBuf[next].prediction;
        int branchPC = reorderBuf[next].pc;
        int targetPC = getTargetPC(instr, branchPC + 1);
        int outcome = reorderBuf[next].result != branchPC + 1;
        updateBTB(branchPC, targetPC, outcome);
        if (prediction == reorderBuf[next].result) {
            popROB();
        } else {
            pc = reorderBuf[next].result;
            while (headRB != tailRB) {
                next = Next(headRB);
                for (int i = 1; i < NUMREGS; ++i) {
                    if (!regResult[i].valid && regResult[i].reorderNum == next) {
                        regResult[i].reorderNum = -1;
                        regResult[i].valid = 1;
                        break;
                    }
                }
                reservation[reorderBuf[next].execUnit - 1].busy = 0;
                popROB();
            }
        }
    } else if (isS(instr)) {
        StoreBuffer buffer;
        buffer.addr = reorderBuf[next].storeAddress;
        buffer.value = reorderBuf[next].result;
        buffer.completeTime = cycles + STEXEC;
        buffer.unit = reorderBuf[next].execUnit;
        storeQueue.push(buffer);
        popROB();
    } else if (opcode(instr) == HALT) {
        halt = 1;
        return 0;
    }
    else {
        for (int i = 1; i < NUMREGS; ++i) {
            if (!regResult[i].valid && regResult[i].reorderNum == next) {
                regFile[i] = reorderBuf[next].result;
                regResult[i].reorderNum = -1;
                regResult[i].valid = 1;
                break;
            }
        }
        popROB();
    }
    return 1;
}

void machineState::writeResult() {
    for (int i = 0; i < RBSIZE; ++i) {
        if (reorderBuf[i].busy == 0 || reorderBuf[i].instrStatus != WRITINGRESULT)
            continue;
        int instr = reorderBuf[i].instr;
        int unit = reorderBuf[i].execUnit - 1;
        if (isS(instr) && reservation[unit].Qk)
            continue;
        if (isS(instr)) {
            reorderBuf[i].storeAddress = getResult(unit);
            reorderBuf[i].result = reservation[unit].Vk;
        } else {
            reorderBuf[i].result = getResult(unit);
            reorderBuf[i].valid = 1;
            updateRes(unit, reorderBuf[i].result);
        }
        reorderBuf[i].instrStatus = COMMITTING;
    }
}

void machineState::execute() {
    for (int i = 0; i < RBSIZE; ++i) {
        if (reorderBuf[i].busy == 0 || reorderBuf[i].instrStatus != EXECUTING)
            continue;
        int unit = reorderBuf[i].execUnit - 1;
        if (!(--reservation[unit].exTimeLeft)) {
            reorderBuf[i].instrStatus = WRITINGRESULT;
        }
    }
}

void machineState::issue() {
    for (int i = 0; i < RBSIZE; ++i) {
        if (reorderBuf[i].busy == 0 || reorderBuf[i].instrStatus != ISSUING)
            continue;
        int instr = reorderBuf[i].instr;
        int unit = reorderBuf[i].execUnit - 1;
        if (reservation[unit].Qj == 0 && (reservation[unit].Qk == 0 || isS(instr))) {
            reorderBuf[i].instrStatus = EXECUTING;
        }
    }
}

void machineState::testIssue() {
    if (pc >= memorySize)
        return;
    int instr = memory[pc];
    int unit = 0;
    if (isL(instr)) {
        if (!reservation[LOAD1 - 1].busy)
            unit = LOAD1;
        else if (!reservation[LOAD2 - 1].busy)
            unit = LOAD2;
    } else if (isS(instr)) {
        if (!reservation[STORE1 - 1].busy)
            unit = STORE1;
        else if (!reservation[STORE2 - 1].busy)
            unit = STORE2;
    } else {
        if (!reservation[INT1 - 1].busy)
            unit = INT1;
        else if (!reservation[INT2 - 1].busy)
            unit = INT2;
    }
    if ((--unit) == -1)
        return;
    int reorderNum = checkReorder();
    if (reorderNum == -1)
        return;
    issueInstr(instr, unit, reorderNum);
    pc = getTarget(reorderNum);
    reorderBuf[reorderNum].prediction = pc;
}

void machineState::count() {
    ++cycles;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    FILE *filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    /*
     * 初始化, 读输入文件等
     *
     */

    machineState *statePtr = new machineState;
    statePtr->read(filePtr);

    /*
     * 处理指令
     */

    while (1) { /* 执行循环:你应该在执行halt指令时跳出这个循环 */

        statePtr->printState();

        /*
         * 基本要求:
         * 首先, 确定是否需要清空流水线或提交位于ROB的队首的指令.
         * 我们处理分支跳转的缺省方法是假设跳转不成功, 如果我们的预测是错误的,
         * 就需要清空流水线(ROB/保留栈/寄存器状态), 设置新的pc = 跳转目标.
         * 如果不需要清空, 并且队首指令能够提交, 在这里更新状态:
         *                 对寄存器访问, 修改寄存器;
         *                 对内存写操作, 修改内存.
         * 在完成清空或提交操作后, 不要忘了释放保留栈并更新队列的首指针.
         */
        /*
         * 选作内容:
         * 在提交的时候, 我们知道跳转指令的最终结果.
         * 有三种可能的情况: 预测跳转成功, 预测跳转不成功,
         * 不能预测(因为分支预测缓冲栈中没有对应的项目). 如果我们预测跳转成功:
         *                 如果我们的预测是正确的, 只需要继续执行就可以了;
         *                 如果我们的预测是错误的, 即实际没有发生跳转,
         * 就必须重新设置正确的PC值, 并清空流水线. 如果我们预测跳转不成功:
         *                 如果预测是正确的, 继续执行;
         *                 如果预测是错误的, 即实际上发生了跳转, 就必须将PC设置为跳转目标,
         * 并清空流水线. 如果我们不能预测跳转是否成功: 如果跳转成功,
         * 仍然需要清空流水线, 将PC修改为跳转目标. 在遇到分支时,
         * 需要更新分支预测缓冲站的内容.
         */
        if (!statePtr->commitAndCheck())
            break;

        /*
         * 提交完成.
         * 检查所有保留栈中的指令, 对下列状态, 分别完成所需的操作:
         */

        /*
         * 对Writing Result状态:
         * 将结果复制到正在等待该结果的其他保留栈中去;
         * 还需要将结果保存在ROB中的临时存储区中.
         * 释放指令占用的保留栈, 将指令状态修改为Committing
         */
        statePtr->writeResult();

        /*
         * 对Executing状态:
         * 执行剩余时间递减;
         * 在执行完成时, 将指令状态修改为Writing Result
         */
        statePtr->execute();

        /*
         * 对Issuing状态:
         * 检查两个操作数是否都已经准备好, 如果是, 将指令状态修改为Executing
         */
        statePtr->issue();

        /*
         * 最后, 当我们处理完了保留栈中的所有指令后, 检查是否能够发射一条新的指令.
         * 首先检查是否有空闲的保留栈, 如果有, 再检查ROB中是否有空闲的空间,
         * 如果也能找到空闲空间, 发射指令.
         */
        /*
         * 选作内容:
         * 在发射跳转指令时, 将PC修改为正确的目标: 是pc = pc+1, 还是pc = 跳转目标?
         * 在发射其他的指令时, 只需要设置pc = pc+1.
         */
        statePtr->testIssue();

        /*
         * 周期计数加1
         */
        statePtr->count();

    } /* while (1) */
    printf("halting machine\n");
    return 0;
}
