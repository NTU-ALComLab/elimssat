#include <iostream> 
#include <algorithm>
#include "intRel2Func.hpp"
#include "subRel2Func.hpp"
#include "CnfConvert.hpp"
#include "IntSolver.hpp"
#include "base/io/ioAbc.h"
#include "extUtil/util.h"

//#define _DEBUG
using namespace std;

bool ABsat(CnfConvert* ccA, CnfConvert* ccB, Abc_Ntk_t* pMultiA, Abc_Ntk_t* pMultiB) {
    

    vector<vector<Lit> > pCnfA = ccA->getCnf();
    vector<vector<Lit> > pCnfB = ccB->getCnf();
    IntSolver *pSat = new IntSolver();
    pSat->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 2);
    //cout << "PI id" << endl;
    map<const Var, char*> mPiName;
    map<const Var, bool> isPiVar;
    map<const Var, Var> mPiVar;
    int maxVar = ccA->getMaxVar() + 1;
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
    delete pSat;
   // delete ccA;
   // delete ccB;
   // Abc_NtkDelete(pMultiA);
   // Abc_NtkDelete(pMultiB);
    return v;
}
bool cmp(vector<Lit> a, vector<Lit> b) {
    return a.size() < b.size();
}
// sort
Abc_Ntk_t* getIntFunc2(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB) {
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
    
    
    sort(pCnfTotal.begin(), pCnfTotal.end(), cmp);
   // random_shuffle(pCnfTotal.begin(), pCnfTotal.end());
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
   // cerr << "end solve" << endl;

   // cerr << "start getint" << endl;
    //assert(!pSat->solve(ctrLits));
    if(pSat->solve(ctrLits)) {
        pNtkNew = NULL;
    }
    else
        pNtkNew = pSat->getInt(mPiName);
   // cerr << "end getint" << endl;
    delete pSat;
    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    return pNtkNew;
}

// random 
Abc_Ntk_t* getIntFunc3(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB) {
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
    random_shuffle(pCnfTotal.begin(), pCnfTotal.end());
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
   // cerr << "end solve" << endl;

   // cerr << "start getint" << endl;
    //assert(!pSat->solve(ctrLits));
    if(pSat->solve(ctrLits)) {
        pNtkNew = NULL;
    }
    else
        pNtkNew = pSat->getInt(mPiName);
   // cerr << "end getint" << endl;
    delete pSat;
    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    return pNtkNew;
}
// ctrl var
Abc_Ntk_t* getIntFunc5(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB) {
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
   // random_shuffle(pCnfTotal.begin(), pCnfTotal.end());
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
   // cerr << "end solve" << endl;

   // cerr << "start getint" << endl;
    //assert(!pSat->solve(ctrLits));
    if(pSat->solve(ctrLits)) {
        pNtkNew = NULL;
    }
    else
        pNtkNew = pSat->getInt(mPiName);
   // cerr << "end getint" << endl;
    delete pSat;
    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    return pNtkNew;
}
// decision order with ctrl variable
Abc_Ntk_t* getIntFunc6(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB) {
    Abc_Ntk_t *pNtkNew;
    Abc_Ntk_t  *pMultiA, *pMultiB;
    pMultiA = Abc_NtkMulti( pNtkA, 0, 100, 1, 0, 0, 0 );
    pMultiB = Abc_NtkMulti( pNtkB, 0, 100, 1, 0, 0, 0 );
    CnfConvert *ccA = new CnfConvert(pMultiA);
    CnfConvert *ccB = new CnfConvert(pMultiB);
    vector<vector<Lit> > pCnfA = ccA->getCnf();
    vector<vector<Lit> > pCnfB = ccB->getCnf();

    //sort(pCnfA.begin(), pCnfA.end(), cmp);
    int maxVar = ccA->getMaxVar() + 1;

    IntSolver *pSat = new IntSolver();
    
    
    Var ctrVar = ccA->getMaxVar() + ccB->getMaxVar() + 2;
 //   Var ctrStr = ctrVar;
    vec<Lit> ctrLits;

    pSat->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 2 + pCnfA.size() + pCnfB.size() + 1 + 2);
    map<const Var, char*> mPiName;
    map<const Var, bool> isPiVar;
    int i;
    Abc_Obj_t* pObj;
    //cout << "PI id" << endl;
    Abc_NtkForEachPi(pMultiA, pObj, i) {
        assert(strcmp(Abc_ObjName(pObj), Abc_ObjName( Abc_NtkPi(pMultiB, i))) == 0);
        assert(pObj->Id ==  Abc_NtkPi(pMultiB, i)->Id);
        mPiName[pObj->Id] = Abc_ObjName(pObj);
        //mPiName[pObj->Id + maxVar] = Abc_ObjName(pObj);
        isPiVar[pObj->Id] = true;
     //   cout << pObj->Id << " " << endl;
    }
    ctrLits.clear();
    // cout << endl;
 //    cout << "a clauses" << endl;
    vec<Lit> lits;
    
    // add Po property
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiA, 0))->Id));
        lits.push(Lit(ctrVar));
        ctrLits.push(~Lit(ctrVar++));
    pSat->addAClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiB, 0))->Id + maxVar));
        lits.push(Lit(ctrVar));
        ctrLits.push(~Lit(ctrVar++));
    pSat->addBClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif
    
    
    for(unsigned j = 0; j < pCnfA.size(); j++) {
        lits.clear();
        for(unsigned k = 0; k < pCnfA[j].size(); k++) {
            if(sign(pCnfA[j][k]))
     //           if(isPiVar[var(pCnfA[j][k])])
       //             lits.push(~Lit(var(pCnfA[j][k]) + maxVar));
         //       else
                    lits.push(~Lit(var(pCnfA[j][k])));
            else   
           //     if(isPiVar[var(pCnfA[j][k])])
             //       lits.push(Lit(var(pCnfA[j][k]) + maxVar));
              //  else
                    lits.push(Lit(var(pCnfA[j][k])));
        }
        lits.push(Lit(ctrVar));
        ctrLits.push(~Lit(ctrVar++));
#ifdef _DEBUG
        printLits(lits);
#endif
        pSat->addAClause(lits);
    }

    //cout << "b clauses" << endl;
    for(unsigned j = 0; j < pCnfB.size(); j++) {
        lits.clear();
        for(unsigned k = 0; k < pCnfB[j].size(); k++) {
            if(sign(pCnfB[j][k])) 
                if(isPiVar[var(pCnfB[j][k])])
                    lits.push(~Lit(var(pCnfB[j][k])));
                else         
                    lits.push(~Lit(var(pCnfB[j][k]) + maxVar));
            else   
                if(isPiVar[var(pCnfB[j][k])])
                    lits.push(Lit(var(pCnfB[j][k])));
                else
                    lits.push(Lit(var(pCnfB[j][k]) + maxVar));
        }
        lits.push(Lit(ctrVar));
        ctrLits.push(~Lit(ctrVar++));
#ifdef _DEBUG
        printLits(lits);
#endif
        pSat->addBClause(lits);
    }



    //cout << "orig cls#: \t" << pCnfA.size() + pCnfB.size() + 2 << endl;
    assert(!pSat->solve(ctrLits));
    /*cout << "proof cls#: \t" << pSat->conflict.size() << endl;
    for(int j = 0; j < ctrLits.size(); j++) {
        ctrLits[j] = Lit(var(ctrLits[j]));
    }
    for(int j = 0; j < pSat->conflict.size(); j++) {
        int k = var(pSat->conflict[j]) - ctrStr;
        ctrLits[k] = ~Lit(var(ctrLits[k]));
    }
    assert(!pSat->solve(ctrLits));
    for(int j = 0; j < pSat->conflict.size(); j++) {
        int k = var(pSat->conflict[j]) - ctrStr;
        if(!sign(ctrLits[k]))
            continue;

        ctrLits[k] = Lit(var(ctrLits[k]));
        if(pSat->solve(ctrLits)) {
            ctrLits[k] = ~Lit(var(ctrLits[k]));
        }
        //else 
           // cout << "get 1 benefit" << endl;
    }
    cout << "min cls#: \t" << pSat->conflict.size() << endl;
    assert(!pSat->solve(ctrLits));
    */

    pNtkNew = pSat->getInt(mPiName);
    delete pSat;
    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    return pNtkNew;
}
// decision order without ctrl variables
Abc_Ntk_t* getIntFunc4(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB) {
    Abc_Ntk_t *pNtkNew;
    Abc_Ntk_t  *pMultiA, *pMultiB;
    pMultiA = Abc_NtkMulti( pNtkA, 0, 100, 1, 0, 0, 0 );
    pMultiB = Abc_NtkMulti( pNtkB, 0, 100, 1, 0, 0, 0 );
    CnfConvert *ccA = new CnfConvert(pMultiA);
    CnfConvert *ccB = new CnfConvert(pMultiB);
    vector<vector<Lit> > pCnfA = ccA->getCnf();
    vector<vector<Lit> > pCnfB = ccB->getCnf();

    //sort(pCnfA.begin(), pCnfA.end(), cmp);
    int maxVar = ccA->getMaxVar() + 1;

    IntSolver *pSat = new IntSolver();
    
    
 //   Var ctrStr = ctrVar;

    pSat->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 2 + pCnfA.size() + pCnfB.size() + 1 + 2);
    map<const Var, char*> mPiName;
    map<const Var, bool> isPiVar;
    int i;
    Abc_Obj_t* pObj;
    //cout << "PI id" << endl;
    Abc_NtkForEachPi(pMultiA, pObj, i) {
        assert(strcmp(Abc_ObjName(pObj), Abc_ObjName( Abc_NtkPi(pMultiB, i))) == 0);
        assert(pObj->Id ==  Abc_NtkPi(pMultiB, i)->Id);
        mPiName[pObj->Id] = Abc_ObjName(pObj);
        //mPiName[pObj->Id + maxVar] = Abc_ObjName(pObj);
        isPiVar[pObj->Id] = true;
     //   cout << pObj->Id << " " << endl;
    }
    // cout << endl;
 //    cout << "a clauses" << endl;
    vec<Lit> lits;
    
    // add Po property
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiA, 0))->Id));
    pSat->addAClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiB, 0))->Id + maxVar));
    pSat->addBClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif
    
    
    for(unsigned j = 0; j < pCnfA.size(); j++) {
        lits.clear();
        for(unsigned k = 0; k < pCnfA[j].size(); k++) {
            if(sign(pCnfA[j][k]))
     //           if(isPiVar[var(pCnfA[j][k])])
       //             lits.push(~Lit(var(pCnfA[j][k]) + maxVar));
         //       else
                    lits.push(~Lit(var(pCnfA[j][k])));
            else   
           //     if(isPiVar[var(pCnfA[j][k])])
             //       lits.push(Lit(var(pCnfA[j][k]) + maxVar));
              //  else
                    lits.push(Lit(var(pCnfA[j][k])));
        }
#ifdef _DEBUG
        printLits(lits);
#endif
        pSat->addAClause(lits);
    }

    //cout << "b clauses" << endl;
    for(unsigned j = 0; j < pCnfB.size(); j++) {
        lits.clear();
        for(unsigned k = 0; k < pCnfB[j].size(); k++) {
            if(sign(pCnfB[j][k])) 
                if(isPiVar[var(pCnfB[j][k])])
                    lits.push(~Lit(var(pCnfB[j][k])));
                else         
                    lits.push(~Lit(var(pCnfB[j][k]) + maxVar));
            else   
                if(isPiVar[var(pCnfB[j][k])])
                    lits.push(Lit(var(pCnfB[j][k])));
                else
                    lits.push(Lit(var(pCnfB[j][k]) + maxVar));
        }
#ifdef _DEBUG
        printLits(lits);
#endif
        pSat->addBClause(lits);
    }



    //cout << "orig cls#: \t" << pCnfA.size() + pCnfB.size() + 2 << endl;
    assert(!pSat->solve());
    /*cout << "proof cls#: \t" << pSat->conflict.size() << endl;
    for(int j = 0; j < ctrLits.size(); j++) {
        ctrLits[j] = Lit(var(ctrLits[j]));
    }
    for(int j = 0; j < pSat->conflict.size(); j++) {
        int k = var(pSat->conflict[j]) - ctrStr;
        ctrLits[k] = ~Lit(var(ctrLits[k]));
    }
    assert(!pSat->solve(ctrLits));
    for(int j = 0; j < pSat->conflict.size(); j++) {
        int k = var(pSat->conflict[j]) - ctrStr;
        if(!sign(ctrLits[k]))
            continue;

        ctrLits[k] = Lit(var(ctrLits[k]));
        if(pSat->solve(ctrLits)) {
            ctrLits[k] = ~Lit(var(ctrLits[k]));
        }
        //else 
           // cout << "get 1 benefit" << endl;
    }
    cout << "min cls#: \t" << pSat->conflict.size() << endl;
    assert(!pSat->solve(ctrLits));
    */

    pNtkNew = pSat->getInt(mPiName);
    delete pSat;
    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    return pNtkNew;
}
// unsat core & EQ
Abc_Ntk_t* getIntFunc8(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB, vector<char*> &vNameY) {

    Abc_Ntk_t *pNtkNew;
    Abc_Ntk_t  *pMultiA, *pMultiB;
    pMultiB = Abc_NtkMulti( pNtkB, 0, 100, 1, 0, 0, 0 );
    CnfConvert *ccA;
    CnfConvert *ccB = new CnfConvert(pMultiB);
    vector<vector<Lit> > pCnfB = ccB->getCnf();



    map<const Var, char*> mPiName;
    map<const Var, bool> isPiVar;
    map<const Var, Var> mPiVar;
    int i;

   // Abc_Ntk_t* pNtkNewA = pNtkA;
    Abc_Obj_t* pObj;
    pNtkA = Abc_NtkDup(pNtkA);
    int benefit = 0;
    for(unsigned j = 0; j < vNameY.size(); j++) {
        if(Abc_NtkFindCi(pNtkA, vNameY[j]) == NULL) 
            continue;
        Abc_Ntk_t* pNtkNewA = EQ(pNtkA, vNameY[j]);
        pMultiA = Abc_NtkMulti( pNtkNewA, 0, 100, 1, 0, 0, 0 );
        ccA = new CnfConvert(pMultiA);
        if(ABsat(ccA, ccB, pMultiA, pMultiB)) {
            Abc_NtkDelete(pNtkNewA);
        }
        else {
            Abc_NtkDelete(pNtkA);
            pNtkA = pNtkNewA;
            benefit++;
        }
        Abc_NtkDelete(pMultiA);
        delete ccA;
    }
 //   cout << "benefit: " << benefit << endl;
    
    pMultiA = Abc_NtkMulti( pNtkA, 0, 100, 1, 0, 0, 0 );
    ccA = new CnfConvert(pMultiA);
    vector<vector<Lit> > pCnfA = ccA->getCnf();
    vector<vector<Lit> > pCnfTotal; 
    map<const Var, unsigned> mClause;
    vec<Lit> ctrLits;
    Var ctrVar = ccA->getMaxVar() + ccB->getMaxVar() + 2;
    IntSolver *pSat = new IntSolver();
    pSat->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 2 + pCnfA.size() + pCnfB.size() + 1);
    int maxVar = ccA->getMaxVar() + 1;
   // cout << "PI id" << endl;
    
    Abc_Obj_t* pPi;
    Abc_NtkForEachPi(pMultiA, pObj, i) {
        pPi = Abc_NtkFindCi(pMultiB, Abc_ObjName(pObj));
        assert(pPi != NULL);
      //  assert(pObj->Id ==  Abc_NtkPi(pMultiB, i)->Id);
        mPiName[pObj->Id] = Abc_ObjName(pObj);
        isPiVar[pPi->Id] = true;
        mPiVar[pPi->Id] = pObj->Id;
        //cout << pPi->Id << endl;
    }
    ctrLits.clear();
    pCnfTotal.clear();
    // cout << endl;
    cout << "a clauses" << endl;
    vec<Lit> lits;
    vector<Lit> vLits;
    for(unsigned j = 0; j < pCnfA.size(); j++) {
        lits.clear();
        vLits.clear();
        for(unsigned k = 0; k < pCnfA[j].size(); k++) {
            if(sign(pCnfA[j][k])) {
                lits.push(~Lit(var(pCnfA[j][k])));
                vLits.push_back(~Lit(var(pCnfA[j][k])));
            }
            else {   
                lits.push(Lit(var(pCnfA[j][k])));
                vLits.push_back(Lit(var(pCnfA[j][k])));
            }
        }
        lits.push(Lit(ctrVar));
        ctrLits.push(~Lit(ctrVar));
        mClause[ctrVar++] = pCnfTotal.size();

#ifdef _DEBUG
        printLits(lits);
#endif
        pSat->addAClause(lits);
        pCnfTotal.push_back(vLits);
    }
    
    int sepAB = pCnfTotal.size();
    cout << "b clauses" << endl;
    for(unsigned j = 0; j < pCnfB.size(); j++) {
        lits.clear();
        vLits.clear();
        for(unsigned k = 0; k < pCnfB[j].size(); k++) {
            if(sign(pCnfB[j][k])) 
                if(isPiVar[var(pCnfB[j][k])]) {
                    lits.push(~Lit(mPiVar[var(pCnfB[j][k])]));
                    vLits.push_back(~Lit(mPiVar[var(pCnfB[j][k])]));
                }
                else {         
                    lits.push(~Lit(var(pCnfB[j][k]) + maxVar));
                    vLits.push_back(~Lit(var(pCnfB[j][k]) + maxVar));
                }
            else   
                if(isPiVar[var(pCnfB[j][k])]) {
                    lits.push(Lit(mPiVar[var(pCnfB[j][k])]));
                    vLits.push_back(Lit(mPiVar[var(pCnfB[j][k])]));
                }
                else {
                    lits.push(Lit(var(pCnfB[j][k]) + maxVar));
                    vLits.push_back(Lit(var(pCnfB[j][k]) + maxVar));
                }
        }
        
        lits.push(Lit(ctrVar));
        ctrLits.push(~Lit(ctrVar));
        mClause[ctrVar++] = pCnfTotal.size();
#ifdef _DEBUG
        printLits(lits);
#endif
        pSat->addBClause(lits);
        pCnfTotal.push_back(vLits);
    }
    // add Po property
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiA, 0))->Id));
    pSat->addAClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiB, 0))->Id + maxVar));
    pSat->addBClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif

    cout << "total clauses: " << pCnfTotal.size() << endl;
    assert(!pSat->solve(ctrLits));
    cout << "unsat core clauses: " << pSat->conflict.size() << endl;
   // pNtkNew = pSat->getInt(mPiName);
    IntSolver *pSatNew = new IntSolver();
    //pSatNew->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 3 + pCnfA.size() + pCnfB.size() + 1);
    pSatNew->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 2);
    for(int j = 0; j < pSat->conflict.size(); j++) {
        lits.clear();
        int n = mClause[var(pSat->conflict[j])];
        for(unsigned k = 0; k < pCnfTotal[n].size(); k++) {
            if(sign(pCnfTotal[n][k]))
                lits.push(~Lit(var(pCnfTotal[n][k])));
            else 
                lits.push(Lit(var(pCnfTotal[n][k])));
        }
        if(n < sepAB)
            pSatNew->addAClause(lits);
        else
            pSatNew->addBClause(lits);
    }

    // add Po property
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiA, 0))->Id));
    pSatNew->addAClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiB, 0))->Id + maxVar));
    pSatNew->addBClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif


    assert(!pSatNew->solve());
    pNtkNew = pSatNew->getInt(mPiName);
    
    delete pSatNew; 
    delete pSat;

    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    return pNtkNew;
}
// EQ first
Abc_Ntk_t* getIntFunc7(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB, vector<char*> &vNameY) {

    Abc_Ntk_t *pNtkNew;
    Abc_Ntk_t  *pMultiA, *pMultiB;
    pMultiB = Abc_NtkMulti( pNtkB, 0, 100, 1, 0, 0, 0 );
    CnfConvert *ccA;
    CnfConvert *ccB = new CnfConvert(pMultiB);
    vector<vector<Lit> > pCnfB = ccB->getCnf();



    map<const Var, char*> mPiName;
    map<const Var, bool> isPiVar;
    map<const Var, Var> mPiVar;
    int i;

   // Abc_Ntk_t* pNtkNewA = pNtkA;
    map<const Abc_Obj_t*, bool> isYPi;
    for(unsigned j = 0; j < vNameY.size(); j++) {
        if(Abc_NtkFindCi(pNtkA, vNameY[j]) != NULL) {
            isYPi[Abc_NtkFindCi(pNtkA, vNameY[j])] = true;
        }
    }

    Abc_Obj_t* pObj;
    pNtkA = Abc_NtkDup(pNtkA);
    int benefit = 0;
    Abc_NtkForEachPi(pNtkA, pObj, i) {
        if(isYPi[pObj])
            continue;

        Abc_Ntk_t* pNtkNewA = EQ(pNtkA, Abc_ObjName(pObj));
        pMultiA = Abc_NtkMulti( pNtkNewA, 0, 100, 1, 0, 0, 0 );
        ccA = new CnfConvert(pMultiA);
        if(ABsat(ccA, ccB, pMultiA, pMultiB)) {
            Abc_NtkDelete(pNtkNewA);
        }
        else {
            Abc_NtkDelete(pNtkA);
            pNtkA = pNtkNewA;
            benefit++;
        }
        Abc_NtkDelete(pMultiA);
        delete ccA;
    }
 //   cout << "benefit: " << benefit << endl;
    
    pMultiA = Abc_NtkMulti( pNtkA, 0, 100, 1, 0, 0, 0 );
    ccA = new CnfConvert(pMultiA);
    vector<vector<Lit> > pCnfA = ccA->getCnf();
    IntSolver *pSat = new IntSolver();
    pSat->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 2);
    int maxVar = ccA->getMaxVar() + 1;
   // cout << "PI id" << endl;
    Abc_Obj_t* pPi;
    Abc_NtkForEachPi(pMultiA, pObj, i) {
        pPi = Abc_NtkFindCi(pMultiB, Abc_ObjName(pObj));
        assert(pPi != NULL);
      //  assert(pObj->Id ==  Abc_NtkPi(pMultiB, i)->Id);
        mPiName[pObj->Id] = Abc_ObjName(pObj);
        isPiVar[pPi->Id] = true;
        mPiVar[pPi->Id] = pObj->Id;
        //cout << pPi->Id << endl;
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
        printLits(lits);
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
        printLits(lits);
#endif
        pSat->addBClause(lits);
    }

    // add Po property
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiA, 0))->Id));
    pSat->addAClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiB, 0))->Id + maxVar));
    pSat->addBClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif

    assert(!pSat->solve());
    pNtkNew = pSat->getInt(mPiName);
    delete pSat;
    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    return pNtkNew;
}

// original 
Abc_Ntk_t* getIntFunc(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB) {
    Abc_Ntk_t *pNtkNew;
    Abc_Ntk_t  *pMultiA, *pMultiB;
    pMultiA = Abc_NtkMulti( pNtkA, 0, 100, 1, 0, 0, 0 );
    pMultiB = Abc_NtkMulti( pNtkB, 0, 100, 1, 0, 0, 0 );
    CnfConvert *ccA = new CnfConvert(pMultiA);
    CnfConvert *ccB = new CnfConvert(pMultiB);
    vector<vector<Lit> > pCnfA = ccA->getCnf();
    vector<vector<Lit> > pCnfB = ccB->getCnf();

    int maxVar = ccA->getMaxVar() + 1;

    IntSolver *pSat = new IntSolver();

    pSat->setnVars(ccA->getMaxVar() + ccB->getMaxVar() + 2);
    map<const Var, char*> mPiName;
    map<const Var, bool> isPiVar;
    int i;
    Abc_Obj_t* pObj;
    //cout << "PI id" << endl;
    Abc_NtkForEachPi(pMultiA, pObj, i) {
        assert(strcmp(Abc_ObjName(pObj), Abc_ObjName( Abc_NtkPi(pMultiB, i))) == 0);
        assert(pObj->Id ==  Abc_NtkPi(pMultiB, i)->Id);
        mPiName[pObj->Id] = Abc_ObjName(pObj);
        isPiVar[pObj->Id] = true;
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
        printLits(lits);
#endif
        pSat->addAClause(lits);
    }

    //cout << "b clauses" << endl;
    for(unsigned j = 0; j < pCnfB.size(); j++) {
        lits.clear();
        for(unsigned k = 0; k < pCnfB[j].size(); k++) {
            if(sign(pCnfB[j][k])) 
                if(isPiVar[var(pCnfB[j][k])])
                    lits.push(~Lit(var(pCnfB[j][k])));
                else         
                    lits.push(~Lit(var(pCnfB[j][k]) + maxVar));
            else   
                if(isPiVar[var(pCnfB[j][k])])
                    lits.push(Lit(var(pCnfB[j][k])));
                else
                    lits.push(Lit(var(pCnfB[j][k]) + maxVar));
        }
#ifdef _DEBUG
        printLits(lits);
#endif
        pSat->addBClause(lits);
    }

    // add Po property
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiA, 0))->Id));
    pSat->addAClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiB, 0))->Id + maxVar));
    pSat->addBClause(lits);
#ifdef _DEBUG
        printLits(lits);
#endif

    assert(!pSat->solve());
    pNtkNew = pSat->getInt(mPiName);
    pNtkNew = resyn(pNtkNew);
    delete pSat;
    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    return pNtkNew;
}
Abc_Ntk_t* singleRel2FuncInt(Abc_Ntk_t* pNtk, char* yName) {

    //cerr << "start sinRel2funcInt" << endl;
    Abc_Ntk_t *pNtkNewA = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Ntk_t *pNtkNewB = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Ntk_t *pNtkFunc;
    Abc_Obj_t *pObj, *pObjNew, *pObj1, *pObj2;
    int i;

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNewA);
//    assert(Abc_NtkFindCi(pNtk, yName) != NULL);
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
 //   char pFileName[100];
 //   sprintf(pFileName, "C5315%s_A.blif", yName);
 //   Io_Write( pNtkNewA, pFileName, IO_FILE_BLIF);
 //   sprintf(pFileName, "C5315%s_B.blif", yName);
 //   Io_Write( pNtkNewB, pFileName, IO_FILE_BLIF); 
    pNtkFunc = getIntFunc(pNtkNewA, pNtkNewB);
  //  cerr << "end add clause" << endl;
 //   pNtkFunc = getIntFunc2(pNtkNewA, pNtkNewB, vNameY);
 //   Abc_Ntk_t *pNtkTemp = getIntFunc6(pNtkNewA, pNtkNewB);
  //  cout << "ori: " << Abc_AigLevel(pNtkTemp) << "\t" << Abc_NtkObjNum(pNtkTemp) << endl;
   // Abc_NtkDelete(pNtkTemp);
    Abc_NtkDelete(pNtkNewA);
    Abc_NtkDelete(pNtkNewB);

    return pNtkFunc;

}

Abc_Ntk_t* rel2FuncInt(Abc_Ntk_t* pNtk, vector<char*> & vNameY, vector<Abc_Ntk_t*> &vFunc) {
    Abc_Ntk_t* pNtkFunc;
    Abc_Ntk_t* pNtkRel = Abc_NtkDup(pNtk);
    pNtkRel = resyn(pNtkRel);
   // Abc_Ntk_t* pNtkRel == Abc_NtkCollapse( pNtk, 50000000, 0, 1, 0 );
    cout << "function info during processing" << endl;
    cout << "-------------------------------" << endl;
    cout << "soltype\tlev#\tnode#\tpi#\trel-lev#\trel-node#" << endl;
    cout << "-------------------------------" << endl;
    for(unsigned i = 0; i < vNameY.size(); i++) {
        //cout << i << "\tRelation Node: " << Abc_NtkObjNum(pNtkRel) << endl;
        pNtkFunc = singleRel2FuncInt(pNtkRel, vNameY[i]);
        //cout << "int node# " << vNameY[i] << ": " << Abc_NtkObjNum(pNtkFunc) << endl;
        //cerr << "begin solve\n" << endl;
        Abc_Ntk_t* pNtkFuncTemp = singleRel2Func(pNtkRel, vNameY[i]);
        //cerr << "end solve\n" << endl; 
        if(pNtkFunc == NULL) {
            pNtkFunc = pNtkFuncTemp;
            cout << "0\t";
        }
        else if(Abc_NtkObjNum(pNtkFuncTemp) > Abc_NtkObjNum(pNtkFunc)) {
            pNtkFunc = resyn(pNtkFunc);
            Abc_NtkDelete(pNtkFuncTemp);
            cout << "1\t";
        }
        else {
            Abc_NtkDelete(pNtkFunc);
            pNtkFunc = pNtkFuncTemp;
            cout << "0\t";
        }

        /*if(pNtkFunc == NULL) {
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
        for(unsigned j = 0; j < vFunc.size(); j++) {
            vFunc[j] = replaceX(vFunc[j], pNtkFunc, vNameY[i]);
        }
        vFunc.push_back(pNtkFunc);
        //if(Abc_NtkObjNum(pNtkFunc) > 50000)
        //    break;
        //     Abc_NtkDelete(pNtkFunc);
    }
    checkRelFunc(pNtkRel, pNtk);
    return pNtkRel;
}
