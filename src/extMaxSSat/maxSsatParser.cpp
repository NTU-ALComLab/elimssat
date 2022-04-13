#include "base/abc/abc.h"
#include "base/io/ioAbc.h"
#include "extUtil/util.h"
#include "maxSsat.h"
#include "misc/vec/vecInt.h"
#include "misc/vec/vecPtr.h"
#include "sat/bsat2/ParseUtils.h"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <string>

ABC_NAMESPACE_IMPL_START

static void max_ssat_finish(max_ssat *s) {
  Vec_Int_t *vVec;
  Vec_Ptr_t *vHardObj = Vec_PtrAlloc(0);
  Vec_Ptr_t *vSoftObj = Vec_PtrAlloc(0);
  // count the number of auxillary variables
  s->nAuxVar = 0;
  int i = 1;
  while (i < s->nSoftClause) {
    s->nAuxVar++;
    i <<= 1;
  }
  if (s->verbose) {
    printf("number of hard clauses: %d\n", Vec_PtrSize(s->hardClause));
  }
  Util_NtkCreateMultiPi(s->pNtk, s->nVar + s->nAuxVar);

  // create hardClause object
  Vec_PtrForEachEntry(Vec_Int_t *, s->hardClause, vVec, i) {
    int lit, j;
    Vec_Ptr_t *vObj = Vec_PtrAlloc(0);
    Vec_IntForEachEntry(vVec, lit, j) {
      assert(abs(lit) <= s->nVar);
      Abc_Obj_t *pObj = Abc_NtkPi(s->pNtk, abs(lit) - 1);
      if (signbit(lit))
        pObj = Abc_ObjNot(pObj);
      Vec_PtrPush(vObj, pObj);
    }
    Vec_PtrPush(vHardObj, Util_NtkCreateMultiOr(s->pNtk, vObj));
    Vec_IntFree(vVec);
    Vec_PtrFree(vObj);
  }
  Vec_PtrClear(vSoftObj);
  Vec_Ptr_t *vAnd = Vec_PtrAlloc(0);
  if (s->verbose) {
    printf("number of soft clauses: %d\n", Vec_PtrSize(s->softClause));
    printf("number of auxillary variables: %d\n", s->nAuxVar);
  }
  // create softClause object
  for (i = 0; i < s->nSoftClause; i++) {
    Vec_Ptr_t *vObj = Vec_PtrAlloc(0);
    int lit, j;
    Vec_IntForEachEntry((Vec_Int_t *)s->softClause->pArray[i], lit, j) {
      assert(abs(lit) <= s->nVar);
      Abc_Obj_t *pObj = Abc_NtkPi(s->pNtk, abs(lit) - 1);
      if (signbit(lit))
        pObj = Abc_ObjNot(pObj);
      Vec_PtrPush(vObj, pObj);
    }
    Vec_IntFree((Vec_Int_t *)s->softClause->pArray[i]);
    for (j = 0; j < s->nAuxVar; j++) {
      Vec_PtrPush(
          vAnd, Abc_ObjNotCond(Abc_NtkPi(s->pNtk, s->nVar + j), (i >> j) & 1));
    }
    assert(Vec_PtrSize(vAnd) == s->nAuxVar);
    Abc_Obj_t *pObj =
        Util_AigNtkAnd(s->pNtk, Util_NtkCreateMultiAnd(s->pNtk, vAnd),
                       Util_NtkCreateMultiOr(s->pNtk, vObj));
    Vec_PtrPush(vSoftObj, pObj);
    Vec_PtrFree(vObj);
    Vec_PtrClear(vAnd);
  }
  Abc_Obj_t *pObj = Util_NtkCreateMultiOr(s->pNtk, vSoftObj);
  Abc_Obj_t *pPo = pObj;
  if (Vec_PtrSize(vHardObj) != 0) {
    pPo = Util_AigNtkAnd(
        s->pNtk, pObj, Util_NtkCreateMultiAnd(s->pNtk, vHardObj));
  }
  Abc_NtkCreatePo(s->pNtk);
  Abc_ObjAddFanin(Abc_NtkPo(s->pNtk, 0), pPo);
  Vec_PtrFree(vSoftObj);
  Vec_PtrFree(vHardObj);
  Vec_PtrFree(vAnd);

  if (s->verbose) {
    printf("AIG size before synthesis: %d\n", Abc_NtkNodeNum(s->pNtk));
    printf("performing synthesis...\n");
  }
  s->pNtk = Util_NtkDc2(s->pNtk, 1);
  // s->pNtk = Util_NtkResyn2rs(s->pNtk, 1);
  // s->pNtk = Util_NtkResyn2rs(s->pNtk, 1);
  // s->pNtk = Util_NtkDFraig(s->pNtk, 1, 1);
  if (s->verbose) {
    printf("AIG size after synthesis: %d\n", Abc_NtkNodeNum(s->pNtk));
  }
}

max_ssat *max_ssat_parse(char *pFileName, int fVerbose) {
  max_ssat *s = max_ssat_new();
  s->verbose = fVerbose;
  gzFile input_stream = gzopen(pFileName, "rb");
  Minisat::StreamBuffer in(input_stream);
  int clauses;
  for (;;) {
    skipWhitespace(in);
    if (*in == EOF) break;
    else if (*in == 'p') {
      if (eagerMatch(in, "p wcnf")) {
        s->nVar = parseInt(in);
        clauses = parseInt(in);
        s->hardClauseWeight = parseInt(in);
      }
    } else if (*in == 'c') {
      skipLine(in);
    } else {
      Vec_Int_t *pVec = Vec_IntAlloc(0);
      int weight = parseInt(in);
      for (;;) {
        int parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        Vec_IntPush(pVec, parsed_lit);
      }
      if (weight == s->hardClauseWeight) {
        Vec_PtrPush(s->hardClause, (void *)pVec);
      } else {
        Vec_PtrPush(s->softClause, (void *)pVec);
        s->nSoftClause++;
      }
    }
  }
  if (gzclose(input_stream) != Z_OK) {
    printf("failed gzclose");
    exit(255);
  }
  max_ssat_finish(s);
  return s;
}

ABC_NAMESPACE_IMPL_END
