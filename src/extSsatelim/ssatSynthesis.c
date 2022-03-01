#include "ssatelim.h"
#include "extUtil/util.h"


void ssat_synthesis(ssat_solver *s) {
  if (s->verbose) {
    Abc_Print(1, "> Perform Synthesis...\n");
    Abc_Print(1, "> Object Number before Synthesis: %d\n",
              Abc_NtkNodeNum(s->pNtk));
  }
  s->pNtk = Util_NtkDc2(s->pNtk, 1);
  s->pNtk = Util_NtkResyn2(s->pNtk, 1);
  s->pNtk = Util_NtkDFraig(s->pNtk, 1, 1);
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
    s->dd = (DdManager *)Abc_NtkBuildGlobalBdds(s->pNtk, 80000, 1, fReorder,
                                                fReverse, fVerbose);
  }
  s->bFunc = NULL;
  if (s->dd == NULL) {
    if (s->verbose)
      Abc_Print(1, "> Build BDD failed\n");
    s->haveBdd = 0;
  } else {
    s->haveBdd = 1;
    s->bFunc = (DdNode *)Abc_ObjGlobalBdd(Abc_NtkCo(s->pNtk, 0));
    if (s->verbose) {
      Abc_Print(1, "> Build BDD success\n");
      Abc_Print(1, "> Use BDD based Solving\n");
      Abc_Print(1, "> Object Number of current network: %d\n",
                Cudd_DagSize(s->bFunc));
    }
  }
}

