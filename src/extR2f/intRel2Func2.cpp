#include <iostream> 
#include <algorithm>
#include "intRel2Func2.hpp"
#include "subRel2Func.hpp"
#include "CnfConvert.hpp"
#include "IntSolver.hpp"
#include "extUtil/util.h"

//#define _DEBUG
using namespace std;

// all sat
// sort
Abc_Ntk_t* getIntFunc22(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB) {
    Abc_Ntk_t *pNtkNew;
    Abc_Ntk_t  *pMultiA, *pMultiB;
    pMultiA = Abc_NtkMulti( pNtkA, 0, 100, 1, 0, 0, 0 );
    pMultiB = Abc_NtkMulti( pNtkB, 0, 100, 1, 0, 0, 0 );
    CnfConvert *ccA = new CnfConvert(pMultiA);
    CnfConvert *ccB = new CnfConvert(pMultiB);
    
    int maxVar = ccA->getMaxVar() + 1;
    vector<vector<Lit> > pCnfA = ccA->getCnf();
    vector<vector<Lit> > pCnfB = ccB->getCnf();
    vector<vector<Lit> > pCnfTotal;
    
    map<const Var, char*> mPiName;
    map<const Var, bool> isPiVar;
    int i;
    Abc_Obj_t* pObj;
    
    // common var info
    //cout << "PI id" << endl;
    Abc_NtkForEachPi(pMultiA, pObj, i) {
        assert(strcmp(Abc_ObjName(pObj), Abc_ObjName( Abc_NtkPi(pMultiB, i))) == 0);
        assert(pObj->Id ==  Abc_NtkPi(pMultiB, i)->Id);
        mPiName[pObj->Id] = Abc_ObjName(pObj);
        isPiVar[pObj->Id] = true;
     //   cout << pObj->Id << " " << endl;
    }
    
    vector<Lit> vLits;
    
    for(unsigned j = 0; j < pCnfA.size(); j++) {
        vLits.clear();
        vLits.push_back(Lit(1));
        for(unsigned k = 0; k < pCnfA[j].size(); k++) {
            if(sign(pCnfA[j][k]))
                vLits.push_back(~Lit(var(pCnfA[j][k])));
            else   
                vLits.push_back(Lit(var(pCnfA[j][k])));
        }
        pCnfTotal.push_back(vLits);
    }
    
    vLits.clear();
    vLits.push_back(Lit(1));
    vLits.push_back(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiA, 0))->Id));
    pCnfTotal.push_back(vLits);
    
    for(unsigned j = 0; j < pCnfB.size(); j++) {
        vLits.clear();
        vLits.push_back(~Lit(1));
        for(unsigned k = 0; k < pCnfB[j].size(); k++) {
            if(sign(pCnfB[j][k])) 
                if(isPiVar[var(pCnfB[j][k])])
                    vLits.push_back(~Lit(var(pCnfB[j][k])));
                else         
                    vLits.push_back(~Lit(var(pCnfB[j][k]) + maxVar));
            else   
                if(isPiVar[var(pCnfB[j][k])])
                    vLits.push_back(Lit(var(pCnfB[j][k])));
                else
                    vLits.push_back(Lit(var(pCnfB[j][k]) + maxVar));
        
        }
        pCnfTotal.push_back(vLits);
    }
    
    vLits.clear();
    vLits.push_back(~Lit(1));
    vLits.push_back(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiB, 0))->Id + maxVar));
    pCnfTotal.push_back(vLits);
    
    
    //sort(pCnfTotal.begin(), pCnfTotal.end(), cmp);
    //random_shuffle(pCnfTotal.begin(), pCnfTotal.end());
    IntSolver *pSat = new IntSolver();
    
    
    Var ctrVar = ccA->getMaxVar() + ccB->getMaxVar() + 2;
 //   Var ctrStr = ctrVar;
    vec<Lit> ctrLits;

    pSat->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 2 + pCnfA.size() + pCnfB.size() + 1 + 2);
    ctrLits.clear();
    // cout << endl;
    
    vec<Lit> lits;
    for(unsigned j = 0; j < pCnfTotal.size(); j++) {
        lits.clear();
        for(unsigned k = 1; k < pCnfTotal[j].size(); k++) {
            if(sign(pCnfTotal[j][k]))
                lits.push(~Lit(var(pCnfTotal[j][k])));
            else   
                lits.push(Lit(var(pCnfTotal[j][k])));
        }
        lits.push(Lit(ctrVar));
        ctrLits.push(~Lit(ctrVar++));
//#ifdef _DEBUG
       // printLits(lits);
//#endif
        assert(var(pCnfTotal[j][0]) == 1);
        if(sign(pCnfTotal[j][0])) 
            pSat->addBClause(lits);
        else
            pSat->addAClause(lits);
      //  if(!pSat->solve(ctrLits))
        //    break;
    }

 //   cerr << "start solve" << endl;
 //   cerr << "end solve" << endl;

 //   cerr << "start getint" << endl;
    //assert(!pSat->solve(ctrLits));
    if(pSat->solve(ctrLits)) 
        pNtkNew = NULL;
    else 
        pNtkNew = pSat->getInt(mPiName);
 //   cerr << "end getint" << endl;
    delete pSat;
    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    return pNtkNew;
}
// decision order
// unsat core & EQ
// EQ first

// original 
Abc_Ntk_t* singleRel2FuncInt2(Abc_Ntk_t* pNtk, char* yName, vector<char*> &vNameY) {

    //cerr << "start sinRel2funcInt" << endl;
    Abc_Ntk_t *pNtkNewA = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Ntk_t *pNtkNewB = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Ntk_t *pNtkFunc;
    Abc_Obj_t *pObj, *pObjNew, *pObj1, *pObj2;
    int i;

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNewA);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        if(strcmp(Abc_ObjName(pObj), yName) == 0) {
            pObjNew = Abc_AigConst1(pNtkNewA);
        }
        else {
            pObjNew = Abc_NtkCreatePi( pNtkNewA );
            // add name
            Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        }
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }
    ntkAddOne(pNtk, pNtkNewA);

    pObj1 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

    pObj = Abc_NtkFindCi(pNtk, yName);
    pObj->pCopy = Abc_ObjNot(Abc_AigConst1(pNtkNewA));
    ntkAddOne(pNtk, pNtkNewA);

    pObj2 = Abc_ObjNot(Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0)));
    assert(pObj1);
    assert(pObj2);

    pObjNew = Abc_AigAnd((Abc_Aig_t*) pNtkNewA->pManFunc, pObj1, pObj2);

    Abc_Obj_t* pObjOut = Abc_NtkCreatePo(pNtkNewA);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pObjOut, name, NULL);
    Abc_ObjAddFanin( pObjOut, pObjNew);



    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNewB);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        if(strcmp(Abc_ObjName(pObj), yName) == 0) {
            pObjNew = Abc_ObjNot(Abc_AigConst1(pNtkNewB));
        }
        else {
            pObjNew = Abc_NtkCreatePi( pNtkNewB );
            // add name
            Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        }
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }
    ntkAddOne(pNtk, pNtkNewB);

    pObj1 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

    pObj = Abc_NtkFindCi(pNtk, yName);
    pObj->pCopy = Abc_AigConst1(pNtkNewB);
    ntkAddOne(pNtk, pNtkNewB);

    pObj2 = Abc_ObjNot(Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0)));
    assert(pObj1);
    assert(pObj2);

    pObjNew = Abc_AigAnd((Abc_Aig_t*) pNtkNewB->pManFunc, pObj1, pObj2);

    pObjOut = Abc_NtkCreatePo(pNtkNewB);
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pObjOut, name, NULL);
    Abc_ObjAddFanin( pObjOut, pObjNew);

    pNtkNewA = resyn(pNtkNewA);
    pNtkNewB = resyn(pNtkNewB);
  //  cerr << "start add clause" << endl;
    pNtkFunc = getIntFunc22(pNtkNewA, pNtkNewB);
  //  cerr << "end add clause" << endl;
 //   pNtkFunc = getIntFunc2(pNtkNewA, pNtkNewB, vNameY);
 //   Abc_Ntk_t *pNtkTemp = getIntFunc6(pNtkNewA, pNtkNewB);
  //  cout << "ori: " << Abc_AigLevel(pNtkTemp) << "\t" << Abc_NtkObjNum(pNtkTemp) << endl;
   // Abc_NtkDelete(pNtkTemp);
    Abc_NtkDelete(pNtkNewA);
    Abc_NtkDelete(pNtkNewB);

    return pNtkFunc;

}

Abc_Ntk_t* rel2FuncInt2(Abc_Ntk_t* pNtk, vector<char*> & vNameY, vector<Abc_Ntk_t*> &vFunc) {
    Abc_Ntk_t* pNtkFunc;
    Abc_Ntk_t* pNtkRel = Abc_NtkDup(pNtk);
    pNtkRel = resyn(pNtkRel);

    vector<Abc_Ntk_t*> vNtkRel;
   // Abc_Ntk_t* pNtkRel == Abc_NtkCollapse( pNtk, 50000000, 0, 1, 0 );
    cout << "function info during processing" << endl;
    cout << "-------------------------------" << endl;
    cout << "soltype\tlev#\tnode#\tpi#\trel-lev#\trel-node#" << endl;
    cout << "-------------------------------" << endl;
    for(unsigned i = 0; i < vNameY.size(); i++) {
        //cout << i << "\tRelation Node: " << Abc_NtkObjNum(pNtkRel) << endl;
        cout << i << "\t";
        if(i < vNameY.size() - 1)
            vNtkRel.push_back(Abc_NtkDup(pNtkRel));
        //cout << "begin solve\n" << endl;
        pNtkFunc = singleRel2FuncInt2(pNtkRel, vNameY[i], vNameY);
        //cout << "end solve\n" << endl;
        Abc_Ntk_t* pNtkFuncTemp = singleRel2Func(pNtkRel, vNameY[i]);
        if(pNtkFunc == NULL) {
            pNtkFunc = pNtkFuncTemp;
            cout << "0\t";
        }
        // else {
        else if(Abc_NtkObjNum(pNtkFuncTemp) > Abc_NtkObjNum(pNtkFunc)) {
            pNtkFunc = resyn(pNtkFunc);
            Abc_NtkDelete(pNtkFuncTemp);
            cout << "1\t";
        }
        else {
            Abc_NtkDelete(pNtkFunc);
            pNtkFunc = pNtkFuncTemp;
            cout << "2\t";
        }
       /* if(pNtkFunc == NULL) {
            cout <<  "sat time out" << MAX_AIG_NODE << endl;
            Abc_NtkDelete(pNtkRel);
            return;
        }*/
        if(Abc_NtkObjNum(pNtkFunc) > MAX_AIG_NODE) {
            cout <<  "The number of AIG nodes reached " << MAX_AIG_NODE << endl;
            Abc_NtkDelete(pNtkFunc);
            Abc_NtkDelete(pNtkRel);
            return NULL;
        }

        pNtkFunc = Util_NtkDFraig(pNtkFunc, 1, 1);
        pNtkFunc = Util_NtkResyn2(pNtkFunc, 1);
        pNtkFunc = Util_NtkDc2(pNtkFunc, 1);
        pNtkRel = replaceX(pNtkRel, pNtkFunc, vNameY[i]);
        pNtkRel = Util_NtkDFraig(pNtkRel, 1, 1);
        pNtkRel = Util_NtkResyn2(pNtkRel, 1);
        pNtkRel = Util_NtkDc2(pNtkRel, 1);
        cout << Abc_AigLevel(pNtkFunc) << "\t" << Abc_NtkObjNum(pNtkFunc) << "\t" << Abc_NtkPiNum(pNtkFunc) << "\t" << Abc_AigLevel(pNtkRel) << "\t" << Abc_NtkObjNum(pNtkRel) << endl;
        /*for(unsigned j = 0; j < vFunc.size(); j++) {
            vFunc[j] = replaceX(vFunc[j], pNtkFunc, vNameY[i]);
        }*/
        vFunc.push_back(pNtkFunc);
        //if(Abc_NtkObjNum(pNtkFunc) > 50000)
        //    break;
        //     Abc_NtkDelete(pNtkFunc);
    }
    // phase 2
//    cout << "vNtk size: " << vNtkRel.size() << endl;
    //for(int i = vNtkRel.size() - 1; i >= 0; i--) {
    //    // replace pi with function 
    //    for(int j = 0; j <= i; j++) {
    //        vNtkRel[j] = replaceX(vNtkRel[j], vFunc[i+1], vNameY[i+1]);
    ////        printf("replace rel %d with func %d\n", j, i+1);
    //    }
    //    //cout << "begin solve\n" << endl;
    //    pNtkFunc = singleRel2FuncInt2(vNtkRel[i], vNameY[i], vNameY);
    //    //cout << "end solve\n" << endl;
    //    Abc_Ntk_t* pNtkFuncTemp = singleRel2Func(vNtkRel[i], vNameY[i]);
        
    //    if(pNtkFunc == NULL) {
    //        pNtkFunc = pNtkFuncTemp;
  ////          cout << "0\t";
    //    }
    //    else if(Abc_NtkObjNum(pNtkFuncTemp) > Abc_NtkObjNum(pNtkFunc)) {
    //        pNtkFunc = resyn(pNtkFunc);
    //        Abc_NtkDelete(pNtkFuncTemp);
    ////        cout << "1\t";
    //    }
    //    else {
    //        Abc_NtkDelete(pNtkFunc);
    //        pNtkFunc = pNtkFuncTemp;
    //  //      cout << "0\t";
    //    }/*
    //    if(pNtkFunc == NULL) {
    //        cout <<  "sat time out" << MAX_AIG_NODE << endl;
    //        Abc_NtkDelete(pNtkRel);
    //        return;
    //    }*/
    //    if(Abc_NtkObjNum(pNtkFunc) > MAX_AIG_NODE) {
    //        cout <<  "The number of AIG nodes reached " << MAX_AIG_NODE << endl;
    //        Abc_NtkDelete(pNtkFunc);
    //        Abc_NtkDelete(pNtkRel);
    //        return NULL;
    //    }
    //    Abc_NtkDelete(vFunc[i]);
    //    vFunc[i] = pNtkFunc;
    //    cout << i << "\t" <<  Abc_AigLevel(pNtkFunc) << "\t" << Abc_NtkObjNum(pNtkFunc) << "\t" << Abc_NtkPiNum(pNtkFunc) << endl;
    //}
    vNtkRel.clear();
    // checkRelFunc(pNtkRel, pNtk);
    return pNtkRel;
}
