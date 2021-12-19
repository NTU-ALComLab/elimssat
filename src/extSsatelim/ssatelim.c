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
#include "base/io/ioAbc.h"
#include "extUtil/util.h"
#include "misc/vec/vecFlt.h"
#include <libgen.h>
#include <signal.h>
#include <zlib.h>

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static const lbool l_Unsupp = -2;
static ssat_solver *_solver;
static void ssat_print_perf(ssat_solver *s);
static fasttime_t t_start, t_end;

static char *get_filename(char *filename) {
  char *name = basename(filename);
  char *ret = strtok(name, ".");
  return ret;
}

void ssat_sighandler(int signal_number) {
  printf("\n");
  printf("Interruption occurs!\n");
  printf("Reporting performance before termination...\n");
  ssat_print_perf(_solver);
  exit(0);
}

static void ssat_print_perf(ssat_solver *s) {
  if (!s)
    return;
  printf("\n");
  printf("==== elimssat profiling ====\n");
  if (!s->pPerf->fDone) {
    printf("  > Solving not finish!\n");
    printf("  > Working on the quantifier type of   = %d\n",
           s->pPerf->current_type);
    t_end = gettime();
    if (s->pPerf->current_type == Quantifier_Exist) {
      s->pPerf->tExists += tdiff(t_start, t_end);
    } else {
      s->pPerf->tRandom += tdiff(t_start, t_end);
    }
  }
  printf("\n");
  printf("  > Number of eliminated quantifier     = %d\n", s->pPerf->nExpanded);
  printf("  > Time consumed on exist elimination  = %lf\n", s->pPerf->tExists);
  printf("  > Time consumed on random elimination = %lf\n", s->pPerf->tRandom);
}

static void ssat_check_const(ssat_solver *s) {
  Abc_Obj_t *pObj, *pPo;
  pPo = Abc_NtkPo(s->pNtk, 0);
  pObj = Abc_ObjFanin0(pPo);
  if (Abc_AigNodeIsConst(pObj)) {
    if (Abc_ObjFaninC0(pPo)) {
      s->result = 0;
    } else {
      s->result = 1;
    }
    s->pPerf->fDone = 1;
  }
}

static void ssat_solve_random(ssat_solver *s, Vec_Int_t *pScope,
                              Vec_Int_t *pRandomReverse) {
  s->pPerf->current_type = Quantifier_Random;
  if (s->pPerf->fDone)
    return;
  t_start = gettime();
  ssat_solver_randomelim(s, pScope, pRandomReverse);
  t_end = gettime();
  s->pPerf->tRandom += tdiff(t_start, t_end);
}

static void ssat_solve_exist(ssat_solver *s, Vec_Int_t *pScope) {
  t_start = gettime();
  if (s->pPerf->fDone)
    return;
  s->pPerf->current_type = Quantifier_Exist;
  ssat_solver_existelim(s, pScope);
  t_end = gettime();
  s->pPerf->tExists += tdiff(t_start, t_end);
}

static void ssat_solve_afterelim(ssat_solver *s) {
  s->pPerf->nExpanded += 1;

  if (s->doSynthesis && !s->useBdd) {
    ssat_synthesis(s);
  }
  if (!s->useBdd) {
    ssat_build_bdd(s);
  }
  ssat_check_const(s);
}

int ssatelim_init_function() { return 0; }

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

static void ssat_quatification_check(ssat_solver *s) {
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
  assert(Vec_IntSize(s->pUnQuan) == 0);
}

void ssat_parser_finished_process(ssat_solver *s, char *filename) {
  // Connect the output
  Abc_ObjAddFanin(Abc_NtkPo(s->pNtk, 0), s->pPo);
  s->pNtk = Util_NtkStrash(s->pNtk, 1);
  Abc_NtkCheck(s->pNtk);
  // try building BDD
  ssat_synthesis(s);
  ssat_build_bdd(s);
  ssat_quatification_check(s);
  // perform manthan on the original cnf if the network is small enough
  if (!s->useBdd && Vec_IntEntryLast(s->pQuanType) == Quantifier_Exist) {
    if (s->verbose) {
      Abc_Print(1, "> exist on original cnf\n");
    }
    s->pPerf->current_type = Quantifier_Exist;
    t_start = gettime();
    ssat_solver_existouter(s, filename);
    t_end = gettime();
    s->pPerf->tExists += tdiff(t_start, t_end);
    s->pPerf->nExpanded += 1;
    Vec_IntPop(s->pQuanType);
    Vec_PtrPop(s->pQuan);
    Vec_FltPop(s->pQuanWeight);
  }
}

int ssat_solver_solve2(ssat_solver *s) {
  Vec_Int_t *pRandomReverse = Vec_IntAlloc(0);
  // perform quantifier elimination
  for (int i = Vec_IntSize(s->pQuanType) - 1; i >= 0; i--) {
    QuantifierType type = Vec_IntEntry(s->pQuanType, i);
    // if (i == 0 && type == Quantifier_Exist && !s->useBdd) break;
    Vec_Int_t *pScope = (Vec_Int_t *)Vec_PtrEntry(s->pQuan, i);
    if (s->verbose) {
      Abc_Print(1, "> level %d, quantifier type %s, %d element\n", i,
                type == Quantifier_Exist ? "Exist" : "Random",
                Vec_IntSize(pScope));
    }
    if (type == Quantifier_Exist) {
      ssat_solve_exist(s, pScope);
    } else {
      ssat_solve_random(s, pScope, pRandomReverse);
    }
    ssat_solve_afterelim(s);
  }
  // count the final result
  ssat_randomCompute(s, pRandomReverse);
  Vec_IntFree(pRandomReverse);
  s->pPerf->fDone = 1;
  return l_True;
}

void ssat_main(char *filename, int fVerbose) {
  signal(SIGINT, ssat_sighandler);
  signal(SIGTERM, ssat_sighandler);
  signal(SIGSEGV, ssat_sighandler);
  _solver = ssat_solver_new();
  char file[256], temp_file[256];
  sprintf(file, "%s", filename);
  _solver->verbose = fVerbose;
  _solver->pName = get_filename(filename);
  printf("%s\n", _solver->pName);
  sprintf(temp_file, "%s.sdimacs", _solver->pName);

  // convert benchmarks to 0.5 prob
  if (!Util_CallProcess("python3", _solver->verbose, "python3",
                        "script/general05.py", file, temp_file, NULL)) {
    Util_CallProcess("python3", _solver->verbose, "python3",
                     "script/wmcrewriting2.py", file, temp_file, NULL);
  }

  // open file
  ssat_Parser(_solver, temp_file);
  ssat_parser_finished_process(_solver, temp_file);
  ssat_check_const(_solver);
  int result = ssat_solver_solve2(_solver);

  Abc_Print(1, "\n");
  Abc_Print(1, "==== Solving Result ====\n");
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
  remove(temp_file);
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
