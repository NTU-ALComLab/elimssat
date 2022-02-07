#include <iostream>
#include <map>

#include "eqRel2Func.hpp"
#include "bdd/cudd/cudd.h"
#include "bdd/cudd/cuddInt.h"

using namespace std;


// minimize 
Abc_Ntk_t* singleRel2FuncBdd2(Abc_Ntk_t* pNtk, vector<char*> &vNameY, int yNum, int long *memUsed, int choice) {
    //cerr << "start sinRel2func" << endl;
    Abc_Ntk_t *pNtkNew = Abc_NtkDup(pNtk);
    Abc_Obj_t *pObj;
    int i;

    DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtkNew, 100000, 1, 1, 1, 0);
   // cerr << "create bdd okay" << endl;
    if(dd == NULL) {
    cerr << "bdd NULL" << endl;
        //Abc_NtkDelete(pNtkNew);
        return NULL;
    }
   
    // map PI to bdd var
    map<const Abc_Obj_t*, int> mObjVar;
    Abc_NtkForEachCi(pNtkNew, pObj, i) {
        mObjVar[pObj] = i;
    }

    assert(Abc_NtkPoNum(pNtkNew) == 1);
    DdNode *f = (DdNode*)Abc_ObjGlobalBdd(Abc_NtkPo(pNtkNew, 0));
    pObj = Abc_NtkFindCi(pNtkNew, vNameY[yNum]);
    assert(pObj != NULL);
    DdNode *f1 = Cudd_Cofactor(dd,f,dd->vars[mObjVar[pObj]]); 
    assert(f1 != NULL);
    Cudd_Ref(f1);
    
    DdNode *f0 = Cudd_Cofactor(dd,f, Cudd_Not(dd->vars[mObjVar[pObj]])); 
    Cudd_Ref(f0);
    DdNode *fCare = Cudd_bddAnd(dd, f0, f1);
    fCare = Cudd_Not(fCare);
    Cudd_Ref(fCare);
    DdNode* newf;
    if(choice == 0) {
        newf = f;
        Cudd_Ref(f);
    }
    else if(choice == 1)
        newf = Cudd_bddMinimize(dd, f, fCare);
    else if (choice == 2) 
        newf = Cudd_bddRestrict(dd, f, fCare);
    else if(choice == 3) 
        newf = Cudd_bddConstrain(dd, f, fCare);
    else {
        DdNode *l = Cudd_bddAnd(dd, f1, fCare);
        Cudd_Ref(l);
        DdNode *u = Cudd_Not(Cudd_bddAnd(dd, f0, fCare));
        Cudd_Ref(u);
        newf = Cudd_bddSqueeze(dd, l, u);
        Cudd_RecursiveDeref(dd, l);
        Cudd_RecursiveDeref(dd, u);
    }
    Cudd_RecursiveDeref(dd, f1);
    Cudd_RecursiveDeref(dd, f0);
        
    Cudd_Ref(newf);
    Cudd_RecursiveDeref(dd, fCare);
    Cudd_RecursiveDeref(dd, f);
    f  = Cudd_Cofactor(dd,newf,dd->vars[mObjVar[pObj]]); 
    Cudd_Ref(f);
    Cudd_RecursiveDeref(dd, newf);
   
    
   // Cudd_RecursiveDeref(dd, f);
   // f = f1;
   // DdNode* newf;
    // 
    for(unsigned j = yNum + 1; j < vNameY.size(); j++) {
        pObj = Abc_NtkFindCi(pNtkNew, vNameY[j]);
        assert(pObj != NULL);
        DdNode *dPi = dd->vars[mObjVar[pObj]];
        newf = Cudd_bddExistAbstract(dd, f, dPi);
        Cudd_RecursiveDeref(dd, f);
        Cudd_Ref(newf);
        f = newf;
    }
    Abc_ObjSetGlobalBdd(Abc_NtkPo(pNtkNew, 0), f);

    Abc_Ntk_t* pNtkBdd = globalBdd2Ntk( pNtkNew );
    //Abc_NtkCecSat(pNtkNew, pNtkBdd, 100000, 0);


    for(unsigned j = yNum+1; j < vNameY.size(); j++) {
        assert(Abc_ObjFanoutNum(pObj = Abc_NtkFindCi(pNtkBdd, vNameY[j])) == 0);
        Abc_NtkDeleteObj(pObj);
    }
    
    vector<Abc_Obj_t*> vPi;
    Abc_NtkForEachPi(pNtkBdd, pObj, i) {
        if(Abc_ObjFanoutNum(pObj) == 0)
            vPi.push_back(pObj);
    }
    //assert(vPi.size() > 0);
    for(unsigned j = 0; j < vPi.size(); j++) {
        Abc_NtkDeleteObj(vPi[j]);
    }
    pNtkBdd = resyn(pNtkBdd);
    
    if(yNum == 0)
        *memUsed = checkMem();
    
    Abc_NtkDelete(pNtkNew);
    return pNtkBdd;
}

Abc_Ntk_t* singleRel2FuncBdd(Abc_Ntk_t* pNtk, vector<char*> &vNameY, int yNum, int long *memUsed) {
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

    DdManager *dd = (DdManager*)Abc_NtkBuildGlobalBdds(pNtkNew, 100000, 1, 1, 1, 0);
    cerr << "create BDD okay" << endl;
    if(yNum == 0)
        *memUsed = checkMem();
    if(dd == NULL) {
        Abc_NtkDelete(pNtkNew);
        return NULL;
    }
//    cerr << "Shared BDD size = " << Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) << " nodes." << endl;
   
    // map PI to bdd var
    map<const Abc_Obj_t*, int> mObjVar;
    Abc_NtkForEachCi(pNtkNew, pObj, i) {
        mObjVar[pObj] = i;
    }


    assert(Abc_NtkPoNum(pNtkNew) == 1);
    DdNode *f = (DdNode*)Abc_ObjGlobalBdd(Abc_NtkPo(pNtkNew, 0));
    // 
    for(unsigned j = yNum + 1; j < vNameY.size(); j++) {
        pObj = Abc_NtkFindCi(pNtkNew, vNameY[j]);
        assert(pObj != NULL);
        DdNode *dPi = dd->vars[mObjVar[pObj]];
        DdNode *newf = Cudd_bddExistAbstract(dd, f, dPi);
        Cudd_RecursiveDeref(dd, f);
        Cudd_Ref(newf);
        f = newf;
    }
    Abc_ObjSetGlobalBdd(Abc_NtkPo(pNtkNew, 0), f);

    Abc_Ntk_t* pNtkBdd = globalBdd2Ntk( pNtkNew );
    //Abc_NtkCecSat(pNtkNew, pNtkBdd, 100000, 0);


    for(unsigned j = yNum+1; j < vNameY.size(); j++) {
        assert(Abc_ObjFanoutNum(pObj = Abc_NtkFindCi(pNtkBdd, vNameY[j])) == 0);
        Abc_NtkDeleteObj(pObj);
    }
    
    vector<Abc_Obj_t*> vPi;
    Abc_NtkForEachPi(pNtkBdd, pObj, i) {
        if(Abc_ObjFanoutNum(pObj) == 0)
            vPi.push_back(pObj);
    }
    //assert(vPi.size() > 0);
    for(unsigned j = 0; j < vPi.size(); j++) {
        Abc_NtkDeleteObj(vPi[j]);
    }
    pNtkBdd = resyn(pNtkBdd);
    
    Abc_NtkDelete(pNtkNew);
    return pNtkBdd;
}

int long rel2FuncBdd(Abc_Ntk_t* pNtk, vector<char*> & vNameY, vector<Abc_Ntk_t*> &vFunc, int choice) {
    Abc_Ntk_t* pNtkFunc;
    Abc_Ntk_t* pNtkRel = Abc_NtkDup(pNtk);
    int long memUsed;
    cout << "function info during processing" << endl;
    cout << "-------------------------------" << endl;
    cout << "lev#\tnode#\tpi#" << endl;
    cout << "-------------------------------" << endl;
    for(unsigned i = 0; i < vNameY.size(); i++) {
        //cout << i << "\tRelation Node: " << Abc_NtkObjNum(pNtkRel) << endl;
        if(choice == 0)
            pNtkFunc = singleRel2FuncBdd(pNtkRel, vNameY, i, &memUsed);
        else   
            pNtkFunc = singleRel2FuncBdd2(pNtkRel, vNameY, i, &memUsed, choice - 1);
        if(pNtkFunc == NULL) {
            Abc_NtkDelete(pNtkFunc);
            Abc_NtkDelete(pNtkRel);
            return memUsed;
        }
        else if(Abc_NtkObjNum(pNtkFunc) > MAX_AIG_NODE) {
            cout <<  "The number of AIG nodes reached " << MAX_AIG_NODE << endl;
            Abc_NtkDelete(pNtkFunc);
            Abc_NtkDelete(pNtkRel);
            return memUsed;
        }

        pNtkRel = replaceX(pNtkRel, pNtkFunc, vNameY[i]);
        cout << Abc_AigLevel(pNtkFunc) << "\t" << Abc_NtkObjNum(pNtkFunc) << "\t" << Abc_NtkPiNum(pNtkFunc) << endl;
        vFunc.push_back(pNtkFunc);
    }
    checkRelFunc(pNtkRel, pNtk);
    Abc_NtkDelete(pNtkRel);
    return memUsed;
}

