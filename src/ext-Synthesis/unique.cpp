#include "unique.h"
#include "aig/aig/aig.h"
#include "base/io/ioAbc.h"
#include "extAvyItp/ItpMinisat.h"
#include "misc/util/abc_global.h"
#include "misc/vec/vecInt.h"
#include "sat/cnf/cnf.h"
#include <algorithm>
#include <boost/logic/tribool.hpp>
#include <vector>

using std::cout;
using std::endl;
using std::vector;

static const int CONFLICT_LIMIT = 1000;

extern "C" {
Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *, int, int);
}

static Aig_Obj_t *getAigObject(Abc_Ntk_t *pNtk, int id) {
  return (Aig_Obj_t *)Abc_NtkObj(pNtk, id)->pCopy;
}

Abc_Ntk_t *uniquePreprocess(Abc_Ntk_t *pNtk, Vec_Int_t **pScope,
                            bool verbose = false) {
  Aig_Obj_t *pObj;
  vector<int> sharedVariables;
  vector<int> selectionVariables;
  int entry, index;
  int *pBeg, *pEnd;
  Aig_Man_t *pMan = Abc_NtkToDar(pNtk, 0, 0);
  // mark considered variables
  Aig_ManIncrementTravId(pMan);
  Vec_IntForEachEntry(*pScope, entry, index) {
    Aig_Obj_t *pObj = getAigObject(pNtk, entry);
    assert(Aig_ObjIsCi(pObj));
    Aig_ObjSetTravIdCurrent(pMan, pObj);
  }
  Cnf_Dat_t *pCnf = Cnf_Derive(pMan, Aig_ManCoNum(pMan));
  Cnf_Dat_t *pCnfDup = Cnf_DataDup(pCnf);
  Cnf_DataPrint(pCnf, 1);
  Cnf_DataLift(pCnfDup, pCnf->nVars);
  Cnf_DataPrint(pCnfDup, 1);
  avy::ItpMinisat *solver =
      new avy::ItpMinisat(2, 2 * pCnf->nVars + 2 * Vec_IntSize(*pScope), false);
  solver->markPartition(1);
  Cnf_CnfForClause(pCnf, pBeg, pEnd, index) {
    if (pEnd - pBeg == 1) {
      solver->addUnit(*pBeg);
    } else {
      solver->addClause(pBeg, pEnd);
    }
  }
  solver->markPartition(2);
  Cnf_CnfForClause(pCnfDup, pBeg, pEnd, index) {
    if (pEnd - pBeg == 1) {
      solver->addUnit(*pBeg);
    } else {
      solver->addClause(pBeg, pEnd);
    }
  }
  // make defining variables equivalent
  Aig_ManForEachCi(pMan, pObj, index) {
    if (!Aig_ObjIsTravIdCurrent(pMan, pObj)) {
      int pClause[2];
      pClause[0] = toLit(pCnf->pVarNums[Aig_ObjId(pObj)]);
      pClause[1] = toLitCond(pCnfDup->pVarNums[Aig_ObjId(pObj)], true);
      printf("%s%d %s%d\n", "", lit_var(pClause[0]), "-", lit_var(pClause[1]));
      solver->markPartition(1);
      solver->addClause(pClause, pClause + 2);
      pClause[0] = toLitCond(pCnf->pVarNums[Aig_ObjId(pObj)], true);
      pClause[1] = toLit(pCnfDup->pVarNums[Aig_ObjId(pObj)]);
      printf("%s%d %s%d\n", "-", lit_var(pClause[0]), "", lit_var(pClause[1]));
      solver->markPartition(2);
      solver->addClause(pClause, pClause + 2);
    }
  }
  cout << endl;
  // add selector clause
  Vec_IntForEachEntry(*pScope, entry, index) {
    assert(Aig_ObjIsTravIdCurrent(pMan, getAigObject(pNtk, entry)));
    int pClause[2];
    pClause[0] = toLit(pCnf->pVarNums[Aig_ObjId(getAigObject(pNtk, entry))]);
    pClause[1] = toLitCond(2 * pCnf->nVars + (2 * index + 1), true);
    printf("%s%d %s%d\n", "", lit_var(pClause[0]), Abc_LitIsCompl(pClause[1]) ? "-": "", lit_var(pClause[1]));
    selectionVariables.push_back(2 * pCnf->nVars + (2 * index + 1));
    solver->markPartition(1);
    solver->addClause(pClause, pClause + 2);
    pClause[0] = toLitCond(
        pCnfDup->pVarNums[Aig_ObjId(getAigObject(pNtk, entry))], true);
    pClause[1] = toLitCond(2 * pCnf->nVars + (2 * index + 2), true);
    printf("%s%d %s%d\n", "-", lit_var(pClause[0]), Abc_LitIsCompl(pClause[1]) ? "-": "", lit_var(pClause[1]));
    selectionVariables.push_back(2 * pCnf->nVars + (2 * index + 2));
    solver->markPartition(2);
    solver->addClause(pClause, pClause + 2);
  }
  Vec_IntForEachEntry(*pScope, entry, index) {
    cout << index << " itertions, checking variables: " << entry << endl;
    vector<int> assumptions;
    std::transform(selectionVariables.begin(), selectionVariables.end(),
                   std::back_inserter(assumptions),
                   [index, pCnf](int v) -> int {
                     int j = 2 * pCnf->nVars + 2 * index;
                     return toLitCond(v, (j + 1 != v) && (j + 2 != v));
                   });
    boost::tribool result = solver->solve(assumptions, CONFLICT_LIMIT);
    cout << index + 1 << "/" << Vec_IntSize(*pScope) << " ";
    if (!result) { // UNSAT, get interpolant
      // TODO get interpolant
      cout << "UNSAT" << endl;
    } else {
      cout << "SAT" << endl;
      cout << "value for first partition" << endl;
    }
  }
  delete solver;
  return pNtk;
}
