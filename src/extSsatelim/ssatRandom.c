#include "ssatelim.h"
#include "ssatBooleanSort.h"
#include "extUtil/util.h"

// extSsatelim/ssatBooleanSort.cc
extern DdNode *RandomQuantify(DdManager *dd, DdNode *bFunc, Vec_Int_t *pScope);
extern DdNode *RandomQuantifyReverse(DdManager **dd, DdNode *bFunc,
                                     Vec_Int_t *pScopeReverse);



static int bit0_is_1(unsigned int bitset) {
  return (Util_BitSet_getValue(&bitset, 0) == 1);
}
static int bit1_is_0(unsigned int bitset) {
  return (Util_BitSet_getValue(&bitset, 1) == 0);
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
  Vec_IntForEachEntryReverse(pRandom, entry, index) {
    const int pModelValue = pNtk->pModel[entry];
    if (!pModelValue) {
      ret += 1;
    }
    ret /= (double)2;
  }
  return ret;
}

static unsigned int simulateTrivialCases(Abc_Ntk_t *pNtk) {
  // trivial cases
  // set [0] bit as 0
  Util_SetCiModelAs0(pNtk);
  // set [1] bit as 1
  Model_setPos1ValueAs1(pNtk);

  return Util_NtkSimulate(pNtk);
}

static Abc_Ntk_t *random_eliminate_scope(Abc_Ntk_t *pNtk, Vec_Int_t *pScopeReverse) {
  return Util_FunctionSortSeq(pNtk, pScopeReverse);
}

void ssat_solver_randomelim(ssat_solver *s, Vec_Int_t *pScope,
                            Vec_Int_t *pRandomReverse) {
  int index;
  int entry;
  Vec_IntForEachEntryReverse(pScope, entry, index) {
    Vec_IntPush(pRandomReverse, entry);
  }
  if (s->useBdd) {
    s->bFunc = RandomQuantifyReverse(&s->dd, s->bFunc, pRandomReverse);
  } else
    s->pNtk = random_eliminate_scope(s->pNtk, pRandomReverse);
}

void ssat_randomCompute(ssat_solver *s, Vec_Int_t *pRandomReverse) {
  if (s->useBdd) {
    s->pNtk = Abc_NtkDeriveFromBdd(s->dd, s->bFunc, NULL, NULL);
    s->pNtk = Util_NtkStrash(s->pNtk, 1);
  }
  Abc_NtkCheck(s->pNtk);
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
  unsigned int simulated_result = simulateTrivialCases(pNtk);
  if (bit0_is_1(simulated_result)) {
    if (s->verbose) {
      Abc_Print(1, "F equals const 1\n");
    }
    s->result = 1;
    Vec_IntFree(pRandom);
    return;
  }
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
