#include "extSynthesis/synthesis.h"
#include "extSynthesis/synthesisUtil.h"
#include "extUtil/util.h"
#include "ssatelim.h"

extern Abc_Ntk_t *Abc_NtkDC2(Abc_Ntk_t *pNtk, int fBalance, int fUpdateLevel,
                             int fFanout, int fPower, int fVerbose);
// extUtil/util.c
extern DdNode *ExistQuantify(DdManager *dd, DdNode *bFunc, Vec_Int_t *pPiIndex);

static void ssat_collect_existouter(ssat_solver *s, char *filename, char *qdimacs_file, Vec_Int_t *vExists, Vec_Int_t *vForalls) {
  FILE *in = fopen(filename, "r");
  FILE *out = fopen(qdimacs_file, "w");
  char *line = NULL;
  size_t len = 0;
  int entry, index;
  ssize_t read;
  Vec_Int_t *pScope;
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
}

static Abc_Ntk_t *ssat_cadet_existouter(Abc_Ntk_t *pNtk, int timeout, char *aig_file, char* qdimacs_file) {
  int ret = Util_CallProcess("bin/cadet", timeout, 0, "bin/cadet", "-e", aig_file,
      qdimacs_file, NULL);
  if (!ret) return NULL;
  Abc_Ntk_t *pSkolem = Io_ReadAiger( aig_file, 1);
  assert(pSkolem);
  pSkolem = Util_NtkDc2(pSkolem, 1);
  pSkolem = Util_NtkResyn2rs(pSkolem, 1);
  pSkolem = Util_NtkDFraig(pSkolem, 1, 1);
  Abc_Ntk_t *pNtkNew = applyExists(pNtk, pSkolem);
  Abc_NtkDelete(pSkolem);
  return pNtkNew;
}

static Abc_Ntk_t *ssat_manthan_existouter(Abc_Ntk_t *pNtk, int timeout, int fVerbose, char *aig_file, char *qdimacs_file, Vec_Int_t* vExists, Vec_Int_t* vForalls) {
  chdir("manthan");
  char temp_file[512], temp_aigfile[256];
  sprintf(temp_file, "../%s", qdimacs_file);
  sprintf(temp_aigfile, "manthan/%s", aig_file);
  int ret = Util_CallProcess("python3", timeout, fVerbose, "python3", "manthan.py", // "--preprocess", "--unique",
              // "--unique",
              "--unique", "--preprocess",
              "--multiclass",
              // "--lexmaxsat",
              "--verb", "0", temp_file, NULL);
  chdir("../");
  if (!ret) {
    return NULL;
  }
  if (fVerbose) {
    printf("%s\n", temp_aigfile);
  }
  Abc_Ntk_t *pSkolem = Io_ReadAiger(temp_aigfile, 1);
  assert(pSkolem);
  Abc_Ntk_t *pSkolemSyn = Abc_NtkDC2(pSkolem, 0, 0, 1, 0, 0);
  Abc_Ntk_t *pNtkNew = applySkolem(pNtk, pSkolemSyn, vForalls, vExists);
  if (fVerbose) {
    printf("finish manthan exist\n");
  }
  return pNtkNew;
}

static void ssat_finish_existouter(ssat_solver *s, Abc_Ntk_t *pNtkNew, char* aig_file, char* qdimacs_file, Vec_Int_t *vExists, Vec_Int_t *vForalls) {
  Abc_NtkDelete(s->pNtk);
  s->pNtk = pNtkNew;
  ssat_synthesis(s);
  if (!s->haveBdd) {
    ssat_build_bdd(s);
  }
  Vec_IntFree(vExists);
  Vec_IntFree(vForalls);
  remove(aig_file);
  remove(qdimacs_file);
}

void ssat_solver_existelim(ssat_solver *s, Vec_Int_t *pScope) {
  if (!s->haveBdd) {
    s->pNtk = unatePreprocess(s->pNtk, &pScope);
    ssat_synthesis(s);
    ssat_build_bdd(s);
  }
  if (s->haveBdd) {
    ++s->useBdd;
    s->bFunc = ExistQuantify(s->dd, s->bFunc, pScope);
    ++s->successBdd;
  }
  else {
    Vec_Int_t *pScopeNew = Vec_IntAlloc(pScope->nSize);
    Abc_Ntk_t *pTemp;
    int entry, index;
    // change PI index into object index
    Vec_IntForEachEntry(pScope, entry, index) {
      Vec_IntPush(pScopeNew, Abc_NtkPi(s->pNtk, entry)->Id);
    }
    ++s->useCadet;
    ++s->useR2f;
    pTemp = r2fExistElim(s->pNtk, pScopeNew, s->verbose);
    if (pTemp) {
      ++s->successR2f;
      Abc_NtkDelete(s->pNtk);
      s->pNtk = pTemp;
      Vec_IntFree(pScopeNew);
      return;
    }
    ++s->useManthan;
    pTemp = manthanExistsElim(s->pNtk, pScopeNew, s->verbose, s->pName);
    if (pTemp) {
      ++s->successManthan;
      Abc_NtkDelete(s->pNtk);
      s->pNtk = pTemp;
      Vec_IntFree(pScopeNew);
      return;
    }
    assert(false);
  }
}

void ssat_solver_existouter(ssat_solver *s, char *filename) {
  int index, entry;
  Vec_Int_t *pScope;
  // cadet/manthan exist on original cnf
  char temp_aigfile[256], temp_qdimcasfile[256];
  sprintf(temp_qdimcasfile, "%s.qdimacs", s->pName);
  sprintf(temp_aigfile, "%s_skolem.aig", s->pName);
  Abc_Ntk_t* pNtkNew;
  Vec_Int_t *vExists = Vec_IntAlloc(0);
  Vec_Int_t *vForalls = Vec_IntAlloc(0);
  ssat_collect_existouter(s, filename, temp_qdimcasfile, vExists, vForalls);
  ++s->useCadet;
  if (s->verbose) {
    printf("try cadet exist elim\n");
  }
  pNtkNew = ssat_cadet_existouter(s->pNtk, 200, temp_aigfile, temp_qdimcasfile);
  if (pNtkNew) {
    ++s->successCadet;
    ssat_finish_existouter(s, pNtkNew, temp_aigfile, temp_qdimcasfile, vExists, vForalls);
    return;
  }
  pScope = (Vec_Int_t *)Vec_PtrEntryLast(s->pQuan);
  Vec_Int_t *pScopeNew = Vec_IntAlloc(pScope->nSize);
  // change PI index into object index
  Vec_IntForEachEntry(pScope, entry, index) {
    Vec_IntPush(pScopeNew, Abc_NtkPi(s->pNtk, entry)->Id);
  }
  ++s->useR2f;
  if (s->verbose) {
    printf("try r2f exist elim\n");
  }
  pNtkNew = r2fExistElim(s->pNtk, pScopeNew, s->verbose);
  Vec_IntFree(pScopeNew);
  if (pNtkNew) {
    ++s->successR2f;
    ssat_finish_existouter(s, pNtkNew, temp_aigfile, temp_qdimcasfile, vExists, vForalls);
    return;
  }

  ++s->useManthan;
  if (s->verbose) {
    printf("try manthan exist elim\n");
  }
  pNtkNew = ssat_manthan_existouter(s->pNtk, 0, s->verbose, temp_aigfile, temp_qdimcasfile, vExists, vForalls);
  assert(pNtkNew);
  ++s->successManthan;
  ssat_finish_existouter(s, pNtkNew, temp_aigfile, temp_qdimcasfile, vExists, vForalls);
  remove(temp_aigfile);
  remove(temp_qdimcasfile);
}
