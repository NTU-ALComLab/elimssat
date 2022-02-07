#ifndef CnfConvert_HPP
#define CnfConvert_HPP

#include <vector>
#include <map>

#include "base/abc/abc.h"
#include "extMsat/Solver.h"
#include "bdd/extrab/extraBdd.h"

class CnfConvert {
public:
    CnfConvert(Abc_Ntk_t* p);
    std::vector<std::vector<Lit> >& getCnf() { return pCnf; }
    std::map<int, bool>& getInter() { return mInter; }
    int getMaxVar() { return maxVar; }
    int ntkToCnf();
    int ntkToCnf(Abc_Ntk_t* p);
    
private:
    Abc_Ntk_t *pNtk;
    int maxVar;
    std::map<int, bool> mInter;
    std::vector<std::vector<Lit> > pCnf;    
    void printLits(const std::vector<Lit> &lits);
    void printCnf();
    void addNodeClauses(char *pSop0, char *pSop1, Abc_Obj_t* pNode);
};

#endif
