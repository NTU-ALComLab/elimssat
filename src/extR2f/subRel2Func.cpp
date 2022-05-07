#include <iostream>
#include <map>
#include "extUtil/util.h"
#include "subRel2Func.hpp"

using namespace std;


Abc_Ntk_t* singleRel2Func(Abc_Ntk_t* pNtk, char* yName) {

    //cerr << "start sinRel2func" << endl;
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        if(strcmp(Abc_ObjName(pObj), yName) == 0) {
            pObjNew = Abc_AigConst1(pNtkNew);
        }
        else {
            pObjNew = Abc_NtkCreatePi( pNtkNew );
            // add name
            Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        }
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }


    //pObjNew = Abc_NtkCreatePo(pNtkNew);

    Util_NtkAppend(pNtkNew, pNtk);
    pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

    Abc_Obj_t* pObjOut = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pObjOut, name, NULL);
    Abc_ObjAddFanin( pObjOut, pObjNew);
    
    pNtkNew = Util_NtkResyn2(pNtkNew, 1);
    pNtkNew = Util_NtkResyn2(pNtkNew, 1);
    
    return pNtkNew;

}

