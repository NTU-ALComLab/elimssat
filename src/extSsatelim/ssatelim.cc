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
#include "bdd/cudd/cudd.h"
#include "extUtil/util.h"
#include "misc/vec/vecFlt.h"
#include <libgen.h>
#include <signal.h>
#include <zlib.h>

#include "extUtil/easylogging++.h"

INITIALIZE_EASYLOGGINGPP

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static ssat_solver *_solver;
static void ssat_print_perf(ssat_solver *s);
static fasttime_t t_start, t_end;

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
    printf("  > Used BDD = %d\n", s->haveBdd);
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
  printf("  > Use BDD on exist elimination = %d\n", s->useBdd);
  printf("  > success BDD on exist elimination = %d\n", s->successBdd);
  printf("  > Use R2F on exist elimination = %d\n", s->useR2f);
  printf("  > success R2F on exist elimination = %d\n", s->successR2f);
  printf("  > Use Cadet on exist elimination = %d\n", s->useCadet);
  printf("  > success Cadet on exist elimination = %d\n", s->successCadet);
  printf("  > Use Manthan on exist elimination = %d\n", s->useManthan);
  printf("  > success Manthan on exist elimination = %d\n", s->successManthan);
}

static void ssat_check_const(ssat_solver *s) {
  if (s->haveBdd) {
    if (Cudd_IsConstant(s->bFunc)) {
      if (Cudd_IsComplement(s->bFunc)) {
        s->result = 0;
      } else {
        s->result = 1;
      }
      s->pPerf->fDone = 1;
    }
    return;
  }
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

  if (s->doSynthesis && !s->haveBdd) {
    ssat_synthesis(s);
  }
  if (!s->haveBdd) {
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
  s->pQuan = Vec_PtrAlloc(0);
  s->pQuanType = Vec_IntAlloc(0);
  s->pUnQuan = Vec_IntAlloc(0);
  s->pQuanWeight = Vec_FltAlloc(0);
  s->pPerf = ABC_ALLOC(ssat_solver_perf, 1);
  s->pPerf->current_type = Quantifier_Unknown;
  s->pPerf->nExpanded = 0;
  s->pPerf->tExists = 0;
  s->pPerf->tRandom = 0;
  s->pPerf->fDone = 0;
  s->result = -1;
  s->haveBdd = 0;
  s->doSynthesis = 1;
  s->useR2f = 0;
  s->useManthan = 0;
  s->useCadet = 0;
  s->successBdd = 0;
  s->successR2f = 0;
  s->successManthan = 0;
  s->successCadet = 0;
  s->verbose = 0;
  s->useReorder = 1;
  s->useProjected = 1;
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
}

void ssat_parser_finished_process(ssat_solver *s) {
  // Connect the output
  s->pNtk = Util_NtkStrash(s->pNtk, 1);
  ssat_quatification_check(s);
  ssat_synthesis(s);
  ssat_build_bdd(s);
}

/**
 * @brief The main routine of SSAT solving with quantifier elimination
 *
 * @return the satisfying probability
 */
int ssat_solver_solve2(ssat_solver *s) {
  Vec_Int_t *pRandomReverse = Vec_IntAlloc(0);
  int fExists = false;
  // perform quantifier elimination
  for (int i = Vec_IntSize(s->pQuanType) - 1; i >= 0; i--) {
    QuantifierType type = (QuantifierType)Vec_IntEntry(s->pQuanType, i);
    if (i == 0 && type == Quantifier_Exist && !s->haveBdd && s->useProjected) {
      fExists = true; // turn on sat solving on the outermost exists
      break;
    }
    Vec_Int_t *pScope = (Vec_Int_t *)Vec_PtrEntry(s->pQuan, i);
    VLOG(1) << "level " << i << ", quantifier type "
            << (type == Quantifier_Exist ? "Exist" : "Random") << ", "
            << Vec_IntSize(pScope) << " element";
    if (type == Quantifier_Exist) {
      ssat_solve_exist(s, pScope);
    } else {
      ssat_solve_random(s, pScope, pRandomReverse);
    }
    ssat_solve_afterelim(s);
  }
  // count the final result
  ssat_randomCompute(s, pRandomReverse, fExists);
  Vec_IntFree(pRandomReverse);
  s->pPerf->fDone = 1;
  return s->result;
}

void ssat_main(char *filename, int fReorder, int fProjected, int fPreprocess,
               int fGatedetect, int fVerbose) {
  signal(SIGINT, ssat_sighandler);
  signal(SIGTERM, ssat_sighandler);
  signal(SIGSEGV, ssat_sighandler);
  // setup easylogging
  el::Configurations defaultConf;
  defaultConf.setToDefault();
  if (fVerbose) {
    el::Loggers::setVerboseLevel(1);
  }
  defaultConf.set(el::Level::Info, el::ConfigurationType::Format,
                  "[%level] %msg");
  defaultConf.setGlobally(el::ConfigurationType::Format, "[%level] %msg");
  defaultConf.set(el::Level::Verbose, el::ConfigurationType::Format,
                  "[%level-%vlevel] %msg");
  el::Loggers::reconfigureAllLoggers(defaultConf);
  _solver = ssat_solver_new();
  _solver->verbose = fVerbose;
  _solver->useReorder = fReorder;
  _solver->useProjected = fProjected;
  _solver->usePreprocess = fPreprocess;
  _solver->useGateDetect = fGatedetect;

  // open file
  ssat_Parser(_solver, filename);
  ssat_parser_finished_process(_solver);
  ssat_check_const(_solver);
  ssat_solver_solve2(_solver);

  printf("\n");
  printf("==== Solving Result ====\n");
  printf("  > Result = %lf\n", _solver->result);
  printf("  > Used BDD = %d\n", _solver->haveBdd);
  ssat_print_perf(_solver);
  // ssat_solver_delete(_solver);
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
