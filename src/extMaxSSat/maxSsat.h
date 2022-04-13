#ifndef ABC__ext__maxssat_h
#define ABC__ext__maxssat_h

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include <vector>
typedef struct max_ssat_t max_ssat;

struct max_ssat_t {
  Abc_Ntk_t *pNtk;
  Vec_Ptr_t* softClause;
  Vec_Ptr_t* hardClause;
  int hardClauseWeight;
  int nSoftClause;
  int nVar;
  int nAuxVar;
  int verbose;
};

extern max_ssat* max_ssat_new();
extern max_ssat* max_ssat_parse(char *pFileName, int fVerbose);
extern void max_ssat_write_ssat(max_ssat* s);
extern void max_ssat_free(max_ssat*);

#endif
