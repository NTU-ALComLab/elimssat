#include "ssatParser.h"
#include "extUtil/easylogging++.h"
#include "misc/vec/vecPtr.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <experimental/filesystem>

using namespace std;

ABC_NAMESPACE_IMPL_START

static char *get_filename(char *filename) {
  char *name = basename(filename);
  char *ret = strtok(name, ".");
  return ret;
}

ssatParser::ssatParser(ssat_solver *s) : _solver(s) {
  _clauseSet = Vec_PtrAlloc(0);
  _quanBlock = Vec_PtrAlloc(0);
  _quanType = Vec_IntAlloc(0);
  _definedVariables.clear();
}

ssatParser::~ssatParser() {
  Vec_Int_t *entry;
  int i;
  Vec_PtrForEachEntry(Vec_Int_t *, _quanBlock, entry, i) { Vec_IntFree(entry); }
  Vec_PtrForEachEntry(Vec_Int_t *, _clauseSet, entry, i) { Vec_IntFree(entry); }
  Vec_PtrFree(_clauseSet);
  Vec_PtrFree(_quanBlock);
  Vec_IntFree(_quanType);
}

void ssatParser::parseClause(Minisat::StreamBuffer &in) {
  int var;
  Vec_Int_t *pClause = Vec_IntAlloc(0);
  for (;;) {
    var = parseInt(in);
    if (var == 0)
      break;
    Vec_IntPush(pClause, var);
  }
  Vec_PtrPush(_clauseSet, pClause);
}

void ssatParser::parseExist(Minisat::StreamBuffer &in) {
  ++in;
  int var;
  Vec_Int_t *pBlock;
  if (Vec_IntSize(_quanType) != 0 &&
      Vec_IntEntryLast(_quanType) == Quantifier_Exist) {
    pBlock = (Vec_Int_t *)Vec_PtrEntryLast(_quanBlock);
  } else {
    pBlock = Vec_IntAlloc(0);
    Vec_PtrPush(_quanBlock, pBlock);
    Vec_IntPush(_quanType, Quantifier_Exist);
  }
  for (;;) {
    var = parseInt(in);
    if (var == 0)
      break;
    // convert variable to PI index
    Vec_IntPush(pBlock, var);
  }
}

void ssatParser::parseRandom(Minisat::StreamBuffer &in) {
  // skip probability
  int var;
  Vec_Int_t *pBlock;
  if (Vec_IntSize(_quanType) != 0 &&
      Vec_IntEntryLast(_quanType) == Quantifier_Random) {
    pBlock = (Vec_Int_t *)Vec_PtrEntryLast(_quanBlock);
  } else {
    pBlock = Vec_IntAlloc(0);
    Vec_PtrPush(_quanBlock, pBlock);
    Vec_IntPush(_quanType, Quantifier_Random);
  }
  ++in;
  skipWhitespace(in);
  while (*in != ' ') {
    ++in;
  }
  for (;;) {
    var = parseInt(in);
    if (var == 0)
      break;
    Vec_IntPush(pBlock, var);
  }
}

void ssatParser::parse(char *filename) {
  gzFile input_stream = gzopen(filename, "rb");
  if (input_stream == NULL) {
    printf("file %s cannot be opened\n", filename);
    exit(1);
  }
  Minisat::StreamBuffer in(input_stream);
  for (;;) {
    skipWhitespace(in);
    if (*in == EOF)
      break;
    else if (*in == 'p') {
      if (eagerMatch(in, "p cnf")) {
        _nVar = parseInt(in);
        ssat_solver_setnvars(_solver, _nVar);
        _nClause = parseInt(in);
      }
    } else if (*in == 'c') {
      skipLine(in);
    } else if (*in == 'r') {
      parseRandom(in);
    } else if (*in == 'e') {
      parseExist(in);
    } else {
      parseClause(in);
    }
  }
  VLOG(1) << "number of variables: " << _nVar;
  val_assert_msg(Vec_PtrSize(_clauseSet) == _nClause,
                 "number of clause is not the same as the header");
}

void ssatParser::buildQuantifier() {
  Vec_Int_t *vVec;
  int index;
  Vec_PtrForEachEntry(Vec_Int_t *, _quanBlock, vVec, index) {
    Vec_Int_t *vBlock = Vec_IntAlloc(0);
    int entry, i;
    Vec_IntForEachEntry(vVec, entry, i) {
      if (_definedVariables.find(entry) != _definedVariables.end())
        continue;
      // convert to PI index
      Vec_IntPush(vBlock, entry - 1);
    }
    if (Vec_IntSize(vBlock) != 0) {
      Vec_PtrPush(_solver->pQuan, vBlock);
      Vec_IntPush(_solver->pQuanType, Vec_IntEntry(_quanType, index));
    } else {
      Vec_IntFree(vBlock);
    }
  }
}

void ssatParser::buildNetwork() {
  Vec_Int_t *vVec;
  Vec_Ptr_t *vClause = Vec_PtrAlloc(0);
  Vec_Ptr_t *vPi = Vec_PtrAlloc(0);
  int index;
  // create object for each clause
  Vec_PtrForEachEntry(Vec_Int_t *, _clauseSet, vVec, index) {
    int lit, i;
    Vec_IntForEachEntry(vVec, lit, i) {
      // Pi index start from 0, so -1 on every entry
      int var = abs(lit);
      Abc_Obj_t* pObj;
      if (_definedVariables.find(var) != _definedVariables.end()) {
        pObj = _definedVariables.at(var);
      } else {
        pObj = Abc_NtkPi(_solver->pNtk, var - 1);
      }
      Vec_PtrPush(vPi, Abc_ObjNotCond(pObj, signbit(lit)));
    }
    Vec_PtrPush(vClause, Util_NtkCreateMultiOr(_solver->pNtk, vPi));
    Vec_PtrClear(vPi);
  }
  // create object for output
  Abc_Obj_t *pPo = Util_NtkCreateMultiAnd(_solver->pNtk, vClause);
  Abc_ObjAddFanin(Abc_NtkPo(_solver->pNtk, 0), pPo);
  Vec_PtrFree(vPi);
  Vec_PtrFree(vClause);
}

void ssat_Parser(ssat_solver *s, char *filename) {
  if (!std::experimental::filesystem::exists(filename)) {
    printf("file %s does not exists!\n", filename);
    exit(1);
  }
  char file[256], temp_file[256];
  sprintf(file, "%s", filename);
  s->pName = get_filename(file);
  sprintf(temp_file, "%s.sdimacs", s->pName);
  if (s->usePreprocess) { // preprocess with hqspre
    assert(Util_CallProcess("./bin/hqspre", 0, s->verbose, "./hqspre", "--preserve_gates", "1",
                      "-o", temp_file, filename, NULL));
    filename = temp_file;
  }
  assert(Util_CallProcess("python3", 0, s->verbose, "python3",
                    "script/wmcrewriting2.py", filename, temp_file, NULL));
  ssatParser *p = new ssatParser(s);
  p->parse(temp_file);
  if (s->useGateDetect) {
    p->uniquePreprocess();
  }
  p->buildNetwork();
  p->buildQuantifier();
  // p->unatePreprocess();
  remove(temp_file);
  delete (p);
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
