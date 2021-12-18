#include "bdd/extrab/extraBdd.h" //
#include "extUtil/util.h"
#include "ssatBooleanSort.h"
#include "ssatelim.h"

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

void ssat_solver_randomelim(ssat_solver *s, Vec_Int_t *pScope,
                            Vec_Int_t *pRandomReverse) {
  int index;
  int entry;
  Vec_IntForEachEntryReverse(pScope, entry, index) {
    Vec_IntPush(pRandomReverse, entry);
  }
  Vec_Int_t *pSort = Vec_IntAlloc(0);
  if (s->useBdd) {
    Cudd_AutodynDisable(s->dd);
  }
  Vec_IntForEachEntry(pRandomReverse, entry, index) {
    Vec_IntPush(pSort, entry);
    if (s->useBdd) {
      DdNode *ptemp = s->bFunc;
      s->bFunc = Util_sortBitonicBDD(s->dd, s->bFunc, pSort);
      Cudd_Ref(s->bFunc);
      Cudd_RecursiveDeref(s->dd, ptemp);
      DdManager *ddNew =
          Cudd_Init(s->dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
      s->bFunc = ssat_BDDReorder(ddNew, s->dd, s->bFunc, entry, 0);
      s->dd = ddNew;
      if (s->verbose) {
        Abc_Print(1, "[DEBUG] %d %d/%d Dag Size of current network: %d\n",
                  entry, index + 1, Vec_IntSize(pRandomReverse),
                  Cudd_DagSize(s->bFunc));
      }
    } else {
      s->pNtk = Util_sortBitonic(s->pNtk, pSort);
      Abc_AigCleanup((Abc_Aig_t *)s->pNtk->pManFunc);
      s->pNtk = Util_NtkDFraig(s->pNtk, 1);
      s->pNtk = Util_NtkDc2(s->pNtk, 1);
      s->pNtk = Util_NtkResyn2(s->pNtk, 1);
      if (s->verbose) {
        Abc_Print(1, "[DEBUG] %d/%d Object Number of current network: %d\n",
                  index + 1, Vec_IntSize(pRandomReverse),
                  Abc_NtkNodeNum(s->pNtk));
      }
    }
  }
  Vec_IntFree(pSort);
}

void ssat_randomCompute(ssat_solver *s, Vec_Int_t *pRandomReverse) {
  if (s->pPerf->fDone)
    return;
  if (s->useBdd) {
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
  unsigned int simulated_result = simulateTrivialCases(pNtk, 0);
  if (bit0_is_1(simulated_result)) {
    if (s->verbose) {
      Abc_Print(1, "F equals const 1\n");
    }
    s->result = 1;
    Vec_IntFree(pRandom);
    return;
  }
  simulated_result = simulateTrivialCases(pNtk, 1);
  if (bit1_is_0(simulated_result)) {
    if (s->verbose) {
      Abc_Print(1, "F equals const 0\n");
    }
    s->result = 0;
    Vec_IntFree(pRandom);
    return;
  }

  Util_MaxInputPatternWithOuputAs0(pNtk, pRandom);
  s->result = ratio_counting(pNtk, pRandom);
  Vec_IntFree(pRandom);
}
