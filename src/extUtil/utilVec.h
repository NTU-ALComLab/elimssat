#ifndef ABC__ext__utilVec_h
#define ABC__ext__utilVec_h

#include "base/main/main.h"
#include <functional>

void Vec_IntMap(Vec_Int_t *vVec, std::function<void(int)> func);


#endif
