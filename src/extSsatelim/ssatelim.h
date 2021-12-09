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

#include <unistd.h>
#include "base/abc/abc.h"
#include "base/main/main.h"
#include "bdd/extrab/extraBdd.h" // 
#include "sat/bsat/satSolver.h"
#include "extUtil/fasttime.h"
#include "ssatBooleanSort.h"

////////////////////////////////////////////////////////////////////////
///                         DECLARATION                              ///
////////////////////////////////////////////////////////////////////////

enum QuantifierType
{
  Quantifier_Exist  = 0,
  Quantifier_Random = 1,
  Quantifier_Forall = 2,
};
typedef enum QuantifierType QuantifierType;

struct ssat_solver_t;
typedef struct ssat_solver_t ssat_solver;
struct ssat_solver_perf_t;
typedef struct ssat_solver_perf_t ssat_solver_perf;


struct ssat_solver_t
{
  DdManager *dd;
  Abc_Ntk_t * pNtk;
  DdNode *bFunc;
  ssat_solver_perf * pPerf;
  Abc_Obj_t * pPo;
  Vec_Ptr_t * pQuan;
  Vec_Int_t * pQuanType;
  Vec_Int_t * pUnQuan;
  Vec_Flt_t * pQuanWeight;
  Vec_Int_t * varPiIndex;
  Vec_Int_t * piVarIndex;
  double result;
  int doSynthesis;
  int useBdd;
  int useCadet;
  int useManthan;
  int verbose;
};

struct ssat_solver_perf_t
{
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
extern ssat_solver* ssat_solver_new();
extern void ssat_solver_delete(ssat_solver* s);

extern int ssatelim_init_function();
extern void ssat_main(char * filename, int fVerbose);
extern void ssat_solver_setnvars(ssat_solver* s,int n);
extern void ssat_Parser(ssat_solver * s, char * filename);
extern void ssat_parser_finished_process(ssat_solver* s);
extern int ssat_solver_solve2(ssat_solver* s);
extern double ssat_result(ssat_solver* s);

// ssatParser.cc
extern int ssat_addexistence(ssat_solver* s, lit* begin, lit* end);
extern int ssat_addforall(ssat_solver* s, lit* begin, lit* end);
extern int ssat_addrandom(ssat_solver* s, lit* begin, lit* end, double prob);
extern int ssat_addclause(ssat_solver* s, lit* begin, lit* end);

// ssatExist.c
extern void ssat_solver_existouter(ssat_solver *s, char *c);
extern void ssat_solver_existelim(ssat_solver* s, Vec_Int_t* pScope);

// ssatRandom.c
extern void ssat_solver_randomelim(ssat_solver* s, Vec_Int_t* pScope, Vec_Int_t *pRandomReverse);
extern void ssat_randomCompute(ssat_solver* s, Vec_Int_t *pRandom);

// ssatsynthesis.c
extern void ssat_synthesis(ssat_solver *s);
extern void ssat_build_bdd(ssat_solver *s);

extern void ssat_write_wmc(Abc_Ntk_t* pNtk, char* name, Vec_Int_t* pRandom, Vec_Flt_t* pWeight);

ABC_NAMESPACE_HEADER_END
#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
