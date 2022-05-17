#include "extUtil/easylogging++.h"
#include "extUtil/utilVec.h"
#include "ssatParser.h"
#include "extUtil/easylogging++.h"

ABC_NAMESPACE_IMPL_START

extern "C" {
Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *, int, int);
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
                       500, 0, 0, 0) == -1) {
    return true;
  }
  return false;
}

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

static void writeCnfIntoSolver(Cnf_Dat_t *pCnf, sat_solver *pSat) {
  for (int i = 0; i < pCnf->nClauses; i++) {
    sat_solver_addclause(pSat, pCnf->pClauses[i], pCnf->pClauses[i + 1]);
  }
}

static void unateDetection(Abc_Ntk_t *pNtk, Vec_Int_t *pScope,
                           Vec_Ptr_t *vPosUnate, Vec_Ptr_t *vNegUnate) {
  int entry, index;
  Aig_Man_t *pMan = Abc_NtkToDar(pNtk, 0, 0);
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
  Vec_IntForEachEntry(pScope, entry, index) {
    VLOG(1) << index + 1 << "/" << Vec_IntSize(pScope);
    if (isPositiveUnate(pSatSolver, pCnfPositiveCofactor, pCnfNegativeCofactor,
                        vEnableVars, pMan, entry - 1)) {
      Vec_PtrPush(vPosUnate, Abc_NtkPi(pNtk, entry - 1));
      continue;
    } else if (isNegativeUnate(pSatSolver, pCnfPositiveCofactor,
                               pCnfNegativeCofactor, vEnableVars, pMan,
                               entry - 1)) {
      Vec_PtrPush(vNegUnate, Abc_NtkPi(pNtk, entry - 1));
    }
  }
  Aig_ManStop(pMan);
  Cnf_DataFree(pCnfPositiveCofactor);
  Cnf_DataFree(pCnfNegativeCofactor);
  sat_solver_delete(pSatSolver);
  Vec_IntFree(vEnableVars);
}

void ssatParser::unatePreprocess() {
  val_assert_msg(Vec_IntSize(_quanType) != 0 && Vec_PtrSize(_quanBlock) != 0,
                 "no quantifier to be processed!");
  val_assert_msg(Vec_PtrSize(_clauseSet) != 0, "no clause to be processed!");
  VLOG(1) << "perform unate detection";
  Abc_Ntk_t *pNtkNew = Abc_NtkStartFrom(_solver->pNtk, _solver->pNtk->ntkType,
                                        _solver->pNtk->ntkFunc);
  Vec_Int_t *pScope = Vec_IntAlloc(0);
  Vec_Int_t *vVec;
  Vec_Ptr_t *vPosUnate = Vec_PtrAlloc(0);
  Vec_Ptr_t *vNegUnate = Vec_PtrAlloc(0);
  Abc_Obj_t *pObj;
  int entry, index;
  Vec_PtrForEachEntry(Vec_Int_t *, _quanBlock, vVec, index) {
    if (Vec_IntEntry(_quanType, index) == Quantifier_Exist) {
      int j;
      Vec_IntForEachEntry(vVec, entry, j) { Vec_IntPush(pScope, entry); }
    }
  }
  unateDetection(_solver->pNtk, pScope, vPosUnate, vNegUnate);
  VLOG(1) << "number of positive unate: " << Vec_PtrSize(vPosUnate);
  VLOG(1) << "number of negative unate: " << Vec_PtrSize(vNegUnate);
  Abc_AigConst1(_solver->pNtk)->pCopy = Abc_AigConst1(pNtkNew);
  Abc_NtkForEachPi(_solver->pNtk, pObj, index) {
    pObj->pCopy = Abc_NtkPi(pNtkNew, index);
  }
  Abc_NtkForEachCo(_solver->pNtk, pObj, index) {
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

  Abc_AigForEachAnd(_solver->pNtk, pObj, index) {
    pObj->pCopy = Abc_AigAnd((Abc_Aig_t *)pNtkNew->pManFunc,
                             Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj));
  }
  // relink the CO nodes
  Abc_NtkForEachCo(_solver->pNtk, pObj, index) {
    Abc_ObjAddFanin(pObj->pCopy, Abc_ObjChild0Copy(pObj));
  }
  Vec_PtrFree(vPosUnate);
  Vec_PtrFree(vNegUnate);
  Abc_NtkDelete(_solver->pNtk);
  _solver->pNtk = pNtkNew;
}
