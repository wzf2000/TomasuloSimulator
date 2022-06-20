#include "utils.hpp"

struct machineState {                  /* 虚拟机状态的数据结构 */
    int pc;                            /* PC */
    int cycles;                        /* 已经过的周期数 */
    resStation reservation[NUMUNITS];  /* 保留栈 */
    reorderEntry reorderBuf[RBSIZE];   /* ROB */
    regResultEntry regResult[NUMREGS]; /* 寄存器状态 */
    btbEntry btBuf[BTBSIZE];           /* 分支预测缓冲栈 */
    int memory[MEMSIZE];               /* 内存 */
    int regFile[NUMREGS];              /* 寄存器 */
    int btPtr = 0;
    int headRB, tailRB;
    int halt;
    int memorySize;
    struct StoreBuffer {
        int addr;
        int value;
        int completeTime;
        int unit;
    };
    std::queue<StoreBuffer> storeQueue;

private:
    void setQV(int r, int &Q, int &V) {
        if (regResult[r].valid) {
            Q = 0;
            V = regFile[r];
        } else if (reorderBuf[regResult[r].reorderNum].valid) {
            Q = 0;
            V = reorderBuf[regResult[r].reorderNum].result;
        } else {
            Q = reorderBuf[regResult[r].reorderNum].execUnit;
        }
    }

    void setReg(int rd, int reorderNum) {
        if (rd == 0)
            return;
        regResult[rd].valid = 0;
        regResult[rd].reorderNum = reorderNum;
    }

    void completeStore() {
        while (!storeQueue.empty() && storeQueue.front().completeTime >= cycles) {
            auto buffer = storeQueue.front();
            storeQueue.pop();
            memory[buffer.addr] = buffer.value;
            reservation[buffer.unit - 1].busy = 0;
        }
    }

    void popROB() {
        int reorderNum = Next(headRB);
        int unit = reorderBuf[reorderNum].execUnit;
        reorderBuf[reorderNum].busy = 0;
        headRB = reorderNum;
    }

public:
    machineState();
    void read(FILE *filePtr);
    void printState();
    void updateRes(int unit, int value);
    void issueInstr(int instr, int unit, int reorderNum);
    int getPrediction(int pc);
    int checkReorder();
    int getResult(int unit);
    void updateBTB(int branchPC, int targetPC, int outcome);
    int getTarget(int reorderNum);
    bool commitAndCheck();
    void writeResult();
    void execute();
    void issue();
    void testIssue();
    void count();
};
