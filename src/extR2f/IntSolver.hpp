#ifndef IntSolver_HPP
#define IntSolver_HPP

#include <map>
#include <vector>

#include "extMsat/Solver.h"
#include "base/abc/abc.h"
#include "misc.hpp"

typedef enum {MCM_TYPE = 0, PUD_TYPE} IntType;

class IntSolver : public Solver {
public:
    IntSolver();
    IntSolver(IntType);
    ~IntSolver();
    Abc_Ntk_t *getInt(std::map<const Var, char *> &);
    void addAClause(const vec<Lit> &ps);
    void addBClause(const vec<Lit> &ps);
    int setnVars(int n);
private:
    std::map<const Var, bool> mALocal;
    std::map<const Var, bool> mBLocal;
    std::map<const Var, bool> mCom;
    std::map<const unsigned, bool> aClause;
    IntType type; 
};

#endif
