import sys
import pdb
import time

def main(argv):
    resolution = 3

    filename = argv[1]
    outfile = argv[2] if len(argv) > 2 else filename.replace(
        '.sdimacs', '-05.sdimacs')
    f = open(filename, 'r')
    lines = f.readlines()
    f.close()

    var_num = 0
    c_num = 0

    t = 0

    # map variables
    f = open(outfile, 'w')
    mapping = {}
    clauses = []
    for line in lines:
        par = line.strip().split()
        if len(par) == 0: continue
        if par[0] == 'c':
            pass
        elif par[0] == 'p':
            var_num = int(par[2])
            c_num = int(par[3])
            # print('Origin var num = ', var_num)
        elif par[0] == 'r':
            prob = float(par[1])
            f.write('r 0.5 ')
            for i in range(2, len(par) - 1):
                var_id = int(par[i])
                start = time.time()
                
                new_var, op = var_map(var_id, prob, var_num, resolution=resolution)
                t += (time.time() - start)

                if len(new_var) != 0:
                    mapping[var_id] = (new_var, op)
                    var_num += len(new_var)
                    # print('Added var num = ', var_num)

                f.write('%d ' % (var_id))
                for var_id in new_var:
                    f.write('%d ' % (var_id))
            f.write("0\n")
        elif par[0] == 'e':
            var_id = int(par[1])
            f.write(line)
        else:
            clauses.append(line)
            # raise ValueError('Wrong quantifier', line)
    f.close()

    print('Normalization time = %s' % (time.time() - start))

    # convert string clause to int clause
    clauses = str2int_clause(clauses)

    start = time.time()

    # convert clause to cnf clause
    for key, (new_var, op) in mapping.items():
        clauses = rewrite_clause(key, new_var, op, clauses)
    
    print('Conversion time = %s' % (time.time() - start))

    f = open(outfile, 'r')
    lines = f.readlines()
    f.close()
    # var_num = len(lines)
    c_num = len(clauses)

    f = open(outfile, 'w')
    f.write('p cnf %d %d\n' % (var_num, c_num))
    for line in lines:
        f.write(line)

    for clause in clauses:
        for var in clause:
            f.write('%d ' % (var))
        f.write('0\n')
    f.close()


def var_map(var_id, prob, var_num, resolution=5):
    app_prob = 0.5
    s = 0.5
    new_var = []
    op = []
    for i in range(resolution):
        if prob == app_prob:
            break

        new_var.append(var_num + i + 1)
        s *= 0.5
        # In op, 0: or, 1: and
        if prob > app_prob:
            app_prob += s
            op.append(0)
        else:
            app_prob -= s
            op.append(1)
        
    return new_var, op


def str2int_clause(clauses):
    new_clauses = []
    for line in clauses:
        par = line.strip().split()
        c = [int(var) for var in par[:-1]]
        new_clauses.append(c)

    return new_clauses


def rewrite_clause(key, new_var, op, clauses):
    neg_var = [-var for var in new_var]
    neg_op = [1 - o for o in op]

    new_clauses = []
    for clause in clauses:
        if key in clause:
            c = gen2cnf(key, new_var, op, clause)
            new_clauses += c
        elif -key in clause:
            c = gen2cnf(-key, neg_var, neg_op, clause)
            new_clauses += c
        else:
            new_clauses.append(clause)

    return new_clauses


def gen2cnf(key, new_var, ops, clause):
    # convert general formula to cnf formula
    if len(new_var) != len(ops):
        raise ValueError('Var and Op cannot match', len(new_var), len(ops))

    clause.remove(key)
    base_clause = [var for var in clause]

    # reverse variables and ops
    vars = [key] + new_var
    # vars = vars[::-1]
    # ops = ops[::-1]

    clause = gen2cnf_rec(vars, ops, base_clause)

    return clause


def gen2cnf_rec(vars, ops, clause):
    if len(ops) == 0:
        return [clause + vars]
    if ops[0] == 0:
        clause += [vars[0]]
        new_clauses = gen2cnf_rec(vars[1:], ops[1:], clause)
    elif ops[0] == 1:
        c1 = clause + [vars[0]]
        c2 = gen2cnf_rec(vars[1:], ops[1:], clause)
        new_clauses = [c1] + c2

    return new_clauses


if __name__ == "__main__":
    main(sys.argv)
