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
#include "extUtil/util.h"
#include "misc/vec/vecFlt.h"
#include <signal.h>
#include <zlib.h>

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static const lbool l_Unsupp = -2;
static ssat_solver *_solver;
static void ssat_print_perf(ssat_solver *s);

// bdd/extrab/extraBdd.h
extern Abc_Ntk_t *Abc_NtkFromGlobalBdds(Abc_Ntk_t *pNtk, int fReverse);

void ssat_sighandler(int signal_number) {
  printf("\n");
  printf("Interruption occurs!\n");
  printf("Reporting performance before termination...\n");
  ssat_print_perf(_solver);
  exit(0);
}

static void ssat_print_perf(ssat_solver *s) {
  if (!s) return;
  printf("\n");
  printf("==== elimssat profiling ====\n");
  printf("\n");
  if (!s->pPerf->fDone) {
    printf("  > Solving not finish!\n");
    printf("  > Number of eliminated quantifier     = %d\n", s->pPerf->nExpanded);
    printf("  > Working on the quantifier type of   = %d\n", s->pPerf->current_type);
  }
  printf("\n");
  printf("  > Time consumed on exist elimination  = %lf\n", s->pPerf->tExists);
  printf("  > Time consumed on random elimination = %lf\n", s->pPerf->tRandom);
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

int ssatelim_init_function() { return 0; }

static inline int VarId_to_PiIndex(int varId) { return varId - 1; }

static inline int PiIndex_to_VarId(int piIndex) { return piIndex + 1; }

void ssat_synthesis(ssat_solver *s) {
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

void ssat_build_bdd(ssat_solver *s) {
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
  s->pPerf = ABC_ALLOC(ssat_solver_perf, 1);
  s->pPerf->current_type = -1;
  s->pPerf->nExpanded = 0;
  s->pPerf->tExists = 0;
  s->pPerf->tRandom = 0;
  s->pPerf->fDone = 0;
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
    ABC_FREE(s->pPerf);
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
      Vec_IntPush(s->pUnQuan, index);
    }
  }
}

void ssat_check_redundant_var(ssat_solver *s) {
  if (s->verbose) {
    // check redundant variables
    Abc_Print(1, "> Count Redundant Variables\n");
    int entry, index;
    Vec_Int_t *unQuan = s->pUnQuan;
    int num = 0;
    Vec_IntForEachEntry(unQuan, entry, index) {
      Abc_Obj_t *pObj = Abc_NtkPi(s->pNtk, entry);
      if (Abc_ObjFanoutNum(pObj) == 0) {
        num++;
      }
    }
    Abc_Print(1, "> Redundant Count = %d\n", num);
  }
}

void ssat_parser_finished_process(ssat_solver *s) {
  // Connect the output
  Abc_ObjAddFanin(Abc_NtkPo(s->pNtk, 0), s->pPo);
  s->pNtk = Util_NtkStrash(s->pNtk, 1);
  Abc_NtkCheck(s->pNtk);
  // try building BDD
  ssat_synthesis(s);
  ssat_build_bdd(s);
  ssat_quatification_check(s);
  ssat_check_redundant_var(s);
  // perform manthan on the original cnf if the network is small enough
  if (!s->useBdd && Vec_IntEntryLast(s->pQuanType) == Quantifier_Exist &&
      Abc_NtkNodeNum(s->pNtk) > 5000) {
    if (s->verbose) {
      Abc_Print(1, "> exist on original cnf\n");
    }
    fasttime_t t_start = gettime();
    ssat_solver_existouter(s, "tmp/temp.sdimacs");
    fasttime_t t_end = gettime();
    s->pPerf->tExists += tdiff(t_start, t_end);
    Vec_IntPop(s->pQuanType);
    Vec_PtrPop(s->pQuan);
    Vec_FltPop(s->pQuanWeight);
  }
}

int ssat_solver_solve2(ssat_solver *s) {
  Vec_Int_t *pRandomReverse = Vec_IntAlloc(0);
  Abc_Print(1, "> #level: %d\n", Vec_IntSize(s->pQuanType));

  // perform quantifier elimination
  for (int i = Vec_IntSize(s->pQuanType) - 1; i >= 0; i--) {
    QuantifierType type = Vec_IntEntry(s->pQuanType, i);
    s->pPerf->current_type = type;
    Vec_Int_t *pScope = (Vec_Int_t *)Vec_PtrEntry(s->pQuan, i);
    if (s->verbose) {
      Abc_Print(1, "> level %d, quantifier type %s, %d element\n", i,
                type == Quantifier_Exist ? "Exist" : "Random",
                Vec_IntSize(pScope));
    }
    if (type == Quantifier_Exist) {
      ssat_solver_existelim(s, pScope);
    } else {
      ssat_solver_randomelim(s, pScope, pRandomReverse);
    }
    s->pPerf->nExpanded += 1;

    if (s->doSynthesis && !s->useBdd) {
      ssat_synthesis(s);
    }
    if (!s->useBdd) {
      ssat_build_bdd(s);
    }
    ssat_check_const(s);
  }
  if (Vec_IntSize(s->pUnQuan)) {
    ssat_solver_existelim(s, s->pUnQuan);
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
  ssat_randomCompute(s, pRandom);
  // Abc_Print( 1, "result: %lf\n", s->result );
  Vec_IntFree(pRandom);
  Vec_IntFree(pRandomReverse);
  s->pPerf->fDone = 1;
  return l_True;
}

void ssat_main(char *filename, int fVerbose) {
  _solver = ssat_solver_new();
  _solver->verbose = fVerbose;
  Util_CallProcess("python3", _solver->verbose, "python3", "general05.py", filename,
                   "tmp/temp.sdimacs", NULL);
  signal(SIGINT, ssat_sighandler);

  // open file
  ssat_Parser(_solver, "tmp/temp.sdimacs");
  ssat_parser_finished_process(_solver);
  ssat_check_const(_solver);
  int result = ssat_solver_solve2(_solver);

  if (result == l_False) {
    // Abc_Print( 1, "s UNSATISFIABLE\n" );
    Abc_Print(1, "s UNSATISFIABLE\n");
    Abc_Print(1, "  > Result = %e\n", _solver->result);
  } else if (result == l_True) {
    // print probabilty
    Abc_Print(1, "s SATISFIABLE\n");
    Abc_Print(1, "  > Result = %e\n", _solver->result);

  } else if (result == l_Undef) {
    Abc_Print(1, "s UNKNOWN\n");
  } else if (result == l_Unsupp) {
    Abc_Print(1, "s UNSUPPORT\n");
  }
  ssat_print_perf(_solver);
  ssat_solver_delete(_solver);
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
