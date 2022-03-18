/**CFile****************************************************************
  FileName    [ssatParser.cc]
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

#include "extUtil/util.h"
#include "formula.hpp"
#include "ssatelim.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <zlib.h>
// #include <math.h>
#include <math.h>
using namespace std;

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static const int DEBUG_PRINT = 0;
static hqspre::Formula formula;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
static const unsigned BUFLEN = 1024000;
static int CLAUSE_NUM = 0;
static int PREFIX_NUM = 0;
static int EXIST_NUM = 0;
static int RANDOM_NUM = 0;

static void error(const char *const msg) {
  std::cerr << msg << "\n";
  exit(255);
}

int ssat_addScope(ssat_solver *s, int *begin, int *end, QuantifierType type) {
  Vec_Int_t *pScope;
  if (Vec_IntSize(s->pQuanType) && Vec_IntEntryLast(s->pQuanType) == type) {
    pScope = (Vec_Int_t *)Vec_PtrEntryLast(s->pQuan);
  } else {
    pScope = Vec_IntAlloc(0);
    Vec_PtrPush(s->pQuan, pScope);
    Vec_IntPush(s->pQuanType, type);
  }

  for (int *it = begin; it < end; it++) {
    Vec_IntPush(pScope, *it - 1);
  }
  return 1;
}

int ssat_addexistence(ssat_solver *s, int *begin, int *end) {
  Vec_FltPush(s->pQuanWeight, -1);
  return ssat_addScope(s, begin, end, Quantifier_Exist);
}

int ssat_addforall(ssat_solver *s, int *begin, int *end) {
  Vec_FltPush(s->pQuanWeight, -1);
  return ssat_addScope(s, begin, end, Quantifier_Forall);
}

int ssat_addrandom(ssat_solver *s, int *begin, int *end, double prob) {
  Vec_FltPush(s->pQuanWeight, prob);
  if (prob != 0.5) {
    Abc_Print(-1, "Probility of random var is not 0.5\n");
  }
  return ssat_addScope(s, begin, end, Quantifier_Random);
}

int ssat_addclause(ssat_solver *s, lit *begin, lit *end) {
  Abc_Obj_t *pObj = Abc_AigConst0(s->pNtk);
  for (lit *it = begin; it < end; it++) {
    int var = lit_var(*it);
    int sign = lit_sign(*it);
    Abc_Obj_t *pPi = Abc_NtkPi(s->pNtk, var - 1);
    if (sign) {
      pPi = Abc_ObjNot(pPi);
    }
    pObj = Util_AigNtkOr(s->pNtk, pObj, pPi);
  }
  s->pPo = Util_AigNtkAnd(s->pNtk, s->pPo, pObj);
  return 1;
}

void ssat_readHeader_cnf(ssat_solver *s, string &str_in) {
  string str = str_in.substr(6); // for the length of str "p cnf "
  std::string::size_type sz = 0;

  long long nVar = stoll(str, &sz);
  str = str.substr(sz);
  if (DEBUG_PRINT)
    std::cout << " nVar: " << nVar << '\n';
  ssat_solver_setnvars(s, nVar);
  long long nClause = stoll(str, &sz);
  if (DEBUG_PRINT)
    std::cout << " nClause: " << nClause << '\n';
}

void ssat_readHeader(ssat_solver *s, string &str) {
  ssat_readHeader_cnf(s, str);
}

void ssat_readExistence(ssat_solver *s, string &str_in) {
  Vec_Int_t *p = Vec_IntAlloc(0);
  string str = str_in.substr(2); // for the length of str "e "
  std::string::size_type sz = 0;
  if (DEBUG_PRINT)
    cout << "e:" << endl;

  while (str.length()) {
    long long var = stoll(str, &sz);
    if (var == 0) {
      break;
    }

    if (DEBUG_PRINT)
      cout << " var:" << var << endl;
    Vec_IntPush(p, var);
    str = str.substr(sz + 1);
    while (str[0] == ' ') {
      str = str.substr(1);
    }
  }

  if (Vec_IntSize(p) > 0) {
    ssat_addexistence(s, Vec_IntArray(p), Vec_IntLimit(p));
  }
  Vec_IntFree(p);
}

void ssat_readForall(ssat_solver *s, string &str_in) {
  Vec_Int_t *p = Vec_IntAlloc(0);
  string str = str_in.substr(2); // for the length of str "a "
  std::string::size_type sz = 0;
  if (DEBUG_PRINT)
    cout << "a:" << endl;

  while (str.length()) {
    long long var = stoll(str, &sz);
    if (var == 0) {
      break;
    }

    if (DEBUG_PRINT)
      cout << " var:" << var << endl;
    Vec_IntPush(p, var);
    str = str.substr(sz + 1);
    while (str[0] == ' ') {
      str = str.substr(1);
    }
  }

  if (Vec_IntSize(p) > 0) {
    ssat_addforall(s, Vec_IntArray(p), Vec_IntLimit(p));
  }
  Vec_IntFree(p);
}

void ssat_readRandom(ssat_solver *s, string &str_in) {
  Vec_Int_t *p = Vec_IntAlloc(0);
  string str = str_in.substr(2); // for the length of str "r "
  std::string::size_type sz = 0;
  if (DEBUG_PRINT)
    cout << "r:" << endl;

  double prob = stod(str, &sz);
  if (DEBUG_PRINT)
    cout << " prob:" << prob << endl;
  str = str.substr(sz + 1);

  while (str.length()) {
    long long var = stoll(str, &sz);
    if (var == 0) {
      break;
    }

    if (DEBUG_PRINT)
      cout << " var:" << var << endl;
    Vec_IntPush(p, var);
    str = str.substr(sz + 1);
    while (str[0] == ' ') {
      str = str.substr(1);
    }
  }

  if (Vec_IntSize(p) > 0) {
    ssat_addrandom(s, Vec_IntArray(p), Vec_IntLimit(p), prob);
  }
  Vec_IntFree(p);
}

void ssat_readClause(ssat_solver *s, string &str_in) {
  Vec_Int_t *p = Vec_IntAlloc(0);
  string str = str_in;
  std::string::size_type sz = 0;

  while (str.length()) {
    long long lit = stoll(str, &sz);
    if (lit == 0) {
      break;
    }

    if (DEBUG_PRINT)
      cout << "lit:" << lit << endl;
    Vec_IntPush(p, toLitCond(abs(lit), signbit(lit)));
    str = str.substr(sz + 1);
    while (str[0] == ' ') {
      str = str.substr(1);
    }
  }

  ssat_addclause(s, Vec_IntArray(p), Vec_IntLimit(p));
  Vec_IntFree(p);
}

void ssat_lineHandle(ssat_solver *s, string &in_str) {
  if (in_str.length() == 0 || in_str[0] == '\n' || in_str[0] == '\r') {
    return;
  } else if (in_str[0] == 'c') {
    // skip command line
    return;
  } else if (in_str[0] == 'p') {
    ssat_readHeader(s, in_str);
  } else if (in_str[0] == 'e') {
    ssat_readExistence(s, in_str);
  } else if (in_str[0] == 'a') {
    ssat_readForall(s, in_str);
  } else if (in_str[0] == 'r') {
    ssat_readRandom(s, in_str);
  } else if (in_str[0] == '-' || isdigit(in_str[0])) {
    CLAUSE_NUM++;
    ssat_readClause(s, in_str);
  } else {
    cout << "Error: Unexpected symbol" << endl;
  }
}

void process(ssat_solver *s, gzFile in) {

  char buf[BUFLEN];
  char *offset = buf;

  for (;;) {
    int err, len = sizeof(buf) - (offset - buf);
    if (len == 0)
      error("Buffer to small for input line lengths");

    len = gzread(in, offset, len);

    if (len == 0)
      break;
    if (len < 0)
      error(gzerror(in, &err));

    char *cur = buf;
    char *end = offset + len;

    for (char *eol; (cur < end) && (eol = std::find(cur, end, '\n')) < end;
         cur = eol + 1) {
      string in_str = string(cur, eol);
      ssat_lineHandle(s, in_str);
    }

    // any trailing data in [eol, end) now is a partial line
    offset = std::copy(cur, end, buf);
  }

  // BIG CATCH: don't forget about trailing data without eol :)
  std::cout << std::string(buf, offset);

  if (gzclose(in) != Z_OK)
    error("failed gzclose");
}

void ssat_Parser(ssat_solver *s, char *filename) {
  if (filename == NULL) {
    return;
  }

  process(s, gzopen(filename, "rb"));
  int entry, index;
  Vec_IntForEachEntry(s->pQuanType, entry, index) {
    PREFIX_NUM++;
    if (entry == Quantifier_Exist) {
      EXIST_NUM += Vec_IntSize((Vec_Int_t *)(Vec_PtrEntry(s->pQuan, index)));
    } else {
      RANDOM_NUM += Vec_IntSize((Vec_Int_t *)(Vec_PtrEntry(s->pQuan, index)));
    }
  }
  printf("\n");
  printf("==== benchmark statistics ====\n");
  printf("  > Number of Prefixes                = %d\n", PREFIX_NUM);
  printf("  > Number of Existitential Variables = %d\n", EXIST_NUM);
  printf("  > Number of Randomize Variables     = %d\n", RANDOM_NUM);
  printf("  > Number of Clauses                 = %d\n", CLAUSE_NUM);

  return;
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
