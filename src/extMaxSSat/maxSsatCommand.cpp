#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "extUtil/util.h"
#include "misc/extra/extra.h"
#include "base/abc/abc.h"
#include "maxSsat.h"
#include "extSsatelim/ssatelim.h"
#include "misc/vec/vecInt.h"
#include <algorithm>
#include <iostream>
#include <zlib.h>
#include <string>

ABC_NAMESPACE_IMPL_START

static int maxSsat_command(Abc_Frame_t* pAbc, int argc, char ** argv) {
  int c;
  int fVerbose = 0;
  char *pFileName;
  max_ssat *m;
  ssat_solver *s;
  Vec_Int_t* pExist;
  Vec_Int_t* pRandom;
  Abc_Obj_t *pObj;
  double ssat_result;
  double result;
  int i;
  Extra_UtilGetoptReset();
  while ( ( c = Extra_UtilGetopt( argc, argv, "v" ) ) != EOF ) {
    switch(c)
    {
      case 'v':
        fVerbose ^= 1;
        break;
      case 'h':
        goto usage;
    }
  }
  pFileName = argv[globalUtilOptind];
  printf("Encoding MaxSAT problem to SSAT...\n");
  m = max_ssat_parse(pFileName, fVerbose);
  max_ssat_write_ssat(m);

  printf("SSAT Solving...\n");
  s = ssat_solver_new();
  s->verbose = fVerbose;
  s->pNtk = m->pNtk;
  pExist = Vec_IntAlloc(0);
  pRandom = Vec_IntAlloc(0);
  Abc_NtkForEachPi(s->pNtk, pObj, i) {
    if (i < m->nVar) {
      Vec_IntPush(pExist, i);
    } else {
      Vec_IntPush(pRandom, i);
    }
  }
  Vec_PtrPush(s->pQuan, pExist);
  Vec_IntPush(s->pQuanType, Quantifier_Exist);
  Vec_PtrPush(s->pQuan, pRandom);
  Vec_IntPush(s->pQuanType, Quantifier_Random);
  ssat_solver_solve2(s);
  ssat_result = s->result;
  printf("solution of ssat: %lf\n", ssat_result);
  result = (double)(1 << m->nAuxVar) * ssat_result;
  printf("o %d\n", m->nSoftClause - (int)result);
  ssat_solver_delete(s);
  max_ssat_free(m);

usage:
  return 1;
}

// called during ABC startup
static void init(Abc_Frame_t* pAbc)
{
    Cmd_CommandAdd( pAbc, "Alcom", "max_ssat", maxSsat_command, 1);
}

// called during ABC termination
static void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t maxsat_frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct maxssat_registrar
{
    maxssat_registrar() 
    {
        Abc_FrameAddInitializer(&maxsat_frame_initializer);
    }
} maxssat_registrar_;

ABC_NAMESPACE_IMPL_END
