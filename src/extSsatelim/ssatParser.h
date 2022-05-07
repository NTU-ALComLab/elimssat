#ifndef ABC__ext__ssatparser_h
#define ABC__ext__ssatparser_h

#include "base/abc/abc.h"
#include "ssatelim.h"
#include <vector>
#include <unordered_set>

ABC_NAMESPACE_HEADER_START

class ssatParser {
public:
  ssatParser(ssat_solver *s): _solver(s) {};
  void parse(char *filename);
private:
  void uniquePreprocess();
  ssat_solver *_solver;
};

ABC_NAMESPACE_HEADER_END
#endif
