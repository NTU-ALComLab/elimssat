/**HFile****************************************************************
  FileName    [ssatelim.h]
  SystemName  []
  PackageName []
  Synopsis    []
  Author      [Kuan-Hua Tu]

  Affiliation []
  Date        [Sun Jun 23 02:39:25 CST 2019]
***********************************************************************/

#ifndef ABC__ext__ssatelim_h
#define ABC__ext__ssatelim_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "bdd/extrab/extraBdd.h" //
#include "extUtil/fasttime.h"
#include "sat/bsat/satSolver.h"
#include "ssatBooleanSort.h"
#include <unistd.h>

////////////////////////////////////////////////////////////////////////
///                         DECLARATION                              ///
////////////////////////////////////////////////////////////////////////

enum QuantifierType {
  Quantifier_Exist = 0,
  Quantifier_Random = 1,
  Quantifier_Forall = 2,
  Quantifier_Unknown,
};
typedef enum QuantifierType QuantifierType;

struct ssat_solver_t;
typedef struct ssat_solver_t ssat_solver;
struct ssat_solver_perf_t;
typedef struct ssat_solver_perf_t ssat_solver_perf;

struct ssat_solver_t {
  DdManager *dd;
  Abc_Ntk_t *pNtk;
  DdNode *bFunc;
  ssat_solver_perf *pPerf;
  Vec_Ptr_t *pQuan;
  Vec_Int_t *pQuanType;
  Vec_Int_t *pUnQuan;
  Vec_Flt_t *pQuanWeight;
  char *pName;
  double result;
  int doSynthesis;
  int haveBdd;
  int useBdd;
  int useCadet;
  int useManthan;
  int useR2f;
  int successBdd;
  int successR2f;
  int successManthan;
  int successCadet;
  int useReorder;
  int useProjected;
  int usePreprocess;
  int useGateDetect;
  int verbose;
};

struct ssat_solver_perf_t {
  int fDone;
  int nExpanded;
  QuantifierType current_type;
  double tExists;
  double tRandom;
};

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////
extern ssat_solver *ssat_solver_new();
extern void ssat_solver_delete(ssat_solver *s);

extern int ssatelim_init_function();
extern void ssat_main(char *filename, int fReorder, int fProjected,
                      int fPreprocess, int fGatedetect, int fVerbose);
extern void ssat_solver_setnvars(ssat_solver *s, int n);
extern void ssat_Parser(ssat_solver *s, char *filename);
extern void ssat_parser_finished_process(ssat_solver *s);
extern int ssat_solver_solve2(ssat_solver *s);
extern double ssat_result(ssat_solver *s);

// ssatParser.cc
extern int ssat_addexistence(ssat_solver *s, lit *begin, lit *end);
extern int ssat_addforall(ssat_solver *s, lit *begin, lit *end);
extern int ssat_addrandom(ssat_solver *s, lit *begin, lit *end, double prob);
extern int ssat_addclause(ssat_solver *s, lit *begin, lit *end);

// ssatExist.c
extern void ssat_solver_existouter(ssat_solver *s, char *c);
extern void ssat_solver_existelim(ssat_solver *s, Vec_Int_t *pScope);
// unate.cc
extern Abc_Ntk_t *unatePreprocess(Abc_Ntk_t *pNtk, Vec_Int_t **pScope);

// ssatRandom.c
extern void ssat_solver_randomelim(ssat_solver *s, Vec_Int_t *pScope,
                                   Vec_Int_t *pRandomReverse);
extern void ssat_randomCompute(ssat_solver *s, Vec_Int_t *pRandom, int fExists);

// ssatsynthesis.c
extern void ssat_synthesis(ssat_solver *s);
extern void ssat_build_bdd(ssat_solver *s);

ABC_NAMESPACE_HEADER_END
#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
