#include "ext-Synthesis/synthesis.h"
#include "ext-Synthesis/synthesisUtil.h"
#include "extUtil/util.h"
#include "ssatelim.h"

extern Abc_Ntk_t *Abc_NtkDC2(Abc_Ntk_t *pNtk, int fBalance, int fUpdateLevel,
                             int fFanout, int fPower, int fVerbose);
// extUtil/util.c
extern DdNode *ExistQuantify(DdManager *dd, DdNode *bFunc, Vec_Int_t *pPiIndex);

static Abc_Ntk_t *
existence_eliminate_scope_bdd(Abc_Ntk_t *pNtk, Vec_Int_t *pScope, int verbose) {
  Abc_Print(1, "existence_eliminate_scope_bdd\n");
  Abc_Ntk_t *pNtkNew = Util_NtkExistQuantifyPis_BDD(pNtk, pScope);
  Abc_Print(1, "Abc_NtkIsBddLogic(pNtk): %d\n", Abc_NtkIsBddLogic(pNtk));
  return pNtkNew;
}

static Abc_Ntk_t *
existence_eliminate_scope_aig(Abc_Ntk_t *pNtk, Vec_Int_t *pScope, int verbose) {
  assert(Abc_NtkIsStrash(pNtk));
  Abc_Ntk_t *pNtkNew = Util_NtkExistQuantifyPis(pNtk, pScope);
  return pNtkNew;
}

static Abc_Ntk_t *existence_eliminate_scope(Abc_Ntk_t *pNtk, Vec_Int_t *pScope,
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

void ssat_solver_existelim(ssat_solver *s, Vec_Int_t *pScope) {
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
      pScope = s->pUnQuan;
      Vec_IntForEachEntry(pScope, entry, index) {
        Vec_IntPush(vForalls, entry + 1);
        fprintf(out, " %d", entry + 1);
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
  // chdir("manthan");
  // Util_CallProcess("python3", s->verbose, "python3",
  //                  "manthan.py", // "--preprocess", "--unique",
  //                                // "--unique",
  //                  "--unique", "--preprocess", "--multiclass", "--lexmaxsat",
  //                  "--verb", "0", "../tmp/temp.qdimacs", NULL);
  // chdir("../");
  // if (s->verbose) {
  //   Abc_Print(1, "> Calling manthan done\n");
  // }
  // Abc_Ntk_t *pSkolem = Io_ReadAiger("manthan/temp_skolem.aig", 1);
  // Abc_Ntk_t *pSkolemSyn = Abc_NtkDC2(pSkolem, 0, 0, 1, 0, 0);
  // Abc_Ntk_t *pNtkNew = applySkolem(s->pNtk, pSkolemSyn, vForalls, vExists);
  Util_CallProcess("bin/cadet", s->verbose, "bin/cadet", "-e", "tmp/temp_skolem.aig",
                   "tmp/temp.qdimacs", NULL);
  Abc_Ntk_t *pSkolem = Io_ReadAiger("tmp/temp_skolem.aig", 1);
  pSkolem = Util_NtkDc2(pSkolem, 1);
  pSkolem = Util_NtkResyn2(pSkolem, 1);
  pSkolem = Util_NtkDFraig(pSkolem, 1);
  Abc_Ntk_t *pNtkNew = applyExists(s->pNtk, pSkolem);
  Abc_NtkDelete(s->pNtk);
  s->pNtk = pNtkNew;
  Abc_NtkDelete(pSkolem);
  Vec_IntFree(vExists);
  Vec_IntFree(vForalls);
  ssat_synthesis(s);
  if (!s->useBdd) {
    ssat_build_bdd(s);
  }
}
