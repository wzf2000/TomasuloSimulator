import tkinter as tk
from argparse import ArgumentParser
import turtle

CYCLES = 0
DICT = {}
NAME2ID = {
    'LOAD1': 0,
    'LOAD2': 1,
    'STORE1': 2,
    'STORE2': 3,
    'INT1': 4,
    'INT2': 5,
}
STATE2ID = {
    'ISSUING': 0,
    'EXECUTING': 1,
    'WRITINGRESULT': 2,
    'COMMTITTING': 3,
}
RES = ['LOAD1', 'LOAD2', 'STORE1', 'STORE2', 'INT1', 'INT2']
STATE = ['发射', '执行', '写结果', '提交']
PRED = ['STRONGNOT', 'WEAKNOT', 'WEAKTAKEN', 'STRONGTAKEN']

def opcode(instr):
    return instr >> 26

def func(instr):
    return instr & 2047

def field0(instr):
    return instr >> 21 & 31

def field1(instr):
    return instr >> 16 & 31

def field2(instr):
    return instr >> 11 & 31

def imm(instr):
    ret = instr & 65535
    if ret & 0x8000:
        ret -= 65536
    return ret

def jaddr(instr):
    ret = instr & 67108863
    if ret & 0x200000:
        ret -= 67108864
    return ret

def instr2str(instr):
    if instr < 0:
        instr += 2 ** 32

    if opcode(instr) == 0:
        funcCode = func(instr)
        if funcCode == 32:
            ret = 'add'
        elif funcCode == 34:
            ret = 'sub'
        elif funcCode == 36:
            ret = 'and'
        return f'{ret} r{field2(instr)},r{field0(instr)},r{field1(instr)}'
    elif opcode(instr) == 35:
        return f'lw r{field1(instr)},r{field0(instr)},{imm(instr)}'
    elif opcode(instr) == 43:
        return f'sw r{field1(instr)},r{field0(instr)},{imm(instr)}'
    elif opcode(instr) == 8:
        return f'addi r{field1(instr)},r{field0(instr)},{imm(instr)}'
    elif opcode(instr) == 12:
        return f'andi r{field1(instr)},r{field0(instr)},{imm(instr)}'
    elif opcode(instr) == 4:
        return f'beqz r{field0(instr)},{imm(instr)}'
    elif opcode(instr) == 2:
        return f'j {jaddr(instr)}'
    elif opcode(instr) == 1:
        return 'halt'
    elif opcode(instr) == 3:
        return 'noop'
    else:
        assert(False)

def get_parser():
    parser = ArgumentParser()
    parser.add_argument('-i', '--input', type=str, help='Input result file.')
    return parser

def read_result(file):
    with open(file, 'r') as f:
        lines = f.readlines()
        start = 0
        while lines[start][:6] == 'memory':
            start += 1
        lines = lines[start:]
        results = []
        cycle = 0
        mem_size = start + 16
        for line in lines:
            line = line.strip()
            if line == 'halting machine':
                break
            if line[:8] == 'Cycles: ':
                cycle = int(line.split()[-1])
                results.append({})
            elif line[:3] == 'pc=':
                pc = int(line[3:])
                results[cycle]['pc'] = pc
            elif line[:21] == 'Reservation stations:':
                results[cycle]['res'] = []
                for _ in range(6):
                    results[cycle]['res'].append({
                        'busy': False,
                    })
            elif line[:16] == 'Reorder buffers:':
                results[cycle]['rob'] = []
                for _ in range(16):
                    results[cycle]['rob'].append({
                        'busy': False,
                    })
            elif line[:23] == 'Register result status:':
                results[cycle]['reg_result'] = []
                for _ in range(32):
                    results[cycle]['reg_result'].append({
                        'valid': True,
                    })
            elif line[:21] == 'Branch target buffer:':
                results[cycle]['btb'] = []
                for _ in range(8):
                    results[cycle]['btb'].append({
                        'valid': False,
                    })
            elif line[:7] == 'Memory:':
                results[cycle]['memory'] = []
                for _ in range(mem_size):
                    results[cycle]['memory'].append(0)
            elif line[:10] == 'Registers:':
                results[cycle]['reg'] = []
                for _ in range(32):
                    results[cycle]['reg'].append(0)
            elif line[:20] == 'Reservation station ':
                tokens = line.split()
                res_id = int(tokens[2][:-1])
                results[cycle]['res'][res_id]['busy'] = True
                if tokens[3] == 'Qj':
                    results[cycle]['res'][res_id]['Qj'] = NAME2ID[tokens[5][1:-1]]
                else:
                    results[cycle]['res'][res_id]['Vj'] = int(tokens[5])
                if tokens[6] == 'Qk':
                    results[cycle]['res'][res_id]['Qk'] = NAME2ID[tokens[8][1:-1]]
                else:
                    results[cycle]['res'][res_id]['Vk'] = int(tokens[8])
                results[cycle]['res'][res_id]['ext_time'] = int(tokens[11])
                results[cycle]['res'][res_id]['rob_id'] = int(tokens[14])
            elif line[:15] == 'Reorder buffer ':
                tokens = line.split()
                rob_id = int(tokens[2][:-1])
                results[cycle]['rob'][rob_id]['busy'] = True
                results[cycle]['rob'][rob_id]['instr'] = int(tokens[4])
                results[cycle]['rob'][rob_id]['res_id'] = NAME2ID[tokens[6][1:-1]]
                results[cycle]['rob'][rob_id]['state'] = STATE2ID[tokens[8]]
                results[cycle]['rob'][rob_id]['valid'] = int(tokens[10])
                results[cycle]['rob'][rob_id]['result'] = int(tokens[12])
                results[cycle]['rob'][rob_id]['addr'] = int(tokens[14])
            elif line[:9] == 'Register ':
                tokens = line.split()
                reg_id = int(tokens[1][:-1])
                results[cycle]['reg_result'][reg_id]['valid'] = False
                results[cycle]['reg_result'][reg_id]['rob_id'] = int(tokens[7])
            elif line[:6] == 'Entry ':
                tokens = line.split()
                btb_id = int(tokens[1][:-1])
                results[cycle]['btb'][btb_id]['valid'] = True
                results[cycle]['btb'][btb_id]['pc'] = int(tokens[2][3:-1])
                results[cycle]['btb'][btb_id]['target'] = int(tokens[3][7:-1])
                results[cycle]['btb'][btb_id]['pred'] = int(tokens[4][5:])
            elif line[:7] == 'memory[':
                tokens = line.split()
                mem_id = int(tokens[0][7:-1])
                results[cycle]['memory'][mem_id] = int(tokens[2])
            elif line[:8] == 'regFile[':
                tokens = line.split()
                reg_id = int(tokens[0][8:-1])
                results[cycle]['reg'][reg_id] = int(tokens[2])
    max_rob_cnt = 0
    for result in results:
        robs = result['rob']
        cnt = 0
        for rob in robs:
            if rob['busy']:
                cnt += 1
        max_rob_cnt = max(max_rob_cnt, cnt)
    DICT['rob_cnt'] = max_rob_cnt

    max_btb_cnt = 0
    for result in results:
        btbs = result['btb']
        cnt = 0
        for btb in btbs:
            if btb['valid']:
                cnt += 1
        max_btb_cnt = max(max_btb_cnt, cnt)
    DICT['btb_cnt'] = max_btb_cnt

    DICT['mem_size'] = 16

    return results

def update_regs(regs: tk.PanedWindow):
    result = DICT['results'][CYCLES]
    columns = ['编号', '值', '有效', 'ROB']
    for r in range(32):
        for c, col in enumerate(columns):
            text, var = regs.my_texts[r][c]
            text: tk.Entry
            var: tk.StringVar
            text.config(state='normal')
            if c == 0:
                var.set(str(r))
            elif c == 1:
                var.set(str(result['reg'][r]))
            elif c == 2:
                var.set('是' if result['reg_result'][r]['valid'] else '否')
            elif not result['reg_result'][r]['valid']:
                var.set(str(result['reg_result'][r]['rob_id']))
            else:
                var.set('')
            text.config(state='readonly')

def get_regs(root):
    regs = tk.PanedWindow(root, orient=tk.VERTICAL, sashrelief='groove')
    fr1 = tk.Frame(regs)
    reg_label = tk.Label(fr1, text='寄存器表', font=('Arial 18 bold'), height=2).pack()
    fr2 = tk.Frame(regs)
    columns = ['编号', '值', '有效', 'ROB']
    for c, col in enumerate(columns):
        text = tk.Entry(fr2, width=5, font=('Arial 10 bold'))
        text.insert(tk.END, col)
        text.config(state='readonly')
        text.grid(row=0, column=c)
    for c, col in enumerate(columns):
        text = tk.Entry(fr2, width=5, font=('Arial 10 bold'))
        text.insert(tk.END, col)
        text.config(state='readonly')
        text.grid(row=0, column=c + len(columns))
    texts = []
    for r in range(32):
        texts.append([])
        for c, col in enumerate(columns):
            var = tk.StringVar()
            text = tk.Entry(fr2, width=5, textvariable=var)
            var.set('')
            text.config(state='readonly')
            text.grid(row=r % 16 + 1, column=c + r // 16 * len(columns))
            texts[r].append((text, var))
    regs.add(fr1)
    regs.add(fr2)
    regs.my_texts = texts
    update_regs(regs)
    return regs

def update_robs(robs: tk.PanedWindow):
    result = DICT['results'][CYCLES]
    columns = ['编号', '指令', '状态', '单元', '有效', '结果', '地址']
    r = 0
    for id in range(16):
        row = result['rob'][id]
        if not row['busy']:
            continue
        for c, col in enumerate(columns):
            text, var = robs.my_texts[r][c]
            text: tk.Entry
            var: tk.StringVar
            text.config(state='normal')
            if c == 0:
                var.set(str(id))
            elif c == 1:
                var.set(instr2str(row['instr']))
            elif c == 2:
                var.set(STATE[row['state']])
            elif c == 3:
                var.set(RES[row['res_id']])
            elif c == 4:
                var.set('是' if row['valid'] else '否')
            elif c == 6:
                var.set(str(row['addr']))
            elif row['valid']:
                var.set(str(row['result']))
            else:
                var.set('')
            text.config(state='readonly')
        r += 1
    while r < DICT['rob_cnt']:
        for c, col in enumerate(columns):
            text, var = robs.my_texts[r][c]
            text: tk.Entry
            var: tk.StringVar
            text.config(state='normal')
            var.set('')
            text.config(state='readonly')
        r += 1

def get_robs(root):
    robs = tk.PanedWindow(root, orient=tk.VERTICAL, sashrelief='groove')
    fr1 = tk.Frame(robs)
    rob_label = tk.Label(fr1, text='ROB', font=('Arial 18 bold'), height=1).pack()
    fr2 = tk.Frame(robs)
    columns = ['编号', '指令', '状态', '单元', '有效', '结果', '地址']
    for c, col in enumerate(columns):
        if c == 1:
            text = tk.Entry(fr2, width=15, font=('Arial 10 bold'))
        else:
            text = tk.Entry(fr2, width=10, font=('Arial 10 bold'))
        text.insert(tk.END, col)
        text.config(state='readonly')
        text.grid(row=0, column=c)
    texts = []
    for r in range(DICT['rob_cnt']):
        texts.append([])
        for c, col in enumerate(columns):
            var = tk.StringVar()
            if c == 1:
                text = tk.Entry(fr2, width=15, textvariable=var)
            else:
                text = tk.Entry(fr2, width=10, textvariable=var)
            text.config(state='readonly')
            text.grid(row=r + 1, column=c)
            texts[r].append((text, var))
    robs.add(fr1)
    robs.add(fr2)
    robs.my_texts = texts
    return robs

def update_res(res: tk.PanedWindow):
    result = DICT['results'][CYCLES]
    columns = ['保留站名', 'Busy', '指令', 'Vj', 'Vk', 'Qj', 'Qk', '剩余时间', 'ROB编号']
    for r in range(6):
        row = result['res'][r]
        for c, col in enumerate(columns):
            text, var = res.my_texts[r][c]
            text: tk.Entry
            var: tk.StringVar
            text.config(state='normal')
            if c == 0:
                var.set(RES[r])
            elif c == 1:
                var.set(row['busy'])
            elif c == 2:
                var.set('' if not row['busy'] or 'instr' not in result['rob'][row['rob_id']] else instr2str(result['rob'][row['rob_id']]['instr']))
            elif c == 3:
                var.set('' if not row['busy'] else row['Vj'] if 'Vj' in row else '')
            elif c == 4:
                var.set('' if not row['busy'] else row['Vk'] if 'Vk' in row else '')
            elif c == 5:
                var.set('' if not row['busy'] else row['Qj'] if 'Qj' in row else '')
            elif c == 6:
                var.set('' if not row['busy'] else row['Qk'] if 'Qk' in row else '')
            elif c == 7:
                var.set('' if not row['busy'] else str(row['ext_time']))
            elif c == 8:
                var.set('' if not row['busy'] else str(row['rob_id']))
            text.config(state='readonly')

def get_res(root):
    res = tk.PanedWindow(root, orient=tk.VERTICAL, sashrelief='groove')
    fr1 = tk.Frame(res)
    res_label = tk.Label(fr1, text='保留站', font=('Arial 18 bold'), height=1).pack()
    fr2 = tk.Frame(res)
    columns = ['保留站名', 'Busy', '指令', 'Vj', 'Vk', 'Qj', 'Qk', '剩余时间', 'ROB编号']
    for c, col in enumerate(columns):
        if c == 2:
            text = tk.Entry(fr2, width=12, font=('Arial 10 bold'))
        else:
            text = tk.Entry(fr2, width=8, font=('Arial 10 bold'))
        text.insert(tk.END, col)
        text.config(state='readonly')
        text.grid(row=0, column=c)
    texts = []
    for r in range(6):
        texts.append([])
        for c, col in enumerate(columns):
            var = tk.StringVar()
            if c == 2:
                text = tk.Entry(fr2, width=12, textvariable=var)
            else:
                text = tk.Entry(fr2, width=8, textvariable=var)
            text.config(state='readonly')
            text.grid(row=r + 1, column=c)
            texts[r].append((text, var))
    res.add(fr1)
    res.add(fr2)
    res.my_texts = texts
    update_res(res)
    return res

def update_btbs(btbs: tk.PanedWindow):
    result = DICT['results'][CYCLES]
    columns = ['PC', 'PC指令', '目标', '目标指令', '历史']
    r = 0
    for id in range(8):
        row = result['btb'][id]
        if not row['valid']:
            continue
        for c, col in enumerate(columns):
            text, var = btbs.my_texts[r][c]
            text: tk.Entry
            var: tk.StringVar
            text.config(state='normal')
            if c == 0:
                var.set(str(row['pc']))
            elif c == 1:
                var.set(instr2str(result['memory'][row['pc']]))
            elif c == 2:
                var.set(str(row['target']))
            elif c == 3:
                var.set(instr2str(result['memory'][row['target']]))
            elif c == 4:
                var.set(PRED[row['pred']])
            text.config(state='readonly')
        r += 1
    while r < DICT['btb_cnt']:
        for c, col in enumerate(columns):
            text, var = btbs.my_texts[r][c]
            text: tk.Entry
            var: tk.StringVar
            text.config(state='normal')
            var.set('')
            text.config(state='readonly')
        r += 1

def get_btbs(root):
    btbs = tk.PanedWindow(root, orient=tk.VERTICAL, sashrelief='groove')
    fr1 = tk.Frame(btbs)
    btb_label = tk.Label(fr1, text='BTB', font=('Arial 18 bold'), height=1).pack()
    fr2 = tk.Frame(btbs)
    columns = ['PC', 'PC指令', '目标', '目标指令', '历史']
    for c, col in enumerate(columns):
        if c == 4 or c == 1 or c == 3:
            text = tk.Entry(fr2, width=15, font=('Arial 10 bold'))
        else:
            text = tk.Entry(fr2, width=10, font=('Arial 10 bold'))
        text.insert(tk.END, col)
        text.config(state='readonly')
        text.grid(row=0, column=c)
    texts = []
    for r in range(DICT['btb_cnt']):
        texts.append([])
        for c, col in enumerate(columns):
            var = tk.StringVar()
            if c == 4 or c == 1 or c == 3:
                text = tk.Entry(fr2, width=15, textvariable=var)
            else:
                text = tk.Entry(fr2, width=10, textvariable=var)
            text.config(state='readonly')
            text.grid(row=r + 1, column=c)
            texts[r].append((text, var))
    btbs.add(fr1)
    btbs.add(fr2)
    btbs.my_texts = texts
    return btbs

def update_memory(memory: tk.PanedWindow):
    result = DICT['results'][CYCLES]
    columns = ['地址', '数据']
    for r in range(DICT['mem_size']):
        for c, col in enumerate(columns):
            text, var = memory.my_texts[r][c]
            text: tk.Entry
            var: tk.StringVar
            text.config(state='normal')
            if c == 0:
                var.set(str(r))
            elif c == 1:
                tmp = result['memory'][r]
                var.set(str(tmp))
            text.config(state='readonly')

def get_memory(root):
    memory = tk.PanedWindow(root, orient=tk.VERTICAL, sashrelief='groove')
    fr1 = tk.Frame(memory)
    memory_label = tk.Label(fr1, text='内存', font=('Arial 18 bold'), height=2).pack()
    fr2 = tk.Frame(memory)
    columns = ['地址', '数据']
    cols = 7
    for i in range(cols):
        for c, col in enumerate(columns):
            if c == 1:
                text = tk.Entry(fr2, width=11, font=('Arial 10 bold'))
            else:
                text = tk.Entry(fr2, width=5, font=('Arial 10 bold'))
            text.insert(tk.END, col)
            text.config(state='readonly')
            text.grid(row=0, column=c + i * len(columns))
    rows = (DICT['mem_size'] + cols - 1) // cols
    texts = []
    for r in range(DICT['mem_size']):
        texts.append([])
        for c, col in enumerate(columns):
            var = tk.StringVar()
            if c == 1:
                text = tk.Entry(fr2, width=11, textvariable=var)
            else:
                text = tk.Entry(fr2, width=5, textvariable=var)
            var.set('')
            text.config(state='readonly')
            text.grid(row=r // cols + 1, column=c + r % cols * len(columns))
            texts[r].append((text, var))
    memory.add(fr1)
    memory.add(fr2)
    memory.my_texts = texts
    update_memory(memory)
    return memory

def update():
    update_regs(DICT['regs'])
    update_robs(DICT['robs'])
    update_res(DICT['res'])
    update_btbs(DICT['btbs'])
    update_memory(DICT['memory'])
    if CYCLES > 0:
        DICT['last'].config(state=tk.NORMAL)
        DICT['reset'].config(state=tk.NORMAL)
    else:
        DICT['last'].config(state=tk.DISABLED)
        DICT['reset'].config(state=tk.DISABLED)
    if CYCLES < len(DICT['results']) - 1:
        DICT['next'].config(state=tk.NORMAL)
    else:
        DICT['next'].config(state=tk.DISABLED)
    if CYCLES < len(DICT['results']) - 10:
        DICT['next10'].config(state=tk.NORMAL)
    else:
        DICT['next10'].config(state=tk.DISABLED)
    DICT['pc'].set(f'PC: {DICT["results"][CYCLES]["pc"]}')
    DICT['cycles'].set(f'周期数: {CYCLES}')

def add_update():
    global CYCLES
    CYCLES += 1
    update()

def sub_update():
    global CYCLES
    CYCLES -= 1
    update()

def add_10_update():
    global CYCLES
    CYCLES += 10
    update()

def reset_update():
    global CYCLES
    CYCLES = 0
    update()

def main():
    parser = get_parser()
    args = parser.parse_args()
    results = read_result(args.input)
    DICT['results'] = results
    root = tk.Tk()
    root.title('Tomasulo Display')
    root.geometry('1080x700+200+100')
    pw0 = tk.PanedWindow(root, orient=tk.VERTICAL, sashrelief='groove')
    pw0.pack(fill=tk.BOTH, expand=True)
    buttons = tk.PanedWindow(pw0, orient=tk.HORIZONTAL, sashrelief='groove')
    next = tk.Button(buttons, text='下一个周期', command=add_update)
    DICT['next'] = next
    next10 = tk.Button(buttons, text='下十个周期', command=add_10_update)
    DICT['next10'] = next10
    last = tk.Button(buttons, text='上一个周期', command=sub_update, state=tk.DISABLED)
    DICT['last'] = last
    reset = tk.Button(buttons, text='重置', command=reset_update, state=tk.DISABLED)
    DICT['reset'] = reset
    infos = tk.PanedWindow(buttons, orient=tk.HORIZONTAL, sashrelief='groove')
    pc_text = tk.StringVar()
    pc = tk.Label(buttons, textvariable=pc_text, font=('Arial 18 bold'))
    pc_text.set('PC: 16')
    DICT['pc'] = pc_text
    cycles_text = tk.StringVar()
    cycles = tk.Label(buttons, textvariable=cycles_text, font=('Arial 18 bold'))
    cycles_text.set('周期数: 0')
    DICT['cycles'] = cycles_text
    buttons.add(last, sticky='W')
    buttons.add(next, sticky='W')
    buttons.add(next10, sticky='W')
    buttons.add(reset, sticky='W')
    infos.add(pc, sticky='E')
    infos.add(cycles, sticky='W')
    buttons.add(infos, sticky='WE')
    pw1 = tk.PanedWindow(root, orient=tk.HORIZONTAL, sashrelief='groove')
    regs = get_regs(pw1)
    DICT['regs'] = regs
    pw2 = tk.PanedWindow(pw1, orient=tk.VERTICAL, sashrelief='groove')
    pw2.pack(fill=tk.BOTH, expand=True)
    robs = get_robs(pw2)
    DICT['robs'] = robs
    res = get_res(pw2)
    DICT['res'] = res
    btbs = get_btbs(pw2)
    DICT['btbs'] = btbs
    memory = get_memory(pw0)
    DICT['memory'] = memory
    pw2.add(robs)
    pw2.add(res)
    pw2.add(btbs)
    pw1.add(regs)
    pw1.add(pw2)
    pw0.add(buttons)
    pw0.add(pw1)
    pw0.add(memory)
    update()
    root.mainloop()

if __name__ == '__main__':
    main()
