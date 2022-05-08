#ifndef ABC__ext__ssatparser_h
#define ABC__ext__ssatparser_h

#include "base/abc/abc.h"
#include "extAvyItp/ItpMinisat.h"
#include "ssatelim.h"
#include "extMinisat/utils/ParseUtils.h"
#include "extUtil/util.h"
#include <vector>
#include <unordered_set>
#include <zlib.h>

ABC_NAMESPACE_HEADER_START

using std::unordered_set;


class ssatParser {
public:
  ssatParser(ssat_solver *s);
  ~ssatParser();
  void parse(char *filename);
  void uniquePreprocess();
  void buildQuantifier();
  void buildNetwork();
private:
  void parseRandom(Minisat::StreamBuffer &in);
  void parseExist(Minisat::StreamBuffer &in);
  void parseClause(Minisat::StreamBuffer &in);

  int _nVar;
  int _nClause;
  unordered_set<int> _deletedVarSet;
  Vec_Ptr_t *_clauseSet;
  Vec_Ptr_t *_quanBlock;
  Vec_Int_t *_quanType;
  avy::ItpMinisat *_itpSolver;
  ssat_solver *_solver;
};

ABC_NAMESPACE_HEADER_END
#endif
