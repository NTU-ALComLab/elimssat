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
#include "ext-Synthesis/synthesisUtil.h"
#include "extUtil/util.h"
#include "misc/vec/vecFlt.h"
#include "ssatBooleanSort.h"
#include <zlib.h>

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static const lbool l_Unsupp = -2;

// bdd/extrab/extraBdd.h
extern Abc_Ntk_t *Abc_NtkFromGlobalBdds(Abc_Ntk_t *pNtk, int fReverse);
extern int Abc_NtkMinimumBase2(Abc_Ntk_t *pNtk);
Abc_Ntk_t *Abc_NtkDC2(Abc_Ntk_t *pNtk, int fBalance, int fUpdateLevel,
                      int fFanout, int fPower, int fVerbose);

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

static void ssat_check_const(ssat_solver *s) {
  Abc_Obj_t *pObj, *pPo;
  if (!s->useBdd) {
    pPo = Abc_NtkPo(s->pNtk, 0);
    pObj = Abc_ObjFanin0(pPo);
    if (Abc_AigNodeIsConst(pObj)) {
      if (Abc_ObjFaninC0(pPo)) {
        printf("CONST 0!\n");
      } else {
        printf("CONST 1!\n");
      }
      exit(0);
    }
  }
}

static void ssat_build_bdd(ssat_solver *s) {
  int fVerbose = 0;
  int fReorder = 1;
  int fReverse = 0;
  int fBddSizeMin = 10240;
  int fBddSizeMax = (Abc_NtkNodeNum(s->pNtk) < fBddSizeMin)
                        ? fBddSizeMin
                        : Abc_NtkNodeNum(s->pNtk) * 1.2;
  if (Abc_NtkNodeNum(s->pNtk) > 10000) {
    s->dd = (DdManager *)Abc_NtkBuildGlobalBdds(s->pNtk, fBddSizeMax, 1,
                                                fReorder, fReverse, fVerbose);
  } else {
    s->dd = (DdManager *)Abc_NtkBuildGlobalBdds(s->pNtk, 100000, 1, fReorder,
                                                fReverse, fVerbose);
  }
  s->bFunc = NULL;
  if (s->dd == NULL) {
    if (s->verbose)
      Abc_Print(1, "> Build BDD failed\n");
    s->useBdd = 0;
  } else {
    s->useBdd = 1;
    s->bFunc = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkCo(s->pNtk, 0));
    if (s->verbose) {
      Abc_Print(1, "> Build BDD success\n");
      Abc_Print(1, "> Use BDD based Solving\n");
      Abc_Print(1, "> Object Number of current network: %d\n",
                Cudd_DagSize(s->bFunc));
    }
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
  if (s->verbose) {
    Abc_Print(1, "> Perform Synthesis...\n");
    Abc_Print(1, "> Object Number before Synthesis: %d\n",
              Abc_NtkNodeNum(s->pNtk));
  }
  s->pNtk = Util_NtkDc2(s->pNtk, 1);
  s->pNtk = Util_NtkResyn2(s->pNtk, 1);
  s->pNtk = Util_NtkDFraig(s->pNtk, 1);
  if (s->verbose) {
    Abc_Print(1, "> Object Number after Synthesis: %d\n",
              Abc_NtkNodeNum(s->pNtk));
  }
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
  s->varPiIndex = Vec_IntAlloc(0);
  s->piVarIndex = Vec_IntAlloc(0);
  s->result = -1;
  s->useBdd = 0;
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
  Abc_Obj_t *pObj;

  Abc_NtkIncrementTravId(s->pNtk);
  int entry, index;
  for (int i = 0; i < Vec_PtrSize(s->pQuan); i++) {
    Vec_Int_t *pScope = (Vec_Int_t *)Vec_PtrEntry(s->pQuan, i);
    Vec_IntForEachEntry(pScope, entry, index) {
      Abc_NodeSetTravIdCurrent(Abc_NtkPi(s->pNtk, entry));
    }
  }
  Abc_NtkForEachPi(s->pNtk, pObj, index) {
    if (!Abc_NodeIsTravIdCurrent(pObj)) {
      Vec_IntPush(s->pUnQuan, PiIndex_to_VarId(index));
    }
  }
}

void ssat_parser_finished_process(ssat_solver *s) {
  // Connect the output
  Abc_ObjAddFanin(Abc_NtkPo(s->pNtk, 0), s->pPo);
  s->pNtk = Util_NtkStrash(s->pNtk, 1);
  Abc_NtkCheck(s->pNtk);
  ssat_quatification_check(s);
  // try building BDD
  ssat_synthesis(s);
  ssat_build_bdd(s);
}

void ssat_solver_existelim(ssat_solver *s, Vec_Int_t *pScope) {
  if (s->verbose) {
    Abc_Print(1, "> expand on existential variable\n");
  }
  if (s->useBdd)
    s->bFunc = ExistQuantify(s->dd, s->bFunc, pScope);
  else if (s->useManthan) {
    Vec_Int_t *pScopeNew = Vec_IntAlloc(pScope->nSize);
    int entry, index;
    // change PI index into object index
    Vec_IntForEachEntry(pScope, entry, index) {
      Vec_IntPush(pScopeNew, Abc_NtkPi(s->pNtk, entry)->Id);
    }
    s->pNtk = manthanExistsElim(s->pNtk, pScopeNew, s->verbose);
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

void ssat_solver_randomelim(ssat_solver *s, Vec_Int_t *pScope,
                            Vec_Int_t *pRandomReverse) {
  if (s->verbose) {
    Abc_Print(1, "> expand on randomize variable\n");
  }
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

void ssat_solver_existouter(ssat_solver *s, char *filename) {
  int index, entry;
  Vec_Int_t *pScope;
  FILE *in = fopen(filename, "r");
  FILE *out = fopen("tmp/temp.qdimacs", "w");
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  Vec_Int_t *vExists = Vec_IntAlloc(0);
  Vec_Int_t *vForalls = Vec_IntAlloc(0);
  while ((read = getline(&line, &len, in)) != -1) {
    if (len == 0)
      continue;
    if (line[0] == 'p') {
      fprintf(out, "%s", line);
      fprintf(out, "a");
      for (int i = 0; i < Vec_IntSize(s->pQuanType) - 1; i++) {
        pScope = (Vec_Int_t *)Vec_PtrEntry(s->pQuan, i);
        Vec_IntForEachEntry(pScope, entry, index) {
          Vec_IntPush(vForalls, entry + 1);
          fprintf(out, " %d", entry + 1);
        }
      }
      fprintf(out, " 0\n");
      fprintf(out, "e");
      pScope = (Vec_Int_t *)Vec_PtrEntryLast(s->pQuan);
      Vec_IntForEachEntry(pScope, entry, index) {
        Vec_IntPush(vExists, entry + 1);
        fprintf(out, " %d", entry + 1);
      }
      fprintf(out, " 0\n");
    } else if (line[0] == 'e' || line[0] == 'r' || line[0] == 'c') {
      continue;
    } else {
      fprintf(out, "%s", line);
    }
  }
  fclose(in);
  fclose(out);
  assert(Vec_IntSize(vExists) + Vec_IntSize(vForalls) == Abc_NtkPiNum(s->pNtk));
  assert(chdir("manthan"));
  Util_CallProcess("python3", s->verbose, "python3",
                   "manthan.py", // "--preprocess", "--unique",
                                 // "--unique",
                   "--unique", "--preprocess", "--multiclass", "--lexmaxsat",
                   "--verb", "0", "../tmp/temp.qdimacs", NULL);
  assert(chdir("../"));
  if (s->verbose) {
    Abc_Print(1, "> Calling manthan done\n");
  }
  Abc_Ntk_t *pSkolem = Io_ReadAiger("manthan/temp_skolem.aig", 1);
  Abc_Ntk_t *pSkolemSyn = Abc_NtkDC2(pSkolem, 0, 0, 1, 0, 0);
  Abc_Ntk_t *pNtkNew = applySkolem(s->pNtk, pSkolemSyn, vForalls, vExists);
  Abc_NtkDelete(s->pNtk);
  s->pNtk = pNtkNew;
  Abc_NtkDelete(pSkolem);
  Abc_NtkDelete(pSkolemSyn);
  Vec_IntFree(vExists);
  Vec_IntFree(vForalls);
  ssat_synthesis(s);
  if (!s->useBdd)
    ssat_build_bdd(s);
}

//=================================================
int ssat_solver_solve2(ssat_solver *s) {
  Vec_Int_t *pRandomReverse = Vec_IntAlloc(0);
  Abc_Print(1, "> #level: %d\n", Vec_IntSize(s->pQuanType));

  // perform quantifier elimination
  for (int i = Vec_IntSize(s->pQuanType) - 1; i >= 0; i--) {
    QuantifierType type = Vec_IntEntry(s->pQuanType, i);
    Vec_Int_t *pScope = (Vec_Int_t *)Vec_PtrEntry(s->pQuan, i);
    if (s->verbose) {
      int entry, index;
      Abc_Print(1, "> level %d: %d element\n", i, Vec_IntSize(pScope));
      Abc_Print(1, "> variables: ");
      Vec_IntForEachEntry(pScope, entry, index) {
        Abc_Print(1, "%d ", PiIndex_to_VarId(entry));
      }
      Abc_Print(1, "\n");
    }
    if (type == Quantifier_Exist) {
      ssat_solver_existelim(s, pScope);
    } else {
      ssat_solver_randomelim(s, pScope, pRandomReverse);
    }

    if (s->doSynthesis && !s->useBdd) {
      ssat_synthesis(s);
    }
    if (!s->useBdd) {
      ssat_build_bdd(s);
    }
    ssat_check_const(s);
  }
  if (Vec_IntSize(s->pUnQuan)) {
    if (s->useBdd)
      s->bFunc = ExistQuantify(s->dd, s->bFunc, s->pUnQuan);
    else
      s->pNtk = existence_eliminate_scope(s->pNtk, s->pUnQuan, s->verbose);
  }

  if (s->useBdd) {
    s->pNtk = Abc_NtkDeriveFromBdd(s->dd, s->bFunc, NULL, NULL);
    s->pNtk = Util_NtkStrash(s->pNtk, 1);
  }

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
  Util_CallProcess("python3", s->verbose, "python3", "general05.py", filename,
                   "tmp/temp.sdimacs", NULL);

  // open file
  ssat_Parser(s, "tmp/temp.sdimacs");
  ssat_parser_finished_process(s);
  if (s->verbose) {
    // check redundant variables
    Abc_Print(1, "> Count Redundant Variables\n");
    int entry, index;
    Vec_Int_t *unQuan = s->pUnQuan;
    int num = 0;
    Vec_IntForEachEntry(unQuan, entry, index) {
      int id = VarId_to_PiIndex(entry);
      Abc_Obj_t *pObj = Abc_NtkPi(s->pNtk, id);
      if (Abc_ObjFanoutNum(pObj) == 0) {
        num++;
      }
    }
    Abc_Print(1, "> Redundant Count = %d\n", num);
  }
  // perform manthan on the original cnf
  if (!s->useBdd && Vec_IntEntryLast(s->pQuanType) == Quantifier_Exist &&
      Abc_NtkNodeNum(s->pNtk) > 5000) {
    if (s->verbose) {
      Abc_Print(1, "> exist on original cnf\n");
    }
    ssat_solver_existouter(s, "tmp/temp.sdimacs");
    Vec_IntPop(s->pQuanType);
    Vec_PtrPop(s->pQuan);
    Vec_FltPop(s->pQuanWeight);
  }
  ssat_check_const(s);
  int result = ssat_solver_solve2(s);

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
