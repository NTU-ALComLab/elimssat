#include <iostream>
#include <map>
#include "subRel2Func.hpp"
#include "CnfConvert.hpp"
#include "extMsat/Solver.h"

using namespace std;
bool isDisjoint(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB) {
    Abc_Ntk_t* pMultiA = Abc_NtkMulti( pNtkA, 0, 100, 1, 0, 0, 0 );
    Abc_Ntk_t* pMultiB = Abc_NtkMulti( pNtkB, 0, 100, 1, 0, 0, 0 );
    CnfConvert *ccA = new CnfConvert(pMultiA);
    CnfConvert *ccB = new CnfConvert(pMultiB);

    vector<vector<Lit> > pCnfA = ccA->getCnf();
    vector<vector<Lit> > pCnfB = ccB->getCnf();

    int maxVar = ccA->getMaxVar() + 1;
    IntSolver *pSat = new IntSolver();
    pSat->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 2);
    
    //cout << "PI id" << endl;
    map<const Var, char*> mPiName;
    map<const Var, bool> isPiVar;
    map<const Var, Var> mPiVar;
    int i;
    Abc_Obj_t *pPi, *pObj;
    Abc_NtkForEachPi(pMultiA, pObj, i) {
        pPi = Abc_NtkFindCi(pMultiB, Abc_ObjName(pObj));
        assert(pPi != NULL);
      //  assert(pObj->Id ==  Abc_NtkPi(pMultiB, i)->Id);
        mPiName[pObj->Id] = Abc_ObjName(pObj);
        isPiVar[pPi->Id] = true;
        mPiVar[pPi->Id] = pObj->Id;
     //   cout << pObj->Id << " " << endl;
    }
    // cout << endl;
   // cout << "a clauses" << endl;
    vec<Lit> lits;
    for(unsigned j = 0; j < pCnfA.size(); j++) {
        lits.clear();
        for(unsigned k = 0; k < pCnfA[j].size(); k++) {
            if(sign(pCnfA[j][k]))
                lits.push(~Lit(var(pCnfA[j][k])));
            else   
                lits.push(Lit(var(pCnfA[j][k])));
        }
#ifdef _DEBUG
       // printLits(lits);
#endif
        pSat->addAClause(lits);
    }

    //cout << "b clauses" << endl;
    for(unsigned j = 0; j < pCnfB.size(); j++) {
        lits.clear();
        for(unsigned k = 0; k < pCnfB[j].size(); k++) {
            if(sign(pCnfB[j][k])) 
                if(isPiVar[var(pCnfB[j][k])])
                    lits.push(~Lit(mPiVar[var(pCnfB[j][k])]));
                else         
                    lits.push(~Lit(var(pCnfB[j][k]) + maxVar));
            else   
                if(isPiVar[var(pCnfB[j][k])])
                    lits.push(Lit(mPiVar[var(pCnfB[j][k])]));
                else
                    lits.push(Lit(var(pCnfB[j][k]) + maxVar));
        }
#ifdef _DEBUG
      //  printLits(lits);
#endif
        pSat->addBClause(lits);
    }

    // add Po property
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiA, 0))->Id));
    pSat->addAClause(lits);
#ifdef _DEBUG
     //   printLits(lits);
#endif
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiB, 0))->Id + maxVar));
    pSat->addBClause(lits);
#ifdef _DEBUG
      //  printLits(lits);
#endif

   // assert(!pSat->solve());
    bool v = pSat->solve();

    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
 
    return !v;
}


Abc_Ntk_t* singleRel2Func2(Abc_Ntk_t* pNtk, char* yName) {

    //cerr << "start sinRel2func" << endl;
    Abc_Ntk_t *pNtkNewA = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Ntk_t *pNtkNewB = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;
    

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNewA);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {   
        if(strcmp(Abc_ObjName(pObj), yName) == 1) {
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


    //pObjNew = Abc_NtkCreatePo(pNtkNew);

    ntkAddOne(pNtk, pNtkNewA);
    Abc_Obj_t* pObj1 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

    pObj = Abc_NtkFindCi(pNtk, yName);
    pObj->pCopy = Abc_ObjNot(Abc_AigConst1(pNtkNewA));
    ntkAddOne(pNtk, pNtkNewA);

    Abc_Obj_t* pObj2 = Abc_ObjNot(Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0)));
    assert(pObj1);
    assert(pObj2);
    
    pObjNew = Abc_AigAnd((Abc_Aig_t*) pNtkNewA->pManFunc, pObj1, pObj2);
    
    Abc_Obj_t* pObjOut = Abc_NtkCreatePo(pNtkNewA);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pObjOut, name, NULL);
    Abc_ObjAddFanin( pObjOut, pObjNew);
    
    pNtkNewA = resyn(pNtkNewA);
    pNtkNewA = resyn(pNtkNewA);
    

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


    pNtkNewB = resyn(pNtkNewB);
    pNtkNewB = resyn(pNtkNewB);


    int count = 0; 
    Abc_NtkForEachPi(pNtk, pObj, i) {
        pObjNew = Abc_NtkFindCi(pNtkNewA, Abc_ObjName(pObj));
        if(pObjNew == NULL)  
            continue;

        Abc_Ntk_t* pNtkTemp = EQ(pNtkNewA, Abc_ObjName(pObjNew));
        if(isDisjoint(pNtkTemp, pNtkNewB)) {
            Abc_NtkDelete(pNtkNewA);
            pNtkNewA = pNtkTemp;
            count++;
        }
        else 
            Abc_NtkDelete(pNtkTemp);
    }
    cout << "EQ: " << count << " var" << endl;
/*
    Abc_NtkForEachPi( pNtkNew, pObj, i) {
        Abc_Ntk_t* pNtkTemp; 
    }
  */
    return pNtkNewA;
}

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

    ntkAddOne(pNtk, pNtkNew);
    pObjNew = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

    Abc_Obj_t* pObjOut = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pObjOut, name, NULL);
    Abc_ObjAddFanin( pObjOut, pObjNew);
    
    pNtkNew = resyn(pNtkNew);
    pNtkNew = resyn(pNtkNew);
    
    return pNtkNew;

}
void rel2FuncSub(Abc_Ntk_t* pNtk, vector<char*> & vNameY, vector<Abc_Ntk_t*> &vFunc) {
    Abc_Ntk_t* pNtkFunc;
    Abc_Ntk_t* pNtkRel = Abc_NtkDup(pNtk);
    pNtkRel = resyn(pNtkRel);
   // Abc_Ntk_t* pNtkRel == Abc_NtkCollapse( pNtk, 50000000, 0, 1, 0 );
    //cout << "# of bits: " << vNameY.size() << endl;
    cout << "function info during processing" << endl;
    cout << "-------------------------------" << endl;
    cout << "lev#\tnode#\tpi#" << endl;
    cout << "-------------------------------" << endl;
    for(unsigned i = 0; i < vNameY.size(); i++) {
        //cout << i << "\tRelation Node: " << Abc_NtkObjNum(pNtkRel) << endl;
        pNtkFunc = singleRel2Func(pNtkRel, vNameY[i]);
        if(Abc_NtkObjNum(pNtkFunc) > MAX_AIG_NODE) {
            cout <<  "The number of AIG nodes reached " << MAX_AIG_NODE << endl;
            Abc_NtkDelete(pNtkFunc);
            Abc_NtkDelete(pNtkRel);
            return;
        }

        vector<Abc_Obj_t*> vPi;
        int j;
        Abc_Obj_t* pObj;
        Abc_NtkForEachPi(pNtkFunc, pObj, j) {
            if(Abc_ObjFanoutNum(pObj) == 0)
                vPi.push_back(pObj);
        }
        for(unsigned k = 0; k < vPi.size(); k++) {
            Abc_NtkDeleteObj(vPi[k]);
        }

 //       pNtkFunc = singleRel2Func(pNtkRel, vNameY[i]);
        //cout << i << "\tFunction Node: " << Abc_NtkObjNum(pNtkFunc) << endl;

        cout << Abc_AigLevel(pNtkFunc) << "\t" << Abc_NtkObjNum(pNtkFunc) << "\t" << Abc_NtkPiNum(pNtkFunc) << endl;
        pNtkRel = replaceX(pNtkRel, pNtkFunc, vNameY[i]);
        for(unsigned j = 0; j < vFunc.size(); j++) {
            vFunc[j] = replaceX(vFunc[j], pNtkFunc, vNameY[i]);
        }
        vFunc.push_back(pNtkFunc);
        //     Abc_NtkDelete(pNtkFunc);
    }
    checkRelFunc(pNtkRel, pNtk);
    Abc_NtkDelete(pNtkRel);
}
