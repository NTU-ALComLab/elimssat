#include <iostream>

#include "intRel2Func.hpp"
#include "eqRel2Func.hpp"
#include "subRel2Func.hpp"
#include "extUtil/util.h"

using namespace std;

Abc_Ntk_t* singleRel2FuncEQ2(Abc_Ntk_t* pNtk, vector<char*> &vNameY, int yNum) {
    //cerr << "start sinRel2func" << endl;
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        if(strcmp(Abc_ObjName(pObj), vNameY[yNum]) == 0) {
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

    ntkAddOne(pNtk, pNtkNew);
    pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

    Abc_Obj_t* pObjOut = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pObjOut, name, NULL);
    Abc_ObjAddFanin( pObjOut, pObjNew);
    pNtkNew = resyn(pNtkNew);
    pNtkNew = resyn(pNtkNew);

    for(unsigned j = yNum; j < vNameY.size(); j++) {
        pNtkNew = EQInt(pNtkNew, vNameY[j]);
    }
    vector<Abc_Obj_t*> vPi;
    int j;
    
    Abc_NtkForEachPi(pNtkNew, pObj, j) {
        if(Abc_ObjFanoutNum(pObj) == 0)
            vPi.push_back(pObj);
    }
    for(unsigned i = 0; i < vPi.size(); i++) {
        Abc_NtkDeleteObj(vPi[i]);
    }
    
    return pNtkNew;
}
Abc_Ntk_t* singleRel2FuncEQ(Abc_Ntk_t* pNtk, vector<char*> &vNameY, int yNum) {
    //cerr << "start sinRel2func" << endl;
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        if(strcmp(Abc_ObjName(pObj), vNameY[yNum]) == 0) {
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

    ntkAddOne(pNtk, pNtkNew);
    pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

    Abc_Obj_t* pObjOut = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pObjOut, name, NULL);
    Abc_ObjAddFanin( pObjOut, pObjNew);
    pNtkNew = resyn(pNtkNew);
    pNtkNew = resyn(pNtkNew);

    for(unsigned j = yNum; j < vNameY.size(); j++) {
        pNtkNew = EQ(pNtkNew, vNameY[j]);
    }
    vector<Abc_Obj_t*> vPi;
    int j;
    
    Abc_NtkForEachPi(pNtkNew, pObj, j) {
        if(Abc_ObjFanoutNum(pObj) == 0)
            vPi.push_back(pObj);
    }
    for(unsigned i = 0; i < vPi.size(); i++) {
        Abc_NtkDeleteObj(vPi[i]);
    }
    
    return pNtkNew;
}

Abc_Ntk_t* rel2FuncEQ(Abc_Ntk_t* pNtk, vector<char*> & vNameY, vector<Abc_Ntk_t*> &vFunc) {
    Abc_Ntk_t* pNtkFunc;
    Abc_Ntk_t* pNtkRel = Abc_NtkDup(pNtk);
    cout << "function info during processing" << endl;
    cout << "-------------------------------" << endl;
    cout << "lev#\tnode#\tpi#" << endl;
    cout << "-------------------------------" << endl;
    for(unsigned i = 0; i < vNameY.size(); i++) {
        //cout << i << "\tRelation Node: " << Abc_NtkObjNum(pNtkRel) << endl;
        pNtkFunc = singleRel2FuncEQ(pNtkRel, vNameY, i);
        if(Abc_NtkObjNum(pNtkFunc) > MAX_AIG_NODE) {
            cout <<  "The number of AIG nodes reached " << MAX_AIG_NODE << endl;
            Abc_NtkDelete(pNtkFunc);
            Abc_NtkDelete(pNtkRel);
            return NULL;
        }
        pNtkRel = replaceX(pNtkRel, pNtkFunc, vNameY[i]);
        cout << Abc_AigLevel(pNtkFunc) << "\t" << Abc_NtkObjNum(pNtkFunc) << "\t" << Abc_NtkPiNum(pNtkFunc) << endl;
        vFunc.push_back(pNtkFunc);
    }
    checkRelFunc(pNtkRel, pNtk);
    return pNtkRel;
}
Abc_Ntk_t* rel2FuncEQ2(Abc_Ntk_t* pNtk, vector<char*> & vNameY, vector<Abc_Ntk_t*> &vFunc) {
    Abc_Ntk_t* pNtkFunc;
    Abc_Ntk_t* pNtkRel = Abc_NtkDup(pNtk);
    cout << "function info during processing" << endl;
    cout << "-------------------------------" << endl;
    cout << "lev#\tnode#\tpi#" << endl;
    cout << "-------------------------------" << endl;
    for(unsigned i = 0; i < vNameY.size(); i++) {
        //cout << i << "\tRelation Node: " << Abc_NtkObjNum(pNtkRel) << endl;
        pNtkFunc = singleRel2FuncEQ2(pNtkRel, vNameY, i);
        if(Abc_NtkObjNum(pNtkFunc) > MAX_AIG_NODE) {
            cout <<  "The number of AIG nodes reached " << MAX_AIG_NODE << endl;
            Abc_NtkDelete(pNtkFunc);
            Abc_NtkDelete(pNtkRel);
            return NULL;
        }
        pNtkFunc = Util_NtkDFraig(pNtkFunc, 1, 1);
        pNtkFunc = Util_NtkResyn2rs(pNtkFunc, 1);
        pNtkFunc = Util_NtkDc2(pNtkFunc, 1);
        pNtkRel = replaceX(pNtkRel, pNtkFunc, vNameY[i]);
        pNtkRel = Util_NtkDFraig(pNtkRel, 1, 1);
        pNtkRel = Util_NtkResyn2rs(pNtkRel, 1);
        pNtkRel = Util_NtkDc2(pNtkRel, 1);
        cout << Abc_AigLevel(pNtkFunc) << "\t" << Abc_NtkObjNum(pNtkFunc) << "\t" << Abc_NtkPiNum(pNtkFunc) << endl;
        vFunc.push_back(pNtkFunc);
    }
    checkRelFunc(pNtkRel, pNtk);
    return pNtkRel;
}

Abc_Ntk_t* rel2FuncEQ3(Abc_Ntk_t* pNtk, vector<char*> & vNameY, vector<Abc_Ntk_t*> &vFunc) {
  //  Abc_Ntk_t* pNtkFunc;
    Abc_Ntk_t* pNtkRel = Abc_NtkDup(pNtk);
    //vector<Abc_Ntk_t*> vNtkRel;
    vFunc.push_back(pNtkRel);
    cout << "function info during processing" << endl;
    cout << "-------------------------------" << endl;
    cout << "soltype\tlev#\tnode#\tpi#\trel-lev#\trel-node#" << endl;
    cout << "-------------------------------" << endl;

    //compute the EQ networks
    for(unsigned i = 0; i < vNameY.size() - 1; i++) {
        //cout << i << "\tRelation Node: " << Abc_NtkObjNum(pNtkRel) << endl;
        //cout << vFunc.size();
        pNtkRel = EQInt(pNtkRel, vNameY[i]);
        vFunc.push_back(pNtkRel);
    //    cout << "EQ size: " << Abc_NtkObjNum(pNtkRel) << endl;
    }
    cout << endl;
    
    pNtkRel = Abc_NtkDup(pNtk);
    for(int i =  vNameY.size() - 1; i >= 0 ; i--) {
        Abc_Ntk_t* pNtkTemp = vFunc[i];
        vFunc[i] = singleRel2FuncInt(pNtkTemp, vNameY[i]);
        Abc_Ntk_t* pNtkFuncTemp = singleRel2Func(pNtkTemp, vNameY[i]);
        Abc_NtkDelete(pNtkTemp);
        if(vFunc[i] == NULL) {
            vFunc[i] = pNtkFuncTemp;
            cout << "0\t";
        }
        else if(Abc_NtkObjNum(pNtkFuncTemp) > Abc_NtkObjNum(vFunc[i])) {
            vFunc[i] = resyn(vFunc[i]);
            Abc_NtkDelete(pNtkFuncTemp);
            cout << "1\t";
        }
        else {
            Abc_NtkDelete(vFunc[i]);
            vFunc[i] = pNtkFuncTemp;
            cout << "0\t";
        }
        if(Abc_NtkObjNum(vFunc[i]) > MAX_AIG_NODE) {
            cout <<  "The number of AIG nodes reached " << MAX_AIG_NODE << endl;
            
            for(unsigned k = 0; k < vFunc.size(); k++)
                Abc_NtkDelete(vFunc[i]);
            vFunc.clear();
            Abc_NtkDelete(pNtkRel);
            return NULL;
        }
        assert(vFunc[i] != NULL);       
        for(int j = i - 1; j >= 0; j--) {
            vFunc[j] = replaceX(vFunc[j], vFunc[i], vNameY[i]);
        }
        pNtkRel = replaceX(pNtkRel, vFunc[i], vNameY[i]);
        cout << Abc_AigLevel(vFunc[i]) << "\t" << Abc_NtkObjNum(vFunc[i]) << "\t" << Abc_NtkPiNum(vFunc[i]) << "\t" << Abc_AigLevel(pNtkRel) << "\t" << Abc_NtkObjNum(pNtkRel) << endl;
        //cout << Abc_AigLevel(vFunc[i]) << "\t" << Abc_NtkObjNum(vFunc[i]) << "\t" << Abc_NtkPiNum(vFunc[i]) << endl;
    }
    
    checkRelFunc(pNtkRel, pNtk);
    return pNtkRel;
}

