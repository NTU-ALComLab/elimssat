/** src/extSsatelim/ssatelim.c *****************************************
  FileName    [ssatelim.c]
  SystemName  []
  PackageName []
  Synopsis    []
  Author      [Kuan-Hua Tu]

  Affiliation []
  Date        [Sun Jun 23 02:39:25 CST 2019]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "ssatelim.h"
#include "base/abc/abc.h"
#include "base/io/ioAbc.h"
#include "ext-Synthesis/synthesis.h"
#include "extUtil/util.h"
#include "misc/vec/vecFlt.h"
#include "ssatBooleanSort.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static const lbool l_Unsupp = -2;

// bdd/extrab/extraBdd.h
extern Abc_Ntk_t *Abc_NtkFromGlobalBdds(Abc_Ntk_t *pNtk, int fReverse);
extern int Abc_NtkMinimumBase2(Abc_Ntk_t *pNtk);

// extSsatelim/ssatBooleanSort.cc
extern DdNode *RandomQuantify(DdManager *dd, DdNode *bFunc, Vec_Int_t *pScope);
extern DdNode *RandomQuantifyReverse(DdManager **dd, DdNode *bFunc,
                                     Vec_Int_t *pScopeReverse);

// extUtil/util.c
extern DdNode *ExistQuantify(DdManager *dd, DdNode *bFunc, Vec_Int_t *pPiIndex);

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

static unsigned int simulateTrivialCases(Abc_Ntk_t *pNtk) {
  // trivial cases
  // set [0] bit as 0
  Util_SetCiModelAs0(pNtk);
  // set [1] bit as 1
  Model_setPos1ValueAs1(pNtk);

  return Util_NtkSimulate(pNtk);
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

static void ssat_build_bdd(ssat_solver* s) {
  int fVerbose = 0;
  int fReorder = 1;
  int fReverse = 0;
  int fBddSizeMin = 10240;
  int fBddSizeMax = (Abc_NtkNodeNum(s->pNtk) < fBddSizeMin)
                        ? fBddSizeMin
                        : Abc_NtkNodeNum(s->pNtk) * 1.2;
  Abc_Print(1, "Abc_NtkNodeNum(s->pNtk): %d  , BDD limit: %d\n",
            Abc_NtkNodeNum(s->pNtk), fBddSizeMax);
  s->dd = (DdManager *)Abc_NtkBuildGlobalBdds(s->pNtk, fBddSizeMax, 1, fReorder,
                                              fReverse, fVerbose);
  s->bFunc = NULL;
  if (s->dd == NULL) {
    s->useBdd = 0;
  } else {
    Abc_Print(1, "[INFO] Build BDD success\n");
    Abc_Print(1, "[INFO] Use BDD based Solving\n");
    s->useBdd = 1;
    s->bFunc = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkPo(s->pNtk, 0));
  }
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************
  Synopsis    [init function]
  Description []

  SideEffects []
  SeeAlso     []
***********************************************************************/
int ssatelim_init_function() { return 0; }

static inline int VarId_to_PiIndex(int varId) { return varId - 1; }

static inline int PiIndex_to_VarId(int piIndex) { return piIndex + 1; }

static void ssat_synthesis(ssat_solver *s) {
  Abc_Print(1, "[INFO] Perform Synthesis...\n");
  Abc_Print(1, "[INFO] Object Number before Synthesis: %d\n",
            Abc_NtkNodeNum(s->pNtk));
  s->pNtk = Util_NtkDc2(s->pNtk, 1);
  s->pNtk = Util_NtkResyn2(s->pNtk, 1);
  Abc_Print(1, "[INFO] Object Number after Synthesis: %d\n",
            Abc_NtkNodeNum(s->pNtk));
}

ssat_solver *ssat_solver_new() {
  ssat_solver *s = (ssat_solver *)ABC_CALLOC(char, sizeof(ssat_solver));
  s->pNtk = Util_NtkAllocStrashWithName("ssat");
  Abc_NtkCreatePo(s->pNtk);
  Abc_ObjAssignName(Abc_NtkPo(s->pNtk, 0), "out", NULL);
  s->pPo = Abc_AigConst1(s->pNtk);
  s->pQuan = Vec_PtrAlloc(0);
  s->pQuanType = Vec_IntAlloc(0);
  s->pUnQuan = Vec_IntAlloc(0);
  s->pQuanWeight = Vec_FltAlloc(0);
  s->result = -1;
  s->useBdd = 1;
  s->doSynthesis = 1;
  s->useManthan = 1;
  s->useCadet = 0;
  s->verbose = 0;
  return s;
}

double ssat_result(ssat_solver *s) { return s->result; }

void ssat_solver_delete(ssat_solver *s) {
  if (s) {
    Abc_NtkDelete(s->pNtk);
    Vec_IntPtrFree(s->pQuan);
    Vec_IntFree(s->pQuanType);
    Vec_IntFree(s->pUnQuan);
    ABC_FREE(s);
  }
}

void ssat_solver_setnvars(ssat_solver *s, int n) {
  if (Abc_NtkPiNum(s->pNtk) < n) {
    Util_NtkCreateMultiPi(s->pNtk, n - Abc_NtkPiNum(s->pNtk));
  }
}

int ssat_addScope(ssat_solver *s, int *begin, int *end, QuantifierType type) {
  Vec_Int_t *pScope;
  if (Vec_IntSize(s->pQuanType) && Vec_IntEntryLast(s->pQuanType) == type) {
    pScope = (Vec_Int_t *)Vec_PtrEntryLast(s->pQuan);
  } else {
    pScope = Vec_IntAlloc(0);
    Vec_PtrPush(s->pQuan, pScope);
    Vec_IntPush(s->pQuanType, type);
  }

  for (int *it = begin; it < end; it++) {
    Vec_IntPush(pScope, VarId_to_PiIndex(*it));
  }
  return 1;
}

int ssat_addexistence(ssat_solver *s, int *begin, int *end) {
  Vec_FltPush(s->pQuanWeight, -1);
  return ssat_addScope(s, begin, end, Quantifier_Exist);
}

int ssat_addforall(ssat_solver *s, int *begin, int *end) {
  Vec_FltPush(s->pQuanWeight, -1);
  return ssat_addScope(s, begin, end, Quantifier_Forall);
}

int ssat_addrandom(ssat_solver *s, int *begin, int *end, double prob) {
  Vec_FltPush(s->pQuanWeight, prob);
  if (prob != 0.5) {
    Abc_Print(-1, "Probility of random var is not 0.5\n");
  }
  return ssat_addScope(s, begin, end, Quantifier_Random);
}

int ssat_addclause(ssat_solver *s, lit *begin, lit *end) {
  Abc_Obj_t *pObj = Abc_AigConst0(s->pNtk);
  for (lit *it = begin; it < end; it++) {
    int var = lit_var(*it);
    int sign = lit_sign(*it);
    Abc_Obj_t *pPi = Abc_NtkPi(s->pNtk, VarId_to_PiIndex(var));
    if (sign) {
      pPi = Abc_ObjNot(pPi);
    }
    pObj = Util_AigNtkOr(s->pNtk, pObj, pPi);
  }
  s->pPo = Util_AigNtkAnd(s->pNtk, s->pPo, pObj);
  return 1;
}

//=================================================

Abc_Ntk_t *existence_eliminate_scope_bdd(Abc_Ntk_t *pNtk, Vec_Int_t *pScope,
                                         int verbose) {
  Abc_Print(1, "existence_eliminate_scope_bdd\n");
  Abc_Ntk_t *pNtkNew = Util_NtkExistQuantifyPis_BDD(pNtk, pScope);
  Abc_Print(1, "Abc_NtkIsBddLogic(pNtk): %d\n", Abc_NtkIsBddLogic(pNtk));
  return pNtkNew;
}

Abc_Ntk_t *existence_eliminate_scope_aig(Abc_Ntk_t *pNtk, Vec_Int_t *pScope,
                                         int verbose) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Print(1, "[INFO] perform expansion on aig\n");
  Abc_Ntk_t *pNtkNew = Util_NtkExistQuantifyPis(pNtk, pScope);
  return pNtkNew;
}

Abc_Ntk_t *existence_eliminate_scope(Abc_Ntk_t *pNtk, Vec_Int_t *pScope,
                                     int verbose) {
  if (Abc_NtkIsBddLogic(pNtk))
    return existence_eliminate_scope_bdd(pNtk, pScope, verbose);
  if (Abc_NtkIsStrash(pNtk))
    return existence_eliminate_scope_aig(pNtk, pScope, verbose);

  pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
  Abc_Ntk_t *pNtkNew = existence_eliminate_scope_aig(pNtk, pScope, verbose);
  Abc_NtkDelete(pNtk);
  return pNtkNew;
}

Abc_Ntk_t *random_eliminate_scope(Abc_Ntk_t *pNtk, Vec_Int_t *pScopeReverse) {
  return Util_FunctionSortSeq(pNtk, pScopeReverse);
}

double compute_random(Abc_Ntk_t *pNtk, Vec_Int_t *pRandom) {
  if (!Abc_NtkIsStrash(pNtk))
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);

  Util_NtkModelAlloc(pNtk);
  int verbose = 0;
  unsigned int simulated_result = simulateTrivialCases(pNtk);
  if (bit0_is_1(simulated_result)) {
    if (verbose) {
      Abc_Print(1, "F equals const 1\n");
    }
    return 1;
  }
  if (bit1_is_0(simulated_result)) {
    if (verbose) {
      Abc_Print(1, "F equals const 0\n");
    }
    return 0;
  }

  Util_MaxInputPatternWithOuputAs0(pNtk, pRandom);
  return ratio_counting(pNtk, pRandom);
}

//=================================================
void ssat_quatification_check(ssat_solver *s) {
  Vec_Int_t *pPi = Vec_IntAlloc(Abc_NtkPiNum(s->pNtk));
  Vec_IntFill(pPi, Abc_NtkPiNum(s->pNtk), 0);

  for (int i = 0; i < Vec_PtrSize(s->pQuan); i++) {
    Vec_Int_t *pScope = (Vec_Int_t *)Vec_PtrEntry(s->pQuan, i);
    for (int j = 0; j < Vec_IntSize(pScope); j++) {
      int varId = Vec_IntEntry(pScope, j);
      Vec_IntWriteEntry(pPi, varId, 1);
    }
  }
  for (int i = 0; i < Vec_IntSize(pPi); i++) {
    int mark = Vec_IntEntry(pPi, i);
    if (!mark)
      Vec_IntPush(s->pUnQuan, i);
  }
}

void ssat_parser_finished_process(ssat_solver *s) {
  // Connect the output
  Abc_ObjAddFanin(Abc_NtkPo(s->pNtk, 0), s->pPo);
  s->pNtk = Util_NtkStrash(s->pNtk, 1);
  Abc_NtkCheck(s->pNtk);
  ssat_quatification_check(s);
  // try building BDD
  ssat_build_bdd(s);
}

void ssat_solver_existelim(ssat_solver *s, Vec_Int_t *pScope) {
  Abc_Print(1, "[INFO] expand on existential variable\n");
  if (s->useBdd)
    s->bFunc = ExistQuantify(s->dd, s->bFunc, pScope);
  else if (s->useManthan) {
    Vec_Int_t *pScopeNew = Vec_IntAlloc(pScope->nSize);
    int entry, index;
    // change PI index into object index
    Vec_IntForEachEntry(pScope, entry, index) {
      Vec_IntPush(pScopeNew, Abc_NtkPi(s->pNtk, entry)->Id);
    }
    s->pNtk = manthanExistsElim(s->pNtk, pScopeNew);
    Vec_IntFree(pScopeNew);
  } else if (s->useCadet) {
    Vec_Int_t *pScopeNew = Vec_IntAlloc(pScope->nSize);
    int entry, index;
    // change PI index into object index
    Vec_IntForEachEntry(pScope, entry, index) {
      Vec_IntPush(pScopeNew, Abc_NtkPi(s->pNtk, entry)->Id);
    }
    s->pNtk = cadetExistsElim(s->pNtk, pScopeNew);
    Vec_IntFree(pScopeNew);
  } else
    s->pNtk = existence_eliminate_scope(s->pNtk, pScope, s->verbose);
}

void ssat_solver_randomelim(ssat_solver *s, Vec_Int_t *pScope, Vec_Int_t *pRandomReverse) {
  Abc_Print(1, "[INFO] expand on randomize variable\n");
  int index;
  int entry;
  Vec_IntForEachEntryReverse(pScope, entry, index) {
    Vec_IntPush(pRandomReverse, entry);
  }
  if (s->useBdd) {
    s->bFunc = RandomQuantifyReverse(&s->dd, s->bFunc, pRandomReverse);
  }
  else
    s->pNtk = random_eliminate_scope(s->pNtk, pRandomReverse);
}

//=================================================
int ssat_solver_solve2(ssat_solver *s) {
  ssat_parser_finished_process(s);
  Vec_Int_t *pRandomReverse = Vec_IntAlloc(0);
  Abc_Print(1, "#level: %d\n", Vec_IntSize(s->pQuanType));

  // perform quantifier elimination
  for (int i = Vec_IntSize(s->pQuanType) - 1; i >= 0; i--) {
    QuantifierType type = Vec_IntEntry(s->pQuanType, i);
    Vec_Int_t *pScope = (Vec_Int_t *)Vec_PtrEntry(s->pQuan, i);
    Abc_Print(1, "level %d: %d element\n", i, Vec_IntSize(pScope));
    if (type == Quantifier_Exist) {
      ssat_solver_existelim(s, pScope);
    } else {
      ssat_solver_randomelim(s, pScope, pRandomReverse);
    }
    if (!s->useBdd) {
      ssat_build_bdd(s);
    }
    if (s->doSynthesis && !s->useBdd) {
      ssat_synthesis(s);
    }
  }
  if (Vec_IntSize(s->pUnQuan)) {
    Abc_Print(1, "[INFO] expand on unquantified variable\n");
    if (s->useBdd)
      s->bFunc = ExistQuantify(s->dd, s->bFunc, s->pUnQuan);
    else
      s->pNtk = existence_eliminate_scope(s->pNtk, s->pUnQuan, s->verbose);
  }

  if (s->useBdd) {
    s->pNtk = Abc_NtkDeriveFromBdd(s->dd, s->bFunc, NULL, NULL);
    s->pNtk = Util_NtkStrash(s->pNtk, 1);
  }

  printf("[INFO] expansion done, start model counting...\n");
  Vec_Int_t *pRandom = Vec_IntAlloc(0);
  {
    int index;
    int entry;
    Vec_IntForEachEntryReverse(pRandomReverse, entry, index) {
      Vec_IntPush(pRandom, entry);
    }
  }
  Abc_NtkCheck(s->pNtk);
  s->result = compute_random(s->pNtk, pRandom);
  // Abc_Print( 1, "result: %lf\n", s->result );
  Vec_IntFree(pRandom);
  Vec_IntFree(pRandomReverse);
  return l_True;
}

void ssat_main(char *filename, int fVerbose) {
  ssat_solver *s = ssat_solver_new();
  s->verbose = fVerbose;
  char preprocess_cmd[128];
  // TODO Change temp file name
  sprintf(preprocess_cmd, "python3 wmcrewriting2.py %s tmp/temp.sdimacs",
          filename);
  system(preprocess_cmd);

  // open file
  ssat_Parser(s, "tmp/temp.sdimacs");
  // ssat_Parser(s, filename);
  int result = ssat_solver_solve2(s);

  Abc_Print(1, "c ssat solving in ABC\n");
  Abc_Print(1, "#var: %d\n", Abc_NtkPiNum(s->pNtk));

  if (result == l_False) {
    // Abc_Print( 1, "s UNSATISFIABLE\n" );
    Abc_Print(1, "s UNSATISFIABLE\n");
    Abc_Print(1, "  > Upper bound = %e\n", s->result);
    Abc_Print(1, "  > Lower bound = %e\n", s->result);
  } else if (result == l_True) {
    // print probabilty
    Abc_Print(1, "s SATISFIABLE\n");
    Abc_Print(1, "  > Upper bound = %e\n", s->result);
    Abc_Print(1, "  > Lower bound = %e\n", s->result);

  } else if (result == l_Undef) {
    Abc_Print(1, "s UNKNOWN\n");
  } else if (result == l_Unsupp) {
    Abc_Print(1, "s UNSUPPORT\n");
  }
  ssat_solver_delete(s);
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
