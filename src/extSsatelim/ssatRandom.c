// #include "bdd/cudd/cudd.h"
#include "base/abc/abc.h"
#include "bdd/cudd/cudd.h"
#include "bdd/extrab/extraBdd.h" //
#include "extUtil/fasttime.h"
#include "extUtil/util.h"
#include "ssatBooleanSort.h"
#include "ssatelim.h"

extern Abc_Ntk_t *Abc_NtkFromGlobalBdds(Abc_Ntk_t *pNtk, int fReverse);
extern Abc_Obj_t *Abc_NodeBddToMuxes_rec(DdManager *dd, DdNode *bFunc,
                                         Abc_Ntk_t *pNtkNew,
                                         st__table *tBdd2Node);

static int bit0_is_1(unsigned int bitset) {
  return (Util_BitSet_getValue(&bitset, 0) == 1);
}
static int bit1_is_0(unsigned int bitset) {
  return (Util_BitSet_getValue(&bitset, 0) == 0);
}

static void Model_setPos1ValueAs1(Abc_Ntk_t *pNtk) {
  int i;
  Abc_Obj_t *pNode;
  // set [1] bit as 1
  Abc_NtkForEachCi(pNtk, pNode, i)
      Util_BitSet_setValue((unsigned int *)(&pNtk->pModel[i]), 1, 1);
}

static double ratio_counting(Abc_Ntk_t *pNtk, Vec_Int_t *pRandom) {
  // Assume that max pattern where ntk output is 0, are 0b1011,
  // then (~(0b1011))/0b10000 is that ratio of 1's
  double ret = 0;
  int index;
  int entry;
  double count = 1;
  double total = pow(2, Vec_IntSize(pRandom));
  int i = 0;
  Vec_IntForEachEntryReverse(pRandom, entry, index) {
    const int pModelValue = pNtk->pModel[entry];
    if (pModelValue) {
      count += pow(2, i);
    }
    i++;
  }
  ret = (total - count) / total;
  return ret;
}

static unsigned int simulateTrivialCases(Abc_Ntk_t *pNtk, int value) {
  // trivial cases
  // set [0] bit as 0
  if (!value) {
    Util_SetCiModelAs0(pNtk);
  } else {
    Util_SetCiModelAs1(pNtk);
  }

  return Util_NtkSimulate(pNtk);
}

static Abc_Ntk_t *ssat_buildNtkFromBdd(Abc_Ntk_t *pNtk, DdManager *dd,
                                       DdNode *bFunc) {
  Abc_Ntk_t *pNtkNew = Abc_NtkStartFrom(pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP);
  st__table *tBdd2Node;
  Abc_Obj_t *pObjNew;
  int i;
  tBdd2Node = st__init_table(st__ptrcmp, st__ptrhash);
  Abc_NtkForEachCi(pNtkNew, pObjNew, i)
      st__insert(tBdd2Node, (char *)Cudd_bddIthVar(dd, i), (char *)pObjNew);
  // DdNode * aFunc = Cudd_BddToAdd( dd, bFunc );  Cudd_Ref( aFunc );
  pObjNew = Abc_NodeBddToMuxes_rec(dd, bFunc, pNtkNew, tBdd2Node);
  Abc_ObjAddFanin(Abc_NtkPo(pNtk, 0)->pCopy, pObjNew);
  st__free_table(tBdd2Node);
  // Cudd_RecursiveDeref( dd, aFunc );
  return pNtkNew;
}

void ssat_solver_randomelim(ssat_solver *s, Vec_Int_t *pScope,
                            Vec_Int_t *pRandomReverse) {
  int index;
  int entry;
  int n = Vec_IntSize(pRandomReverse);
  Vec_IntForEachEntryReverse(pScope, entry, index) {
    Vec_IntPush(pRandomReverse, entry);
  }
  Vec_Int_t *pSort = Vec_IntAlloc(0);
  if (s->haveBdd) {
    Cudd_AutodynDisable(s->dd);
  }
  Vec_IntForEachEntry(pRandomReverse, entry, index) {
    Vec_IntPush(pSort, entry);
    if (index < n)
      continue;
    if (s->haveBdd) {
      s->bFunc = Util_sortBitonicBDD(s->dd, s->bFunc, pSort);
      Cudd_Ref(s->bFunc);
      if (s->useReorder) {
        DdManager *ddNew =
            Cudd_Init(s->dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        s->bFunc = ssat_BDDReorder(ddNew, s->dd, s->bFunc, entry, 0);
        s->dd = ddNew;
      }
      if (s->verbose) {
        Abc_Print(1, "[DEBUG] %d %d/%d Dag Size of current network: %d\n",
                  entry, index + 1, Vec_IntSize(pRandomReverse),
                  Cudd_DagSize(s->bFunc));
      }
    } else {
      fasttime_t start = gettime();
      s->pNtk = Util_sortBitonic(s->pNtk, pSort);
      fasttime_t end = gettime();
      if (s->verbose) {
        printf("time spend on sorting : %lf\n", tdiff(start, end));
      }
      Abc_AigCleanup((Abc_Aig_t *)s->pNtk->pManFunc);
      s->pNtk = Util_NtkDFraig(s->pNtk, 1, 1);
      // s->pNtk = Util_NtkGiaFraig(s->pNtk, 1);
      s->pNtk = Util_NtkDc2(s->pNtk, 1);
      s->pNtk = Util_NtkResyn2rs(s->pNtk, 1);
      if (s->verbose) {
        Abc_Print(1, "[DEBUG] %d/%d Object Number of current network: %d\n",
                  index + 1, Vec_IntSize(pRandomReverse),
                  Abc_NtkNodeNum(s->pNtk));
      }
    }
  }
  Vec_IntFree(pSort);
}

static int ssat_solve_append(Abc_Ntk_t *pNtk, Vec_Int_t *pRandom,
                             int fVerbose) {
  int entry, index;
  Abc_Obj_t *pObj;
  Abc_Ntk_t *pNtkNew = Abc_NtkStartFrom(pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG);
  Abc_NtkForEachPi(pNtk, pObj, index) {
    pObj->pCopy = Abc_NtkPi(pNtkNew, index);
  }
  Vec_IntForEachEntry(pRandom, entry, index) {
    // Abc_NtkPi(pNtk, entry)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkPi(pNtk, entry)->pCopy =
        pNtk->pModel[entry] ? Abc_AigConst1(pNtkNew) : Abc_AigConst0(pNtkNew);
  }
  Abc_Obj_t *pPo = Util_NtkAppend(pNtkNew, pNtk);
  if (Abc_ObjIsComplement(Abc_NtkPo(pNtk, 0))) {
    pPo = Abc_ObjNot(pPo);
  }
  Abc_ObjAddFanin(Abc_NtkPo(pNtkNew, 0), pPo);
  int res = Util_NtkSat(pNtkNew, 0);
  return res;
}

static double ssat_solve_randomexist(Abc_Ntk_t *pNtk, Vec_Int_t *pRandom,
                                     int fVerbose) {
  // trivial cases
  int entry, index;
  Util_SetCiModelAs0(pNtk);
  if (!ssat_solve_append(pNtk, pRandom, fVerbose)) {
    return 1;
  }
  Vec_IntForEachEntry(pRandom, entry, index) {
    pNtk->pModel[entry] = 1;
    pNtk->pModel[entry] =
        ssat_solve_append(pNtk, pRandom, fVerbose) == 0 ? 0 : 1;
    if (fVerbose) {
      printf("index: %d/%d: %s\n", index + 1, Vec_IntSize(pRandom),
             pNtk->pModel[entry] == 0 ? "SAT" : "UNSAT");
    }
  }
  return ratio_counting(pNtk, pRandom);
}

void ssat_randomCompute(ssat_solver *s, Vec_Int_t *pRandomReverse,
                        int fExists) {
  if (s->pPerf->fDone)
    return;
  if (s->haveBdd) {
    s->pNtk = Abc_NtkDeriveFromBdd(s->dd, s->bFunc, NULL, NULL);
    s->pNtk = Util_NtkStrash(s->pNtk, 1);
  }
  // Abc_NtkCheck(s->pNtk);
  Vec_Int_t *pRandom = Vec_IntAlloc(0);
  {
    int index;
    int entry;
    Vec_IntForEachEntryReverse(pRandomReverse, entry, index) {
      Vec_IntPush(pRandom, entry);
    }
  }
  Abc_Ntk_t *pNtk = s->pNtk;
  if (!Abc_NtkIsStrash(s->pNtk))
    pNtk = Abc_NtkStrash(s->pNtk, 0, 0, 0);

  Util_NtkModelAlloc(pNtk);
  if (!fExists) {
    unsigned int simulated_result = simulateTrivialCases(pNtk, 0);
    if (bit0_is_1(simulated_result)) {
      if (s->verbose) {
        Abc_Print(1, "F equals const 1\n");
      }
      s->result = 1;
      Vec_IntFree(pRandom);
      return;
    }
    Util_MaxInputPatternWithOuputAs0(pNtk, pRandom);
    s->result = ratio_counting(pNtk, pRandom);
  } else {
    if (s->verbose)
      printf("solve by projected model counting...\n");
    s->result = ssat_solve_randomexist(pNtk, pRandom, s->verbose);
  }
  Vec_IntFree(pRandom);
}
