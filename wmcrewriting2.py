'''
General formula normalize to 0.5, variables assigned to next exist level
'''

import sys
import time


def main(argv):
    filename = argv[1]
    outfile = argv[2] if len(argv) > 2 else filename.replace(
        '.sdimacs', '-05.sdimacs')

    start, v_num, rand_vars, exist_vars, c_num, clauses = parse_sdimacs(
        filename)

    started = time.time()

    v_num, rand_vars, exist_vars, new_clauses = rewrite(
        start, rand_vars, exist_vars, v_num)

    print('Conversion time = %s' % (time.time() - started))

    clauses += new_clauses
    c_num = len(clauses)

    f = open(outfile, 'w')
    f.write('p cnf %d %d\n' % (v_num, c_num))
    if start == 'e':
        f.write('e ')
        for var in exist_vars[0]:
            f.write('%d ' % (var))
        f.write('0\n')

    for level, rand_var in enumerate(rand_vars):
        if start == 'e':
            e_level = level + 1
        else:
            e_level = level
        exist_var = exist_vars[e_level]
        f.write('r 0.5 ')
        for var in rand_var:
            f.write('%d ' % (var[0]))
        f.write('0 \n')
        f.write('e ')
        for var in exist_var:
            f.write('%d ' % (var))
        f.write('0\n')

    for clause in clauses:
        for id in clause:
            f.write('%d ' % (id))
        f.write('0\n')


def parse_sdimacs(filename):
    f = open(filename, 'r')
    lines = f.readlines()
    f.close()

    start = None
    v_num = 0
    c_num = 0
    level = 0
    cur_quant = None
    rand_var = []
    exist_var = []
    clauses = []
    for line in lines:
        p = line.strip().split()
        if p[0] == 'c':
            continue
        elif p[0] == 'p':
            v_num = int(p[2])
            c_num = int(p[3])
        elif p[0] == 'e':
            if start is None:
                start = 'e'
                level += 1
                exist_var.append([])
            if cur_quant == 'r':
                level += 1
                exist_var.append([])
            cur_quant = 'e'
            for v in p[1:-1]:
                l = int((level - 1) / 2)
                exist_var[l].append(int(v))
        elif p[0] == 'r':
            if start is None:
                start = 'r'
                level += 1
                rand_var.append([])
            if cur_quant == 'e':
                level += 1
                rand_var.append([])
            cur_quant = 'r'
            for v in p[2:-1]:
                l = int((level - 1) / 2)
                rand_var[l].append((int(v), float(p[1])))
        else:
            c = []
            for v in p:
                if int(v) != 0:
                    c.append(int(v))
            clauses.append(c)

    v_num_get = 0
    for v in rand_var:
        v_num_get += len(v)
    for v in exist_var:
        v_num_get += len(v)
    # assert v_num_get == v_num, '%d and %d' % (v_num_get, v_num)
    assert len(clauses) == c_num, '%d and %d' % (len(clauses), c_num)

    return start, v_num, rand_var, exist_var, c_num, clauses


def rewrite(start, rand_vars, exist_vars, v_num, iter=3):
    new_clauses = []
    for level, rand_var in enumerate(rand_vars):
        if start == 'e':
            e_level = level + 1
        else:
            e_level = level
        try:
            exist_var = exist_vars[e_level]
        except:
            exist_var = []
            exist_vars.append(exist_var)
        rm_vars = set([])
        for (vid, prob) in rand_var:
            for j in range(iter):
                if prob == 0.5:
                    break
                # introduce 2 new variables
                var1 = v_num + 1
                var2 = v_num + 2
                v_num += 2
                if j == 0:
                    rm_vars.add((vid, prob))
                exist_var.append(vid)
                rand_var.append((var1, 0.5))
                '''
                (inv ^ var) <=> (y_var & z_var)
                Next, z_var = var
                '''
                if prob > 0.5:
                    prob = 1 - prob
                    new_clauses.append([vid, var1])
                    new_clauses.append([vid, var2])
                    new_clauses.append([-var1, -var2, -vid])
                else:
                    new_clauses.append([-vid, var1])
                    new_clauses.append([-vid, var2])
                    new_clauses.append([-var1, -var2, vid])

                prob *= 2
                vid = var2
                if j == iter - 1:
                    rand_var.append((vid, 0.5))
                elif prob == 0.5:
                    rand_var.append((vid, 0.5))
        for v in rm_vars:
            rand_var.remove(v)

    return v_num, rand_vars, exist_vars, new_clauses


if __name__ == "__main__":
    main(sys.argv)
