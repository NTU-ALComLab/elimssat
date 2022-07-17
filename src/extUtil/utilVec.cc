#include "utilVec.h"

void Vec_IntMap(Vec_Int_t *vVec, std::function<void(int)> func) {
  int entry, index;
  Vec_IntForEachEntry(vVec, entry, index) { func(entry); }
}
