from argparse import ArgumentParser

INSTR_NAMES = ['add', 'addi', 'and', 'andi', 'sub', 'lw', 'sw', 'beqz', 'j', 'halt', 'noop']
R = ['add', 'and', 'sub']
OPCODES = {
    'lw': 35,
    'sw': 43,
    'add': 0,
    'addi': 8,
    'sub': 0,
    'and': 0,
    'andi': 12,
    'beqz': 4,
    'j': 2,
    'halt': 1,
    'noop': 3,
}
FUNCS = {
    'add': 32,
    'and': 36,
    'sub': 34,
}
I = ['addi', 'andi', 'lw', 'sw', 'beqz']

def get_parser():
    parser = ArgumentParser()
    parser.add_argument('-i', '--input', type=str, help='Input asm file.')
    parser.add_argument('-o', '--output', type=str, help='Output machine code file.')
    return parser

if __name__ == '__main__':
    parser = get_parser()
    args = parser.parse_args()
    outputs = []
    labels = {}
    with open(args.input, 'r') as f:
        lines = f.readlines()
        for i, line in enumerate(lines):
            label = line.strip().split()[0]
            if label in INSTR_NAMES:
                continue
            labels[label] = i
        
        for i, line in enumerate(lines):
            line = line.strip()
            if i in labels.values():
                name = line.split()[1].lower()
                if name != 'halt' and name != 'noop':
                    regs = line.split()[2]
            else:
                name = line.split()[0].lower()
                if name != 'halt' and name != 'noop':
                    regs = line.split()[1]
            op = OPCODES[name]
            if name in R:
                r3, r1, r2 = regs.split(',')
                func = FUNCS[name]
                r1 = int(r1[1:])
                r2 = int(r2[1:])
                r3 = int(r3[1:])
                out = op
                out = out << 5 | r1
                out = out << 5 | r2
                out = out << 5 | r3
                out = out << 11 | func
            elif name == 'beqz':
                r1, imm = regs.split(',')
                r2 = 0
                r1 = int(r1[1:])
                if imm in labels:
                    imm = labels[imm] - i - 1
                else:
                    imm = int(imm)
                if imm < 0:
                    imm += 65536
                out = op
                out = out << 5 | r1
                out = out << 5 | r2
                out = out << 16 | imm
            elif name in I:
                r2, r1, imm = regs.split(',')
                r1 = int(r1[1:])
                r2 = int(r2[1:])
                imm = int(imm)
                if imm < 0:
                    imm += 65536
                out = op
                out = out << 5 | r1
                out = out << 5 | r2
                out = out << 16 | imm
            elif name == 'j':
                imm = regs
                if imm in labels:
                    imm = labels[imm] - i - 1
                else:
                    imm = int(imm)
                if imm < 0:
                    imm += 67108864
                out = op
                out = out << 26 | imm
            else:
                out = op << 26
            outputs.append(out)
    with open(args.output, 'w') as f:
        for out in outputs:
            assert(out < 2 ** 32)
            if out >= 2 ** 31:
                out -= 2 ** 32
            f.write(f"{out}\n")
