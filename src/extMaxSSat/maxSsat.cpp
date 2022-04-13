#include "maxSsat.h"
#include "base/abc/abc.h"
#include "misc/vec/vecInt.h"
#include "misc/vec/vecPtr.h"

ABC_NAMESPACE_IMPL_START

static void writeCnfClause(FILE *out, Abc_Ntk_t *pNtk, int numPo) {
  Abc_Obj_t *pObj, *pFanin0, *pFanin1;
  int i;

  Abc_NtkForEachNode(pNtk, pObj, i) {
    pFanin0 = Abc_ObjFanin0(pObj);
    pFanin1 = Abc_ObjFanin1(pObj);
    if (!Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d -%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (Abc_ObjFaninC0(pObj) && !Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d %d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (!Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d -%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
    if (Abc_ObjFaninC0(pObj) && Abc_ObjFaninC1(pObj)) {
      fprintf(out, "%d %d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0),
              Abc_ObjId(pFanin1));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
      fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin1));
    }
  }

  pObj = Abc_NtkPo(pNtk, numPo);
  pFanin0 = Abc_ObjFanin0(pObj);
  if (!Abc_ObjFaninC0(pObj)) {
    fprintf(out, "%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
    fprintf(out, "-%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
  } else {
    fprintf(out, "%d %d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
    fprintf(out, "-%d -%d 0\n", Abc_ObjId(pObj), Abc_ObjId(pFanin0));
  }
  fprintf(out, "%d 0\n", Abc_ObjId(pObj));
}

max_ssat* max_ssat_new() {
  max_ssat *s = (max_ssat *)ABC_CALLOC(char, sizeof(max_ssat));
  s->nSoftClause = 0;
  s->softClause = Vec_PtrAlloc(0);
  s->hardClause = Vec_PtrAlloc(0);
  s->pNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
  s->verbose = 0;
  return s;
}

void max_ssat_free(max_ssat* s) {
  if (s) {
    Vec_PtrFree(s->softClause);
    Vec_PtrFree(s->hardClause);
    ABC_FREE(s);
  }
}

void max_ssat_write_ssat(max_ssat* s) {
  char name[256] = "test.sdimacs";
  FILE *out;
  int numVar, numCla;
  int entry, index;
  Abc_Obj_t* pObj;
  Vec_Int_t* vRandom = Vec_IntAlloc(0);
  Vec_Int_t* vExist1 = Vec_IntAlloc(0);
  Vec_Int_t* vExist2 = Vec_IntAlloc(0);
  int i;

  Abc_NtkIncrementTravId(s->pNtk);
  for (int i = 0; i < s->nAuxVar; i++) {
    Abc_NodeSetTravIdCurrent(Abc_NtkPi(s->pNtk, s->nVar + i));
  }
  Abc_NtkForEachObj(s->pNtk, pObj, i) {
    if (pObj->Id == 0) continue;
    if (Abc_NodeIsTravIdCurrent(pObj)) {
      Vec_IntPush(vRandom, Abc_ObjId(pObj));
    } else if (Abc_ObjIsPi(pObj)) {
      Vec_IntPush(vExist1, Abc_ObjId(pObj));
    } else {
      Vec_IntPush(vExist2, Abc_ObjId(pObj));
    }
  }
  out = fopen(name, "w");
  if (!out) {
    Abc_Print(-1, "Cannot open file %s\n", name);
    return;
  }
  numVar = Abc_NtkObjNumMax(s->pNtk) - 1;
  numCla = 3 * Abc_NtkNodeNum(s->pNtk) + 2 + 1;

  fprintf(out, "c qdimacs file for boolean function synthesis\n");
  fprintf(out, "p cnf %d %d\n", numVar, numCla);
  fprintf(out, "e ");
  Vec_IntForEachEntry(vExist1, entry, index) { fprintf(out, "%d ", entry); }
  fprintf(out, "0\n");
  fprintf(out, "r 0.5 ");
  Vec_IntForEachEntry(vRandom, entry, index) { fprintf(out, "%d ", entry); }
  fprintf(out, "0\n");
  fprintf(out, "e ");
  Vec_IntForEachEntry(vExist2, entry, index) { fprintf(out, "%d ", entry); }
  fprintf(out, "0\n");

  writeCnfClause(out, s->pNtk, 0);
  fclose(out);
  Vec_IntClear(vRandom);
  Vec_IntClear(vExist1);
  Vec_IntClear(vExist2);
  // Abc_Print(-2, "File %s is written.\n", name);
}

ABC_NAMESPACE_IMPL_END
