#include <string> 
#include <vector> 
#include <iostream>
#include <cstdlib>

#include "misc.hpp"
#include "CnfConvert.hpp"
#include "IntSolver.hpp"

//#define _DEBUG

extern "C" int Abc_NtkQuantify( Abc_Ntk_t * pNtk, int fUniv, int iVar, int fVerbose );

using namespace std;

int checkMem() {
    FILE *inf = fopen("/proc/self/status", "r");
    if (!inf) {
        printf( "Cannot get memory usage\n" );
        return 0;
    }
    const size_t bufSize = 128;
    char bufStr[bufSize];
    while( fscanf( inf, "%s", bufStr ) != EOF ) {
        if (strncmp(bufStr, "VmSize", 6) == 0) {
            fscanf( inf, "%s", bufStr );
            int memSizeLong = atol(bufStr);
            fclose(inf);
            return memSizeLong;
        }
    }
    fclose(inf);
    return 0;
}
bool cmps(vector<Lit> a, vector<Lit> b) {
    return a.size() < b.size();
}
    void printLits(const vec<Lit> &lits) {
        if(lits.size() < 1)
            return;
        cout << "(";
        for(int j = 0; j < lits.size()-1; j++) {
            if(sign(lits[j]))
                cout << "-";
            else 
                cout << " ";
            cout << var(lits[j]) + 1 << " + " ;
        }
        if(sign(lits[lits.size()-1]))
            cout << "-";
        else 
            cout << " ";
        cout << var(lits[lits.size()-1]) + 1;
        cout << ")" << endl;
        return;
    }
bool checkRelFunc(Abc_Ntk_t* pNtkA, Abc_Ntk_t* pNtkB) {
    return true;
    cerr << "start check correctness" << endl;
    Abc_Ntk_t* pMultiA = Abc_NtkMulti( pNtkA, 0, 100, 1, 0, 0, 0 );
    Abc_Ntk_t* pMultiB = Abc_NtkMulti( pNtkB, 0, 100, 1, 0, 0, 0 );
    CnfConvert *ccA = new CnfConvert(pMultiA);
    CnfConvert *ccB = new CnfConvert(pMultiB);
    vector<vector<Lit> > pCnfA = ccA->getCnf();
    vector<vector<Lit> > pCnfB = ccB->getCnf();

    int maxVar = ccA->getMaxVar() + 1;
    Solver *pSat = new Solver();

    while(pSat->nVars() < maxVar + ccB->getMaxVar() + 1) 
        pSat->newVar();

    map<const Var, bool> isPiVar;
    map<const Var, Var> mPiVar;



    Abc_Obj_t* pObj; 
    int i;
    Abc_NtkForEachPi(pMultiA, pObj, i) {
        Abc_Obj_t *pPi = Abc_NtkFindCi(pMultiB, Abc_ObjName(pObj));
        assert(pPi != NULL);
        isPiVar[pPi->Id] = true;
        mPiVar[pPi->Id] = pObj->Id;
    }

    vec<Lit> lits;
    for(unsigned j = 0; j < pCnfA.size(); j++) {
        lits.clear();
        for(unsigned k = 0; k < pCnfA[j].size(); k++) {
            if(sign(pCnfA[j][k]))
                lits.push(~Lit(var(pCnfA[j][k])));
            else   
                lits.push(Lit(var(pCnfA[j][k])));
        }
        pSat->addClause(lits);
    }
    lits.clear();
    lits.push(~Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiA, 0))->Id));
    pSat->addClause(lits);

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
        pSat->addClause(lits);
    }
    lits.clear();
    lits.push(Lit(Abc_ObjFanin0(Abc_NtkPo(pMultiB, 0))->Id + maxVar));
    pSat->addClause(lits);

    //  assert(!pSat->solve());
    if(pSat->solve()) {
        assert(pSat->TimeOut);
    }
    delete pSat;
    delete ccA;
    delete ccB;
    Abc_NtkDelete(pMultiA);
    Abc_NtkDelete(pMultiB);
    cerr << "end check correctness" << endl;
    return true;
}
void ntkAddLevelObj(Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkNew, map<const Abc_Obj_t*, bool> &mObj) {
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsDfsOrdered(pNtk) );
    Abc_AigForEachAnd( pNtk, pNode, i ) {
        if(!mObj[pNode])
            continue;

        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
    }
}
void ntkAddLevel(Abc_Ntk_t *pNtk, Abc_Ntk_t *pNtkNew) {
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsDfsOrdered(pNtk) );
    Abc_AigForEachAnd( pNtk, pNode, i ) {
        if(!pNode->fMarkA)
            continue;
        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
    }
}
void ntkBfs_rec(Vec_Ptr_t * vNodes, Vec_Ptr_t* vCurNodes) {

    Vec_Ptr_t* vNextNodes;
    void *pObj;
    int i;

    vNextNodes = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( void*, vCurNodes, pObj, i ) {

        Abc_Obj_t* pNode = (Abc_Obj_t*) pObj;

        assert( !Abc_ObjIsNet(pNode) );
        // if this node is already visited, skip
        if ( Abc_NodeIsTravIdCurrent( pNode ) )
            continue;

        // mark the node as visited
        Abc_NodeSetTravIdCurrent( pNode );
        // skip the CI
        if ( Abc_ObjIsCi(pNode) || (Abc_NtkIsStrash(pNode->pNtk) && Abc_AigNodeIsConst(pNode)) )
            continue;
        assert( Abc_ObjIsNode( pNode ) || Abc_ObjIsBox( pNode ) );

        Vec_PtrPush( vNodes, pNode );
        // visit the transitive fanin of the node
        Abc_Obj_t* pFanin;
        int j;
        Abc_ObjForEachFanin( pNode, pFanin, j )
        {
            Vec_PtrPush( vNextNodes, pFanin);
        }
    }
    Vec_PtrFree(vCurNodes);
    if(Vec_PtrSize(vNextNodes) == 0) {
        Vec_PtrFree(vNextNodes);
        return;
    }
    ntkBfs_rec(vNodes, vNextNodes);
}

Vec_Ptr_t* ntkBfs(Abc_Ntk_t* pNtk) {
    Vec_Ptr_t * vNodes;
    Vec_Ptr_t * vCurNodes;
    Abc_Obj_t * pObj;
    int i;
    // set the traversal ID
    Abc_NtkIncrementTravId( pNtk );
    // start the array of nodes
    vNodes = Vec_PtrAlloc( 100 );
    vCurNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        Abc_NodeSetTravIdCurrent( pObj );
        //Vec_PtrPush(vNodes, Abc_ObjFanin0(pObj));
        Vec_PtrPush(vCurNodes, Abc_ObjFanin0(pObj));
    }
    ntkBfs_rec( vNodes, vCurNodes );

    return vNodes;

}

Abc_Ntk_t* ntkMerge(Abc_Ntk_t* pNtk1, Abc_Ntk_t* pNtk2) {

    Abc_Obj_t *pObj, *pObjNew;
    int i;

    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi(pNtk1, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        pObj->pCopy = pObjNew;
    }
    ntkAddOne(pNtk1, pNtkNew);

    // cout << Abc_NtkObjNum(pNtk) << endl;
    Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi(pNtk2, pObj, i) {
        pObjNew = Abc_NtkFindCo( pNtk1, Abc_ObjName(pObj));
        //  assert(pObjNew != NULL);
        if(pObjNew == NULL) {
            pObjNew = Abc_NtkFindCi( pNtkNew, Abc_ObjName(pObj));
            if(pObjNew == NULL) {
                pObjNew = Abc_NtkCreatePi(pNtkNew);
                Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
            }
            pObj->pCopy = pObjNew;
        }
        else
            pObj->pCopy = Abc_ObjChild0Copy(pObjNew);
    }
    ntkAddOne(pNtk2, pNtkNew);

    Abc_NtkForEachPo(pNtk2, pObj, i) {
        pObjNew = Abc_NtkCreatePo( pNtkNew );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }
    return pNtkNew;
}

Abc_Ntk_t* partialClp(Abc_Ntk_t* pNtk, int nLevel, int levelSize) {
    Abc_Ntk_t* pNtkNew;
    Abc_Obj_t* pObj, *pObjNew;
    vector<map<const Abc_Obj_t*, bool> > vmLevObj;
    vector<vector<Abc_Obj_t*> > vvObj;
    vector<vector<Abc_Obj_t*> > vInterPi;
    vector<vector<Abc_Obj_t*> > vInterPo;

    int i; 
    int ntkLevel = Abc_NtkLevel(pNtk);

    // set parameter
    if(nLevel > 0) {
        levelSize = ntkLevel / nLevel;
    }
    else if(levelSize > 0) {
        nLevel = ntkLevel  / levelSize;
    }
    else {
        levelSize = 100;
        nLevel = ntkLevel / levelSize;
    }

    if(ntkLevel < nLevel) {
        nLevel /= 10;
        if(nLevel == 0)
            return Abc_NtkCollapse( pNtk, 50000000, 0, 1, 0 , 0, 0);
    }

    if(ntkLevel <= levelSize)
        return Abc_NtkCollapse( pNtk, 50000000, 0, 1, 0 , 0, 0);
    //   if(levelSize < 10) 
    //     return Abc_NtkCollapse( pNtk, 50000000, 0, 1, 0 );

    for(int i = 0; i < nLevel; i++) {
        map<const Abc_Obj_t*, bool> mObj;
        vector<Abc_Obj_t*> vObj;
        vmLevObj.push_back(mObj);
        vvObj.push_back(vObj);
        if(i < nLevel - 1) {
            vInterPi.push_back(vObj);
            vInterPo.push_back(vObj);
        }
    }

    // set level obj
    /*   cerr << "nLevel: " << nLevel << endl;
         cerr << "levelSize: " << levelSize << endl;
         cerr << "ntk level: " << Abc_NtkLevel(pNtk) << endl;
         */
    Abc_NtkForEachObj(pNtk, pObj, i) {
        pObj->pCopy = NULL;
        int lev = Abc_ObjLevel(pObj) / levelSize;
        if(lev >= nLevel) 
            lev = nLevel - 1;
        //    cerr << "obj lev: " << Abc_ObjLevel(pObj) << endl;
        //   cerr << "lev: " << lev << endl;
        vmLevObj[lev][pObj] = true;
        vvObj[lev].push_back(pObj);
        Abc_Obj_t *pFanin;
        int j;
        Abc_ObjForEachFanin(pObj, pFanin, j) {
            if(Abc_ObjLevel(pFanin) < (lev) * levelSize) {
                vInterPi[lev-1].push_back(pFanin);
            }
        }

        if(lev < nLevel - 1) {
            Abc_Obj_t *pFanout;
            Abc_ObjForEachFanout(pObj, pFanout, j) {
                if(Abc_ObjLevel(pFanout) >= (lev + 1) * levelSize) {
                    vInterPo[lev].push_back(pObj);
                    break;
                }
            }
        }
    }
    /*
       cerr << "Pi: " << i << " Obj: ";
       Abc_NtkForEachPi(pNtk, pObj, i) {
       cerr << Abc_ObjName(pObj) << " ";
       }
       cerr << endl;

       for(unsigned i = 0; i < vvObj.size(); i++) {
       cerr << "level " << i << " Obj: ";
       for(unsigned j = 0; j < vvObj[i].size(); j++) {
       cerr << Abc_ObjName(vvObj[i][j]) << " ";
       }
       cerr << endl;

       if(i < vvObj.size() - 1) {
       cerr << "inter Po: " << i << " Obj: ";
       for(unsigned j = 0; j < vInterPo[i].size(); j++) {
       cerr << Abc_ObjName(vInterPo[i][j]) << " ";
       }
       cerr << endl;
       cerr << "inter Pi: " << i+1 << " Obj: ";
       for(unsigned j = 0; j < vInterPi[i].size(); j++) {
       cerr << Abc_ObjName(vInterPi[i][j]) << " ";
       }
       cerr << endl;
       }
       }
       */  
    // create the last level ntk
    pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);

    assert(!vInterPi[nLevel - 2].empty());
    for(unsigned j = 0; j < vInterPi[nLevel - 2].size(); j++) {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        char* name = (char*) malloc(sizeof(char) * (strlen(Abc_ObjName(vInterPi[nLevel - 2][j])) + 5));
        sprintf(name, "%s_int", Abc_ObjName(vInterPi[nLevel - 2][j]));
        Abc_ObjAssignName( pObjNew, name, NULL );
        vInterPi[nLevel - 2][j]->pCopy = pObjNew;
    }
    ntkAddLevelObj(pNtk, pNtkNew, vmLevObj[nLevel - 1]);

    Abc_NtkForEachPo(pNtk, pObj, i) {
        pObjNew = Abc_NtkCreatePo(pNtkNew);
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        Abc_ObjAddFanin(pObjNew, Abc_ObjChild0Copy(pObj));
    }
    Abc_NtkCheck(pNtkNew);

    Abc_Ntk_t *pNtkInter;
    Abc_Ntk_t* pNtkTemp;
    for(int i = nLevel - 2; i > 0; i--) {
        pNtkInter = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkInter);
        for(unsigned j = 0; j < vInterPi[i - 1].size(); j++) {
            pObjNew = Abc_NtkFindCi(pNtkInter, Abc_ObjName(vInterPi[i-1][j]));
            if(pObjNew == NULL) {
                pObjNew = Abc_NtkCreatePi(pNtkInter);
                // add name
                char* name = (char*) malloc(sizeof(char) * (strlen(Abc_ObjName(vInterPi[i - 1][j])) + 5));
                sprintf(name, "%s_int", Abc_ObjName(vInterPi[i - 1][j]));
                Abc_ObjAssignName( pObjNew, name, NULL );
            }
            vInterPi[i - 1][j]->pCopy = pObjNew;
        }
        ntkAddLevelObj(pNtk, pNtkInter, vmLevObj[i]);
        for(unsigned j = 0; j < vInterPo[i].size(); j++) {
            pObjNew = Abc_NtkCreatePo( pNtkInter);
            // add name
            char* name = (char*) malloc(sizeof(char) * (strlen(Abc_ObjName(vInterPo[i][j])) + 5));
            sprintf(name, "%s_int", Abc_ObjName(vInterPo[i][j]));
            Abc_ObjAssignName( pObjNew, name, NULL );
            Abc_ObjAddFanin(pObjNew, vInterPo[i][j]->pCopy);
        }
        Abc_NtkCheck(pNtkInter);
        pNtkInter = Abc_NtkCollapse( pNtkTemp = pNtkInter, 50000000, 0, 1, 0 , 0, 0);
        Abc_NtkDelete(pNtkTemp);
        pNtkInter = Abc_NtkStrash( pNtkTemp = pNtkInter, 0, 1, 0 );
        Abc_NtkDelete(pNtkTemp);
        pNtkNew = ntkMerge(pNtkInter, pNtkTemp = pNtkNew);
        Abc_NtkCheck(pNtkNew);
        Abc_NtkDelete(pNtkTemp);
        Abc_NtkDelete(pNtkInter);
    }
    pNtkInter = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkInter);
    Abc_NtkForEachPi(pNtk, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkInter);
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj) , NULL );
        pObj->pCopy = pObjNew;
    }
    ntkAddLevelObj(pNtk, pNtkInter, vmLevObj[0]);
    for(unsigned j = 0; j < vInterPo[0].size(); j++) {
        pObjNew = Abc_NtkCreatePo(pNtkInter);
        // add name
        char* name = (char*) malloc(sizeof(char) * (strlen(Abc_ObjName(vInterPo[0][j])) + 5));
        sprintf(name, "%s_int", Abc_ObjName(vInterPo[0][j]));
        Abc_ObjAssignName( pObjNew, name, NULL );
        Abc_ObjAddFanin(pObjNew, vInterPo[0][j]->pCopy);
    }
    Abc_NtkCheck(pNtkInter);
    pNtkInter = Abc_NtkCollapse( pNtkTemp = pNtkInter, 50000000, 0, 1, 0 , 0 , 0);
    Abc_NtkDelete(pNtkTemp);
    pNtkInter = Abc_NtkStrash( pNtkTemp = pNtkInter, 0, 1, 0 );
    Abc_NtkDelete(pNtkTemp);
    pNtkNew = ntkMerge(pNtkInter, pNtkTemp = pNtkNew);

    //pNtkNew = Abc_NtkStrash( pNtkTemp = pNtkNew, 0, 1, 0 );
    //Abc_NtkDelete(pNtkTemp);
    /*  cerr << "Pi: " << i << " Obj: ";
        Abc_NtkForEachPi(pNtkNew, pObj, i) {
        cerr << Abc_ObjName(pObj) << " ";
        }
        cerr << endl;*/
    Abc_NtkCheck(pNtkNew);
    Abc_NtkDelete(pNtkTemp);
    Abc_NtkCheck(pNtkInter);



    /*

       Abc_NtkForEachPi(pNtk, pObj, i) {
       pObjNew = Abc_NtkCreatePi( pNtkInter);
       Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
       pObj->pCopy = pObjNew;
       }


*/
    return pNtkNew;
}
Abc_Ntk_t* resyn(Abc_Ntk_t* pNtk) {
    Abc_Ntk_t* pNtkNew;
    //cerr << "begin resyn" << endl;
    //assert(Abc_NtkIsStrash(pNtk));
    // pNtkNew = Abc_NtkStrash( pNtk, 0, 1, 0 );
    // Abc_NtkDelete(pNtk);

    //pNtk = pNtkNew;
    pNtkNew = Abc_NtkMulti( pNtk, 0, 100, 1, 0, 0, 0 );
    Abc_NtkDelete(pNtk);

    pNtk = pNtkNew;
    pNtkNew = Abc_NtkStrash( pNtk, 0, 1, 0 );
    Abc_NtkDelete(pNtk);

    Abc_NtkSynthesize(&pNtkNew, 1);
    //  Abc_NtkRewrite( pNtkNew, 0, 1, 0, 0, 0 );

    //cerr << "#obj: " << Abc_NtkObjNum(pNtkNew) << endl;
    //cerr << "start IvyFraig" << endl;
    // pNtk = pNtkNew;
    //  pNtkNew = Abc_NtkIvyFraig(pNtk, 100, 1, 0, 0, 0);
    //  Abc_NtkDelete(pNtk);

    /*    pNtk = pNtkNew;
          pNtkNew = Abc_NtkCollapse( pNtk, 50000000, 0, 1, 0 );
          Abc_NtkDelete(pNtk);
          */
    //  pNtk = pNtkNew;
    /*
       vector<Abc_Obj_t*> vPi;
       int j;
       Abc_Obj_t* pObj;
       Abc_NtkForEachPi(pNtk, pObj, j) {
       if(Abc_ObjFanoutNum(pObj) == 0)
       vPi.push_back(pObj);
       }
       for(unsigned i = 0; i < vPi.size(); i++) {
       Abc_NtkDeleteObj(vPi[i]);
       }
       */

    /*
       pNtkNew = Abc_NtkStrash( pNtk, 0, 1, 0 );
       Abc_NtkDelete(pNtk);
       Abc_NtkSynthesize(&pNtkNew, 1);
       */
    Abc_AigCleanup((Abc_Aig_t*)pNtkNew->pManFunc);
    //    cerr << "end resyn" << endl;


    return pNtkNew;
}
void ntkAddOne( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkMiter )
{
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsDfsOrdered(pNtk) );
    Abc_AigForEachAnd( pNtk, pNode, i ) 
        pNode->pCopy = Abc_AigAnd( (Abc_Aig_t*)pNtkMiter->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
}

Abc_Ntk_t* getEquIntNtkCtr(Abc_Ntk_t *pNtk, int choice) {
    Abc_Ntk_t* pNtkNew;

    Abc_Ntk_t* pMulti = Abc_NtkMulti( pNtk, 0, 100, 1, 0, 0, 0 );
    CnfConvert *cc = new CnfConvert(pMulti);
    IntSolver *pSat = new IntSolver();
    int maxVar = cc->getMaxVar() + 1;

    vector<vector<Lit> > pCnf = cc->getCnf();
    vector<vector<Lit> > pCnfTotal;

    Abc_Obj_t* pObj;
    int i;
    map<const Var, char*> mPiName;
    map<const Var, bool> isPiVar;

    // assign Pi name & set Pi var
    Abc_NtkForEachPi(pMulti, pObj, i) {
        mPiName[pObj->Id] = Abc_ObjName(pObj);
        isPiVar[pObj->Id] = true;
    }

    Var ctrVar = 2 * maxVar;
    pSat->setnVars(maxVar * 2);
    vec<Lit> ctrLits;
    vector<Lit> vLits;
    for(unsigned j = 0; j < pCnf.size(); j++) {
        vLits.clear();
        vLits.push_back(Lit(1));
        for(unsigned k = 0; k < pCnf[j].size(); k++) {
            if(sign(pCnf[j][k]))
                vLits.push_back(~Lit(var(pCnf[j][k])));
            else   
                vLits.push_back(Lit(var(pCnf[j][k])));
        }   
        vLits.push_back(Lit(ctrVar));
        pSat->newVar();
        ctrLits.push(~Lit(ctrVar++));
        pCnfTotal.push_back(vLits);
    }
    vLits.clear();
    vLits.push_back(Lit(1));
    vLits.push_back(Lit(Abc_ObjFanin0(Abc_NtkPo(pMulti, 0))->Id));
    vLits.push_back(Lit(ctrVar));
    pSat->newVar();
    ctrLits.push(~Lit(ctrVar++));
    pCnfTotal.push_back(vLits);

    for(unsigned j = 0; j < pCnf.size(); j++) {
        vLits.clear();
        vLits.push_back(~Lit(1));
        for(unsigned k = 0; k < pCnf[j].size(); k++) {
            if(sign(pCnf[j][k]))
                if(isPiVar[var(pCnf[j][k])])
                    vLits.push_back(~Lit(var(pCnf[j][k])));
                else 
                    vLits.push_back(~Lit(var(pCnf[j][k]) + maxVar));
            else   
                if(isPiVar[var(pCnf[j][k])])
                    vLits.push_back(Lit(var(pCnf[j][k])));
                else
                    vLits.push_back(Lit(var(pCnf[j][k]) + maxVar));
        }   
        vLits.push_back(Lit(ctrVar));
        pSat->newVar();
        ctrLits.push(~Lit(ctrVar++));
        pCnfTotal.push_back(vLits);
    }
    vLits.clear();
    vLits.push_back(~Lit(1));
    vLits.push_back(~Lit(Abc_ObjFanin0(Abc_NtkPo(pMulti, 0))->Id + maxVar));
    vLits.push_back(Lit(ctrVar));
    pSat->newVar();
    ctrLits.push(~Lit(ctrVar++));
    pCnfTotal.push_back(vLits);

    //if(choice == 1)
    //  sort(pCnfTotal.begin(), pCnfTotal.end(), cmps);
    //else if(choice == 2)
    //random_shuffle(pCnfTotal.begin(), pCnfTotal.end());

    vec<Lit> lits;
    for(unsigned j = 0; j < pCnfTotal.size(); j++) {
        lits.clear();
        for(unsigned k = 1; k < pCnfTotal[j].size(); k++) {
            if(sign(pCnfTotal[j][k]))
                lits.push(~Lit(var(pCnfTotal[j][k])));
            else   
                lits.push(Lit(var(pCnfTotal[j][k])));
        }
        assert(var(pCnfTotal[j][0]) == 1);
        if(sign(pCnfTotal[j][0]))
            pSat->addBClause(lits);
        else 
            pSat->addAClause(lits);
    }
    //assert(!pSat->solve(ctrLits));
    if(pSat->solve()) {
        pNtkNew = Abc_NtkDup(pNtk);
    }
    else pNtkNew = pSat->getInt(mPiName);

    pNtkNew = resyn(pNtkNew);
    delete pSat;
    delete cc;
    Abc_NtkDelete(pMulti);
    return pNtkNew;
}
Abc_Ntk_t* EQInt(Abc_Ntk_t* pNtk, char* yName) {
    /*   Abc_Ntk_t *pNtkNew = Abc_NtkDup(pNtk);
         int i;
         Abc_Obj_t* pObj;
         Abc_NtkForEachCi(pNtkNew, pObj, i) {
         if(strncmp(Abc_ObjName(pObj), yName, strlen(yName)) == 0) {
         cout << "abc quantify" << endl;
         Abc_NtkQuantify( pNtkNew, 0, i, 0);
         cout << "after q: " << Abc_NtkObjNum(pNtkNew) << endl;
         break;
         }
         }
         pNtkNew = resyn(pNtkNew);*/

    Abc_Ntk_t *pNtkNew = EQ(pNtk, yName);
    pNtk = pNtkNew; 
    pNtkNew = getEquIntNtkCtr(pNtk, 0);
    Abc_NtkDelete(pNtk);
    return pNtkNew;

}

    Abc_Ntk_t* EQ(Abc_Ntk_t* pNtk, char* yName) {
        if(Abc_NtkFindCi(pNtk, yName) == NULL) 
            return pNtk;

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
        Abc_Obj_t* pObj1 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

        Abc_NtkFindCi(pNtk, yName)->pCopy = Abc_ObjNot(Abc_AigConst1(pNtkNew));
        ntkAddOne(pNtk, pNtkNew);
        Abc_Obj_t* pObj2 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));
        pObjNew = Abc_AigOr((Abc_Aig_t*) pNtkNew->pManFunc, pObj1, pObj2);
        Abc_Obj_t* pObjOut = Abc_NtkCreatePo(pNtkNew);
        char name[10];
        sprintf(name, "%s", "test");
        Abc_ObjAssignName(pObjOut, name, NULL);
        Abc_ObjAddFanin( pObjOut, pObjNew);
        pNtkNew = resyn(pNtkNew);
        pNtkNew = resyn(pNtkNew);

        return pNtkNew;
    }


Abc_Ntk_t* replaceX(Abc_Ntk_t* pNtk, Abc_Ntk_t* pNtkFunc, char* yName) {
    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;

    //cerr << "start replace" << endl;
    assert(Abc_NtkFindCi(pNtkFunc, yName) == NULL);
    // assert(Abc_NtkPiNum(pNtk) >= Abc_NtkPiNum(pNtkFunc) + 1);
    // assert(Abc_NtkFindCi(pNtk, yName) != NULL);
    if(Abc_NtkFindCi(pNtk, yName) == NULL)
        return pNtk;

    Abc_AigConst1(pNtkFunc)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtkFunc, pObj, i )
    {
        assert(strcmp(Abc_ObjName(pObj), yName) != 0);
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );

        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }

    ntkAddOne(pNtkFunc, pNtkNew);
    Abc_Obj_t* pObj1 = Abc_ObjChild0Copy(Abc_NtkPo(pNtkFunc, 0));

    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        if(strcmp(Abc_ObjName(pObj), yName) == 0) {
            pObjNew = pObj1;
        }
        else {
            pObjNew = Abc_NtkFindCi(pNtkNew, Abc_ObjName(pObj));
            if(pObjNew == NULL) {
                pObjNew = Abc_NtkCreatePi( pNtkNew );
                // add name
                Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
            }
        }
        assert(pObjNew != NULL);
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }
    ntkAddOne(pNtk, pNtkNew);


    Abc_Obj_t *pObj2 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk, 0));

    Abc_Obj_t* pObjOut = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pObjOut, name, NULL);
    Abc_ObjAddFanin( pObjOut, pObj2);


    Abc_NtkDelete(pNtk);

    pNtkNew = resyn(pNtkNew);
    pNtkNew = resyn(pNtkNew);

    return pNtkNew;

}

Abc_Obj_t *globalBdd2Ntk_rec(Abc_Ntk_t *pNtkNew, DdManager *dd, DdNode *f, map<int, int>&mVarPi, map<DdNode*, Abc_Obj_t*>& mBddNode)
{
    //    cerr << "bdd rec" << endl;
    DdNode *one = DD_ONE(dd);
    DdNode *F = Cudd_Regular(f);
    if(f == one)
        return Abc_AigConst1(pNtkNew);

    if(F == one)
        return Abc_ObjNot(Abc_AigConst1(pNtkNew));
    if(mBddNode[f] != NULL)
        return mBddNode[f];


    assert(!cuddIsConstant(f));
    Abc_Obj_t* pObjT = globalBdd2Ntk_rec(pNtkNew, dd, cuddT(F), mVarPi, mBddNode);
    Abc_Obj_t* pObjE = globalBdd2Ntk_rec(pNtkNew, dd, cuddE(F), mVarPi, mBddNode);

    /*  Abc_Obj_t* pObj;
        int i;
        Abc_NtkForEachCi( pNtkNew, pObj, i ) {

    //    cout << "pi: " << i << "   bdd var: " << dd->vars[i]->index << endl;
    cout << "pi: " << i << "   bdd perm var: " << dd->perm[dd->vars[i]->index] << endl;
    }
    */
    //  cout << "bdd lev perm : " << dd->perm[Cudd_Regular(bFunc)->index] << endl;
    // cout << "index: " << bFunc->index << "   bdd var: " << dd->invperm[bFunc->index] << endl;
    //  cerr << mVarPi[dd->perm[Cudd_Regular(bFunc)->index]] << endl;
    //    cerr << "# PI:" << Abc_NtkPiNum(pNtkNew) << endl;
    assert(mVarPi[dd->perm[F->index]] < Abc_NtkPiNum(pNtkNew));

    Abc_Obj_t* pPi = Abc_NtkCi(pNtkNew, mVarPi[dd->perm[F->index]]);
    // assert(pPi->Id == dd->invperm[dd->perm[bFunc->index]]);
    pObjT = Abc_AigAnd( (Abc_Aig_t*)pNtkNew->pManFunc, pObjT,  pPi);
    pObjE = Abc_AigAnd( (Abc_Aig_t*)pNtkNew->pManFunc, pObjE,  Abc_ObjNot(pPi));

    Abc_Obj_t* pObjNew = Abc_AigOr( (Abc_Aig_t*)pNtkNew->pManFunc, pObjE, pObjT);

    if(Cudd_IsComplement(f))
        pObjNew = Abc_ObjNot(pObjNew);
    mBddNode[f] = pObjNew;
    return pObjNew;
}
Abc_Ntk_t * globalBdd2Ntk( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pObjNew;
    DdManager * dd = (DdManager*)Abc_NtkGlobalBddMan( pNtk );
    int i;
    // start the new network
    pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    map<int, int> mVarPi;
    Abc_NtkForEachPi(pNtk, pObj, i) {
        pObjNew = Abc_NtkCreatePi(pNtkNew);
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        assert(pObj->Id == pObjNew->Id);
        // map pi var to lev
        mVarPi[dd->perm[dd->vars[i]->index]] = i;
        //        cout << "create PI" << endl;
    }
    map<DdNode*, Abc_Obj_t*> mBddNode;
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pObjNew = Abc_NtkCreatePo( pNtkNew );
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );

        DdNode* bFunc = (DdNode*)Abc_ObjGlobalBdd(pObj);
        assert(bFunc != NULL);
        Abc_ObjAddFanin( pObjNew, globalBdd2Ntk_rec(pNtkNew, dd, bFunc, mVarPi, mBddNode));
    }
    //   pNtkNew = resyn(pNtkNew);
    return pNtkNew;
}
Abc_Ntk_t* createRelNtkAnd(Abc_Ntk_t* pNtk) {

    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;


    if(Abc_NtkPoNum(pNtk) < 2)
        return Abc_NtkDup(pNtk);

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }
    ntkAddOne(pNtk, pNtkNew);
    vector<Abc_Obj_t*> vObj;
    Abc_NtkForEachPo( pNtk, pObj, i ) {
        //  pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        //  char yName[100];
        //  sprintf(yName, "_yPo_%s", Abc_ObjName(pObj));
        //  Abc_ObjAssignName( pObjNew, yName, NULL );
        pObjNew = Abc_ObjChild0Copy(pObj);
        vObj.push_back(pObjNew);
    }

    pObjNew = vObj[0];
    for(unsigned j = 1; j < vObj.size(); j++) {
        pObjNew = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, vObj[j], pObjNew);
    }
    Abc_Obj_t* pPo = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pPo, name, NULL);
    Abc_ObjAddFanin(pPo, pObjNew);
    pNtkNew = resyn(pNtkNew);
    Abc_NtkCheck(pNtkNew);
    return pNtkNew;
}

Abc_Ntk_t* createRelNtkOr(Abc_Ntk_t* pNtk) {

    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;


    if(Abc_NtkPoNum(pNtk) < 2)
        return Abc_NtkDup(pNtk);

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }
    ntkAddOne(pNtk, pNtkNew);
    vector<Abc_Obj_t*> vObj;
    Abc_NtkForEachPo( pNtk, pObj, i ) {
        //       pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        //       char yName[100];
        //       sprintf(yName, "_yPo_%s", Abc_ObjName(pObj));
        //       Abc_ObjAssignName( pObjNew, yName, NULL );
        pObjNew = Abc_ObjChild0Copy(pObj);
        vObj.push_back(pObjNew);
    }

    pObjNew = vObj[0];
    for(unsigned j = 1; j < vObj.size(); j++) {
        pObjNew = Abc_AigOr((Abc_Aig_t*)pNtkNew->pManFunc, vObj[j], pObjNew);
    }
    Abc_Obj_t* pPo = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pPo, name, NULL);
    Abc_ObjAddFanin(pPo, pObjNew);
    pNtkNew = resyn(pNtkNew);
    Abc_NtkCheck(pNtkNew);
    return pNtkNew;
}
Abc_Ntk_t* createRelNtk(Abc_Ntk_t* pNtk) {

    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;


    if(Abc_NtkPoNum(pNtk) < 2)
        return Abc_NtkDup(pNtk);

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }
    ntkAddOne(pNtk, pNtkNew);
    vector<Abc_Obj_t*> vObj;
    Abc_NtkForEachPo( pNtk, pObj, i ) {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        char yName[100];
        sprintf(yName, "_yPo_%s", Abc_ObjName(pObj));
        Abc_ObjAssignName( pObjNew, yName, NULL );
        pObjNew = Abc_ObjNot(Abc_AigXor((Abc_Aig_t*)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), pObjNew));
        vObj.push_back(pObjNew);
    }

    pObjNew = vObj[0];
    for(unsigned j = 1; j < vObj.size(); j++) {
        pObjNew = Abc_AigAnd((Abc_Aig_t*)pNtkNew->pManFunc, vObj[j], pObjNew);
    }
    Abc_Obj_t* pPo = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pPo, name, NULL);
    Abc_ObjAddFanin(pPo, pObjNew);
    pNtkNew = resyn(pNtkNew);
    return pNtkNew;
}
Abc_Ntk_t * ntkTransRel( Abc_Ntk_t * pNtk, int fInputs, int fVerbose )
{
    char Buffer[1000];
    Vec_Ptr_t * vPairs;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pMiter;
    int i, nLatches;
    int fSynthesis = 1;

    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkLatchNum(pNtk) );
    nLatches = Abc_NtkLatchNum(pNtk);
    // start the network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    // duplicate the name and the spec
    sprintf( Buffer, "%s_TR", pNtk->pName );
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
    //    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    Abc_NtkCleanCopy( pNtk );
    // create current state variables
    Abc_NtkForEachLatchOutput( pNtk, pObj, i )
    {
        pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    }
    // create next state variables
    Abc_NtkForEachLatchInput( pNtk, pObj, i ) {
        // Abc_ObjAssignName( Abc_NtkCreatePi(pNtkNew), Abc_ObjName(pObj), NULL );
        char yName[100];
        sprintf(yName, "_yPo_%s", Abc_ObjName(pObj));
        Abc_ObjAssignName(Abc_NtkCreatePi(pNtkNew), yName, NULL);
    }

    // create PI variables
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 1 );
    // create the PO
    Abc_NtkCreatePo( pNtkNew );
    // restrash the nodes (assuming a topological order of the old network)
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachNode( pNtk, pObj, i )
        pObj->pCopy = Abc_AigAnd((Abc_Aig_t*) pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    // create the function of the primary output
    assert( Abc_NtkBoxNum(pNtk) == Abc_NtkLatchNum(pNtk) );
    vPairs = Vec_PtrAlloc( 2*nLatches );
    Abc_NtkForEachLatchInput( pNtk, pObj, i )
    {
        Vec_PtrPush( vPairs, Abc_ObjChild0Copy(pObj) );
        Vec_PtrPush( vPairs, Abc_NtkPi(pNtkNew, i+nLatches) );
    }
    pMiter = Abc_AigMiter((Abc_Aig_t*) pNtkNew->pManFunc, vPairs, 0);
    Vec_PtrFree( vPairs );
    // add the primary output
    Abc_ObjAddFanin( Abc_NtkPo(pNtkNew,0), Abc_ObjNot(pMiter) );
    Abc_ObjAssignName( Abc_NtkPo(pNtkNew,0), "rel", NULL );

    // quantify inputs
    if ( fInputs )
    {
        assert( Abc_NtkPiNum(pNtkNew) == Abc_NtkPiNum(pNtk) + 2*nLatches );
        for ( i = Abc_NtkPiNum(pNtkNew) - 1; i >= 2*nLatches; i-- )
            //        for ( i = 2*nLatches; i < Abc_NtkPiNum(pNtkNew); i++ )
        {
            Abc_NtkQuantify( pNtkNew, 0, i, fVerbose );
            //            if ( fSynthesis && (i % 3 == 2) )
            if ( fSynthesis  )
            {
                Abc_NtkCleanData( pNtkNew );
                Abc_AigCleanup( (Abc_Aig_t*)pNtkNew->pManFunc );
                Abc_NtkSynthesize( &pNtkNew, 1 );
            }
            //            printf( "Var = %3d. Nodes = %6d. ", Abc_NtkPiNum(pNtkNew) - 1 - i, Abc_NtkNodeNum(pNtkNew) );
            //            printf( "Var = %3d. Nodes = %6d. ", i - 2*nLatches, Abc_NtkNodeNum(pNtkNew) );
        }
        //        printf( "\n" );
        Abc_NtkCleanData( pNtkNew );
        Abc_AigCleanup( (Abc_Aig_t*)pNtkNew->pManFunc );
        for ( i = Abc_NtkPiNum(pNtkNew) - 1; i >= 2*nLatches; i-- )
        {
            pObj = Abc_NtkPi( pNtkNew, i );
            assert( Abc_ObjFanoutNum(pObj) == 0 );
            Abc_NtkDeleteObj( pObj );
        }
    }

    // check consistency of the network
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkTransRel: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}
void binRandom(int *vec, int size) {
    for(int i = 0; i < size; i++) {
        vec[i] = rand()%2;
    }
}
Abc_Ntk_t* createCubeNtk(Abc_Ntk_t* pNtk, int nCubes) {
    
    if(nCubes == 0)
        return NULL;

    Abc_Ntk_t* pNtkNew = Abc_NtkAlloc(ABC_NTK_LOGIC, ABC_FUNC_SOP, 1);

    Abc_Obj_t *pObj;
    Abc_Obj_t *pFanin;

    int i;

    Abc_NtkForEachPi(pNtk, pObj, i) {
        Abc_ObjAssignName(Abc_NtkCreatePi(pNtkNew), Abc_ObjName(pObj), NULL);
    }
    Abc_Obj_t *pSum = Abc_NtkCreateNode(pNtkNew);

    int* pComp = (int*) malloc(sizeof(int) * Abc_NtkPiNum(pNtkNew));
    for(int j = 0; j < nCubes; j++) {
        Abc_Obj_t *pCube = Abc_NtkCreateNode(pNtkNew);
        Abc_NtkForEachPi(pNtkNew, pFanin, i) {
            Abc_ObjAddFanin(pCube, pFanin);
        }
        binRandom(pComp, Abc_NtkPiNum(pNtkNew));
        pCube->pData = Abc_SopCreateAnd((Mem_Flex_t*)pNtkNew->pManFunc, Abc_NtkPiNum(pNtkNew), pComp);
        Abc_ObjAddFanin(pSum, pCube);
    }
    pSum->pData = Abc_SopCreateOr((Mem_Flex_t*)pNtkNew->pManFunc, nCubes, NULL);
    Abc_Obj_t* pPo = Abc_NtkCreatePo(pNtkNew);
    Abc_ObjAddFanin(pPo, pSum);

    Abc_Ntk_t* pNtkTemp = pNtkNew; 
    pNtkNew = Abc_NtkStrash(pNtkNew, 0, 0, 0);
    Abc_NtkDelete(pNtkTemp);
    pNtkNew = resyn(pNtkNew);
    return pNtkNew;
}

Abc_Ntk_t * createNtkUni(Abc_Ntk_t* pNtk1, Abc_Ntk_t* pNtk2) {

    if(pNtk1 == NULL) 
        return Abc_NtkDup(pNtk2);
    else if(pNtk2 == NULL)
        return Abc_NtkDup(pNtk1);

    Abc_Ntk_t *pNtkNew = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    Abc_Obj_t *pObj, *pObjNew;
    int i;

    Abc_AigConst1(pNtk1)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachPi( pNtk1, pObj, i )
    {
        pObjNew = Abc_NtkCreatePi( pNtkNew );
        // add name
        Abc_ObjAssignName( pObjNew, Abc_ObjName(pObj), NULL );
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }


    //pObjNew = Abc_NtkCreatePo(pNtkNew);

    ntkAddOne(pNtk1, pNtkNew);
    Abc_Obj_t* pObj1 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk1, 0));

    Abc_NtkForEachPi( pNtk2, pObj, i )
    {
        pObjNew = Abc_NtkFindCi(pNtkNew, Abc_ObjName(pObj));
        assert(pObjNew != NULL);
        // remember this PI in the old PIs
        pObj->pCopy = pObjNew;
    }

    ntkAddOne(pNtk2, pNtkNew);
    Abc_Obj_t* pObj2 = Abc_ObjChild0Copy(Abc_NtkPo(pNtk2, 0));

    pObjNew = Abc_AigOr((Abc_Aig_t*) pNtkNew->pManFunc, pObj1, pObj2);
    Abc_Obj_t* pObjOut = Abc_NtkCreatePo(pNtkNew);
    char name[10];
    sprintf(name, "%s", "test");
    Abc_ObjAssignName(pObjOut, name, NULL);
    Abc_ObjAddFanin( pObjOut, pObjNew);
    pNtkNew = resyn(pNtkNew);
    pNtkNew = resyn(pNtkNew);

    return pNtkNew;

}
//Abc_Ntk_t * createFlexNtk(Abc_Ntk_t* pNtk, double rate) {
//}
/*
void Abc_AigReplace_int( Abc_Aig_t * pMan, Abc_Obj_t * pOld, Abc_Obj_t * pNew, int fUpdateLevel )
{
    Abc_Obj_t * pFanin1, * pFanin2, * pFanout, * pFanoutNew, * pFanoutFanout;
    int k, v, iFanin; 
    // make sure the old node is regular and has fanouts
    // (the new node can be complemented and can have fanouts)
    assert( !Abc_ObjIsComplement(pOld) );
    assert( Abc_ObjFanoutNum(pOld) > 0 );
    // look at the fanouts of old node
    Abc_NodeCollectFanouts( pOld, pMan->vNodes );
    Vec_PtrForEachEntry( pMan->vNodes, pFanout, k )
    {
        if ( Abc_ObjIsCo(pFanout) )
        {
            Abc_ObjPatchFanin( pFanout, pOld, pNew );
            continue;
        }
        // find the old node as a fanin of this fanout
        iFanin = Vec_IntFind( &pFanout->vFanins, pOld->Id );
        assert( iFanin == 0 || iFanin == 1 );
        // get the new fanin
        pFanin1 = Abc_ObjNotCond( pNew, Abc_ObjFaninC(pFanout, iFanin) );
        assert( Abc_ObjRegular(pFanin1) != pFanout );
        // get another fanin
        pFanin2 = Abc_ObjChild( pFanout, iFanin ^ 1 );
        assert( Abc_ObjRegular(pFanin2) != pFanout );
        // check if the node with these fanins exists
        if ( (pFanoutNew = Abc_AigAndLookup( pMan, pFanin1, pFanin2 )) )
        { // such node exists (it may be a constant)
            // schedule replacement of the old fanout by the new fanout
            Vec_PtrPush( pMan->vStackReplaceOld, pFanout );
            Vec_PtrPush( pMan->vStackReplaceNew, pFanoutNew );
            continue;
        }
        // such node does not exist - modify the old fanout node 
        // (this way the change will not propagate all the way to the COs)
        assert( Abc_ObjRegular(pFanin1) != Abc_ObjRegular(pFanin2) );             

        // if the node is in the level structure, remove it
        if ( pFanout->fMarkA )
            Abc_AigRemoveFromLevelStructure( pMan->vLevels, pFanout );
        // if the node is in the level structure, remove it
        if ( pFanout->fMarkB )
            Abc_AigRemoveFromLevelStructureR( pMan->vLevelsR, pFanout );

        // remove the old fanout node from the structural hashing table
        Abc_AigAndDelete( pMan, pFanout );
        // remove the fanins of the old fanout
        Abc_ObjRemoveFanins( pFanout );
        // recreate the old fanout with new fanins and add it to the table
        Abc_AigAndCreateFrom( pMan, pFanin1, pFanin2, pFanout );
        assert( Abc_AigNodeIsAcyclic(pFanout, pFanout) );

        if ( fUpdateLevel )
        {
            // schedule the updated fanout for updating direct level
            assert( pFanout->fMarkA == 0 );
            pFanout->fMarkA = 1;
            Vec_VecPush( pMan->vLevels, pFanout->Level, pFanout );
            // schedule the updated fanout for updating reverse level
            if ( pMan->pNtkAig->vLevelsR ) 
            {
                assert( pFanout->fMarkB == 0 );
                pFanout->fMarkB = 1;
                Vec_VecPush( pMan->vLevelsR, Abc_ObjReverseLevel(pFanout), pFanout );
            }
        }

        // the fanout has changed, update EXOR status of its fanouts
        Abc_ObjForEachFanout( pFanout, pFanoutFanout, v )
            if ( Abc_AigNodeIsAnd(pFanoutFanout) )
                pFanoutFanout->fExor = Abc_NodeIsExorType(pFanoutFanout);
    }
    // if the node has no fanouts left, remove its MFFC
    if ( Abc_ObjFanoutNum(pOld) == 0 )
        Abc_AigDeleteNode( pMan, pOld );
}*/


void insertDC(Abc_Ntk_t *pNtk, int time) {
    Vec_Ptr_t *vNodes = Abc_NtkDfs(pNtk, 1);
    void *pObj;
    int i;
    Vec_PtrForEachEntry( void*, vNodes, pObj, i ) {
        Abc_Obj_t* pNode = (Abc_Obj_t*) pObj;
        if(!Abc_ObjIsNode(pNode)) 
            assert(!(pNode->fMarkA || pNode->fMarkB || pNode->fMarkC));
    }
    int tc = 0;
    Vec_PtrForEachEntry( void*, vNodes, pObj, i ) {
        if (tc >= time)
            break;
        if(!Abc_ObjIsNode((Abc_Obj_t*)pObj))
            continue;
        orReplace(pNtk, (Abc_Obj_t*)pObj);
        Abc_AigCleanup((Abc_Aig_t*)pNtk->pManFunc);
        tc++;
    }
    assert(Abc_AigCheck((Abc_Aig_t*)(pNtk->pManFunc)));
//    pNtk = resyn(pNtk);
    Vec_PtrFree(vNodes);
}
void orReplace(Abc_Ntk_t *pNtk, Abc_Obj_t *pOld) {
    assert(Abc_NtkIsStrash(pNtk));
    assert(Abc_ObjIsNode(pOld));
    assert(Abc_ObjFanoutNum(pOld) > 0);
    assert(Abc_ObjFaninNum(pOld) == 2);
    
    Abc_Obj_t *pNew= Abc_AigOr((Abc_Aig_t*)pNtk->pManFunc, Abc_ObjChild0(pOld), Abc_ObjChild1(pOld));
    
    Abc_AigReplace((Abc_Aig_t*)pNtk->pManFunc, pOld, pNew, 1);
//    assert( !Abc_ObjIsComplement(pOld) );

}
