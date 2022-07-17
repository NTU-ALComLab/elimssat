#ifndef ABC__ext__ssatparser_h
#define ABC__ext__ssatparser_h

#include "base/abc/abc.h"
#include "extAvyItp/ItpMinisat.h"
#include "extMinisat/utils/ParseUtils.h"
#include "extUtil/util.h"
#include "ssatelim.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <zlib.h>

ABC_NAMESPACE_HEADER_START

using std::unordered_set;
using std::unordered_map;
using std::vector;

class ssatParser {
public:
  ssatParser(ssat_solver *s);
  ~ssatParser();
  void parse(char *filename);
  void unatePreprocess();
  void uniquePreprocess();
  void buildQuantifier();
  void buildNetwork();

private:
  // ssatParser.cc
  void parseRandom(Minisat::StreamBuffer &in);
  void parseExist(Minisat::StreamBuffer &in);
  void parseClause(Minisat::StreamBuffer &in);

  // unate.cc
  sat_solver *createUnateSolver(Vec_Int_t *vConsider);

  // unique.cc
  Abc_Obj_t *buildDefinition(vector<int> &, Aig_Man_t *);
  avy::ItpMinisat *createItpSolver(vector<int> &consider_vector);
  void collectVariable(vector<int> &consider_vector,
      vector<int> &shared_vector,
      unordered_set<int> &exist_map);

  int _nVar;
  int _nClause;
  unordered_map<int, Abc_Obj_t *> _definedVariables;

  Vec_Ptr_t *_clauseSet;
  Vec_Ptr_t *_quanBlock;
  Vec_Int_t *_quanType;
  ssat_solver *_solver;
};

ABC_NAMESPACE_HEADER_END
#endif
