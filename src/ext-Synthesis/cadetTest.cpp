#include "base/abc/abc.h"
#include "bdd/extrab/extraBdd.h"  //
#include "synthesis.h"
#include "synthesisUtil.h"
#include "extSsatelim/ssatelim.h"
// #include "extUtil/util.h"  //

extern "C" {
int Abc_NtkDarCec(Abc_Ntk_t* pNtk1, Abc_Ntk_t* pNtk2, int nConfLimit,
                  int fPartition, int fVerbose);
Abc_Ntk_t* Abc_NtkDC2(Abc_Ntk_t* pNtk, int fBalance, int fUpdateLevel,
                      int fFanout, int fPower, int fVerbose);
}

/**
 * @brief perform exist quantified using BDD
 *
 */
static DdNode* ExistQuantify(DdManager* dd, DdNode* bFunc,
                             Vec_Int_t* pPiIndex) {
  Cudd_Ref(bFunc);
  int* array = Vec_IntArray(pPiIndex);
  int n = Vec_IntSize(pPiIndex);
  DdNode* cube = Cudd_IndicesToCube(dd, array, n);
  Cudd_Ref(cube);
  DdNode* bRet = Cudd_bddExistAbstract(dd, bFunc, cube);
  Cudd_Ref(bRet);
  Cudd_RecursiveDeref(dd, cube);
  Cudd_RecursiveDeref(dd, bFunc);
  Cudd_Deref(bRet);
  return bRet;
}

int cadetTest(char* pFileName, int fVerbose) {
  // NOTE do nothing now because it contains dependency
  // of the abc based ssat solver
  // TODO remove the dependency of ssat solver
  ssat_solver* s = ssat_solver_new();
  ssat_Parser(s, pFileName);

  Abc_Ntk_t* pNtkComp = Abc_NtkDup(s->pNtk);
  DdNode* bFunc = NULL;
  if (!s->dd) {
    Abc_Print(1, "can't build BDD, choose a smaller instance for testing\n");
    return 0;
  } else {
    Abc_Print(1, "Build BDD success\n");
    bFunc = s->bFunc;
  }
  for (int i = Vec_IntSize(s->pQuanType) - 1; i >= 0; i--) {
    QuantifierType type = (QuantifierType)Vec_IntEntry(s->pQuanType, i);
    Vec_Int_t* pScope = (Vec_Int_t*)Vec_PtrEntry(s->pQuan, i);
    if (type == Quantifier_Exist) {
      printf("number of exist variable = %d\n", pScope->nSize);
      bFunc = ExistQuantify(s->dd, bFunc, pScope);
      s->pNtk = Abc_NtkDeriveFromBdd(s->dd, bFunc, NULL, NULL);
      Io_WriteAiger(Abc_NtkStrash(s->pNtk, 0, 0, 0), "golden_exist.aig", 0, 0,
                    0);
      pNtkComp = Abc_NtkDC2(pNtkComp, 0, 0, 1, 0, 0);
      // Abc_Ntk_t* pNtkGolden = Abc_NtkStrash(s->pNtk, 0, 1, 0);
      int entry, index;
      Vec_IntForEachEntry(pScope, entry, index) {
        Vec_IntSetEntry(pScope, index, entry+1);
      }
      Abc_Ntk_t* pNtkNew = cadetExistsElim(pNtkComp, pScope);
      Abc_NtkShortNames(s->pNtk);
      Abc_NtkShortNames(pNtkNew);
      printf("checking if two circuits are equivalent...\n");
      Abc_NtkDarCec(s->pNtk, pNtkNew, 100000, 0, 0);
      Abc_NtkDelete(pNtkNew);
      break;
    }
  }
  Abc_NtkDelete(pNtkComp);
  ssat_solver_delete(s);
  return 0;
}
