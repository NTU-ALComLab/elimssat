#ifndef ABC__ext__synthesisUtil_h
#define ABC__ext__synthesisUtil_h

#include "base/main/main.h"

void writeQdimacs(Abc_Ntk_t *pNtk, Vec_Int_t *pIndex, char *pFileName);
void writeQdimacsFile(Abc_Ntk_t *pNtk, char *name, int numPo,
                      Vec_Int_t *vForalls, Vec_Int_t *vExists);
Vec_Int_t *collectTFO(Abc_Ntk_t *pNtk, Vec_Int_t *pScope);
void callProcess(char *command, int fVerbose, char *exec_command, ...);

#endif
