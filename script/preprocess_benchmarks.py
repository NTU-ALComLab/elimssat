import os

path = "benchmarks/Adder/sdimacs"
files = os.listdir(path)
new_path = "benchmarks/Adder-P/sdimacs"
os.makedirs(new_path)
print(files)

for index in range(len(files)):
    file = os.path.join(path, files[index])
    print(file)
    with open(file) as f:
        lines = f.readlines()
        f.close()
        
    var_num = 0
    clause_num = 0
    random_var_set = set()
    exist_var_set = set()
    quantifier_list = []
    clause_list = []

    for line in lines:
        par = line.strip().split()
        if len(par) == 0: continue
        if par[0] == 'c':
            pass
        elif par[0] == 'p':
            var_num = int(par[2])
            clause_num = int(par[3])
        elif par[0] == 'e':
            quantifier_list.append(line)
            for i in range(1, len(par) - 1):
                exist_var_set.add(int(par[i]))
        elif par[0] == 'r':
            quantifier_list.append(line)
            for i in range(2, len(par) - 1):
                random_var_set.add(int(par[i]))
        else:
            clause_list.append(line)
    new_var = 1
    var_to_new_var = {}
    for i in range(var_num):
        var = i + 1
        if var in random_var_set or var in exist_var_set:
            var_to_new_var[var] = new_var
            new_var += 1
    var_num = new_var - 1

    file = os.path.join(new_path, files[index])
    print(file)
    f = open(file, "w")
    f.write("p cnf {} {}\n".format(var_num, clause_num))
    for line in quantifier_list:
        par = line.strip().split()
        if par[0] == 'e':
            f.write("{} ".format(par[0]))
            for i in range(1, len(par) - 1):
                f.write("{} ".format(var_to_new_var[int(par[i])]))
        else:
            f.write("{} {} ".format(par[0], par[1]))
            for i in range(2, len(par) - 1):
                f.write("{} ".format(var_to_new_var[int(par[i])]))
        f.write(" 0\n")
    for line in clause_list:
        par = line.strip().split()
        for i in range(len(par) - 1):
            var = int(par[i])
            if var < 0:
                f.write("{} ".format(-var_to_new_var[-var]))
            else:
                f.write("{} ".format(var_to_new_var[var]))


        f.write("0\n")
    f.close()
