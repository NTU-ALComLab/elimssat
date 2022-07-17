#include "extUtil/easylogging++.h"
#include "extUtil/utilVec.h"
#include "ssatParser.h"
#include "extUtil/easylogging++.h"

ABC_NAMESPACE_IMPL_START

extern "C" {
Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *, int, int);
}

static void unateDetection(Abc_Ntk_t *, Vec_Int_t *, Vec_Ptr_t *, Vec_Ptr_t *);

static void writeCnfIntoSolver(Cnf_Dat_t *, sat_solver *);
static bool isUnate(sat_solver *, Cnf_Dat_t *, Cnf_Dat_t *, Vec_Int_t *,
                    Aig_Man_t *, int, bool);
static bool isPositiveUnate(sat_solver *pSat, Cnf_Dat_t *pCnfPos,
                            Cnf_Dat_t *pCnfNeg, Vec_Int_t *pVecEnVars,
                            Aig_Man_t *pMan, int pIndexCi) {
  return isUnate(pSat, pCnfPos, pCnfNeg, pVecEnVars, pMan, pIndexCi, true);
}
static bool isNegativeUnate(sat_solver *pSat, Cnf_Dat_t *pCnfPos,
                            Cnf_Dat_t *pCnfNeg, Vec_Int_t *pVecEnVars,
                            Aig_Man_t *pMan, int pIndexCi) {
  return isUnate(pSat, pCnfPos, pCnfNeg, pVecEnVars, pMan, pIndexCi, false);
}

Abc_Ntk_t* unatePreprocess(Abc_Ntk_t* pNtk, Vec_Int_t **pScope) {
  Abc_Ntk_t *pNtkNew = Abc_NtkStartFrom(pNtk, pNtk->ntkType, pNtk->ntkFunc);
  VLOG(1) << "size of the pScope before preprocess: " << Vec_IntSize(*pScope);
  Abc_Obj_t *pObj;
  int entry, index;
  Vec_Ptr_t* vPosUnate = Vec_PtrAlloc(0);
  Vec_Ptr_t* vNegUnate = Vec_PtrAlloc(0);
  Vec_Int_t* pScopeNew = Vec_IntAlloc(0);
  unateDetection(pNtk, *pScope, vPosUnate, vNegUnate);
  VLOG(1) << "number of positive unate: " << Vec_PtrSize(vPosUnate);
  VLOG(1) << "number of negative unate: " << Vec_PtrSize(vNegUnate);
  Abc_NtkForEachPi(pNtk, pObj, index) {
    pObj->pCopy = Abc_NtkPi(pNtkNew, index);
  }
  Abc_NtkForEachCo(pNtk, pObj, index) {
    pObj->pCopy = Abc_NtkPo(pNtkNew, index);
  }
  Abc_NtkIncrementTravId(pNtkNew);
  Vec_PtrForEachEntry(Abc_Obj_t*, vPosUnate, pObj, index) {
    Abc_NodeSetTravIdCurrent(pObj->pCopy);
    pObj->pCopy = Abc_AigConst1(pNtkNew);
  }
  Vec_PtrForEachEntry(Abc_Obj_t*, vNegUnate, pObj, index) {
    Abc_NodeSetTravIdCurrent(pObj->pCopy);
    pObj->pCopy = Abc_ObjNot(Abc_AigConst1(pNtkNew));
  }
  Vec_IntForEachEntry(*pScope, entry, index) {
    if (!Abc_NodeIsTravIdCurrent(Abc_NtkPi(pNtkNew, entry))) {
      Vec_IntPush(pScopeNew, entry);
    }
  }

  Abc_AigForEachAnd(pNtk, pObj, index) {
    pObj->pCopy = Abc_AigAnd((Abc_Aig_t *)pNtkNew->pManFunc,
                             Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
  }
  // relink the CO nodes
  Abc_NtkForEachCo(pNtk, pObj, index) {
    Abc_ObjAddFanin(pObj->pCopy, Abc_ObjChild0Copy(pObj));
  }
  Vec_PtrFree(vPosUnate);
  Vec_PtrFree(vNegUnate);
  *pScope = pScopeNew;
  VLOG(1) << "size of the pScope after preprocess: " << Vec_IntSize(*pScope);
  Abc_NtkDelete(pNtk);
  return pNtkNew;
}

void unateDetection(Abc_Ntk_t *pNtkCone, Vec_Int_t *pScope,
                    Vec_Ptr_t *vPosUnate, Vec_Ptr_t *vNegUnate) {
  Aig_Man_t *pMan = Abc_NtkToDar(pNtkCone, 0, 0);
  Cnf_Dat_t *pCnfPositiveCofactor = Cnf_Derive(pMan, Aig_ManCoNum(pMan));
  Cnf_Dat_t *pCnfNegativeCofactor = Cnf_DataDup(pCnfPositiveCofactor);
  Cnf_DataLift(pCnfNegativeCofactor, pCnfPositiveCofactor->nVars);
  sat_solver *pSatSolver = sat_solver_new();
  sat_solver_setnvars(pSatSolver, pCnfPositiveCofactor->nVars +
                                      pCnfNegativeCofactor->nVars);
  writeCnfIntoSolver(pCnfPositiveCofactor, pSatSolver);
  writeCnfIntoSolver(pCnfNegativeCofactor, pSatSolver);
  Vec_Int_t *vEnableVars = Vec_IntAlloc(Aig_ManCiNum(pMan));
  Aig_Obj_t *pCi;
  int j;
  Aig_ManForEachCi(pMan, pCi, j) {
    Vec_IntPush(vEnableVars, sat_solver_addvar(pSatSolver));
    sat_solver_add_buffer_enable(pSatSolver,
                                 pCnfPositiveCofactor->pVarNums[Aig_ObjId(pCi)],
                                 pCnfNegativeCofactor->pVarNums[Aig_ObjId(pCi)],
                                 Vec_IntEntryLast(vEnableVars), 0);
  }
  int entry, index;
  Vec_IntForEachEntry(pScope, entry, index) {
    if (isPositiveUnate(pSatSolver, pCnfPositiveCofactor, pCnfNegativeCofactor,
                        vEnableVars, pMan, entry)) {
      Vec_PtrPush(vPosUnate, Abc_NtkPi(pNtkCone, entry));
    }
    else if (isNegativeUnate(pSatSolver, pCnfPositiveCofactor, pCnfNegativeCofactor,
                        vEnableVars, pMan, entry)) {
      Vec_PtrPush(vNegUnate, Abc_NtkPi(pNtkCone, entry));
    }
  }
  Aig_ManStop(pMan);
  Cnf_DataFree(pCnfPositiveCofactor);
  Cnf_DataFree(pCnfNegativeCofactor);
  sat_solver_delete(pSatSolver);
  Vec_IntFree(vEnableVars);
}

void writeCnfIntoSolver(Cnf_Dat_t *pCnf, sat_solver *pSat) {
  for (int i = 0; i < pCnf->nClauses; i++) {
    sat_solver_addclause(pSat, pCnf->pClauses[i], pCnf->pClauses[i + 1]);
  }
}

bool isUnate(sat_solver *pSat, Cnf_Dat_t *pCnfPos, Cnf_Dat_t *pCnfNeg,
             Vec_Int_t *pVecEnVars, Aig_Man_t *pMan, int pIndexCi,
             bool pPhase) {
  lit assumptions[Aig_ManCiNum(pMan) + 2 + 2];
  int Entry, i;
  Vec_IntForEachEntry(pVecEnVars, Entry, i) {
    assumptions[i] =
        (i == pIndexCi) ? toLitCond(Entry, 1) : toLitCond(Entry, 0);
  }
  assumptions[Aig_ManCiNum(pMan)] =
      toLitCond(pCnfPos->pVarNums[Aig_ObjId(Aig_ManCi(pMan, pIndexCi))], 0);
  assumptions[Aig_ManCiNum(pMan) + 1] =
      toLitCond(pCnfNeg->pVarNums[Aig_ObjId(Aig_ManCi(pMan, pIndexCi))], 1);
  assumptions[Aig_ManCiNum(pMan) + 2] =
      toLitCond(pCnfPos->pVarNums[Aig_ObjId(Aig_ManCo(pMan, 0))], pPhase);
  assumptions[Aig_ManCiNum(pMan) + 3] =
      toLitCond(pCnfNeg->pVarNums[Aig_ObjId(Aig_ManCo(pMan, 0))], !pPhase);
  if (sat_solver_solve(pSat, assumptions, assumptions + Aig_ManCiNum(pMan) + 4,
                       1000, 0, 0, 0) == -1) {
    return true;
  }
  return false;
}
