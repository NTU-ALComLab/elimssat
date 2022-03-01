#include <fstream>
#include <cstdio>
#include <iostream>
#include <map>
//#include <io.h>
#include "IntSolver.hpp"

#include "extMsat/Sort.h"
#include "bdd/cudd/cuddInt.h"

//#define _DEBUG
//#define out_cnf

extern "C" Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor );
extern "C" void Abc_NtkShow( Abc_Ntk_t * pNtk, int fGateNames, int fSeq, int fUseReverse );
extern "C" void Abc_NtkSynthesize( Abc_Ntk_t ** ppNtk, int fMoreEffort );
extern "C" Abc_Ntk_t * Abc_NtkIvyFraig( Abc_Ntk_t * pNtk, int nConfLimit, int fDoSparse, int fProve, int fTransfer, int fVerbose );
extern "C" Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor );
extern "C" Abc_Ntk_t * Abc_NtkIvyRewrite( Abc_Ntk_t * pNtk, int nConfLimit, int fDoSparse, int fProve );

using namespace std;

static void writeNode(ofstream& ofile, map<const Abc_Obj_t*, int> &mVar, char *pSop0, char *pSop1, Abc_Obj_t* pNode) {
    Abc_Obj_t * pFanin;
    int i, c, nFanins;
    char * pCube;

    //veci* lits;
    vec<Lit> lits;


    nFanins = Abc_ObjFaninNum( pNode );
    assert( nFanins == Abc_SopGetVarNum( pSop0 ) );

    if ( Cudd_Regular(pNode->pData) == Cudd_ReadOne((DdManager*)pNode->pNtk->pManFunc) )
    {
        lits.clear();
        if ( !Cudd_IsComplement(pNode->pData) )
            lits.push(Lit(mVar[pNode]));
        else
            lits.push(~Lit(mVar[pNode]));
#ifdef _DEBUG
        printLits(lits);
#endif
        // pSat->addAClause(lits);
        ofile << lits.size() << " ";
        for(int i = 0; i < lits.size(); i++) {
            if(sign(lits[i]))
                ofile << "-";
            ofile << var(lits[i]) << " ";
        }
        ofile << endl;
        return;
    }

    // add pCnf for the negative phase
    for ( c = 0; ; c++ )
    {
        // get the cube
        pCube = pSop0 + c * (nFanins + 3);
        if ( *pCube == 0 )
            break;
        // add the clause

        lits.clear();
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            assert(Abc_ObjIsNode(pFanin) || Abc_ObjIsPi(pFanin));
            if ( pCube[i] == '0' )
                lits.push(Lit(mVar[pFanin]));
            else if ( pCube[i] == '1' )
                lits.push(~Lit(mVar[pFanin]));
        }
        lits.push(~Lit(mVar[pNode]));
#ifdef _DEBUG
        printLits(lits);
#endif
        ofile << lits.size() << " ";
        for(int i = 0; i < lits.size(); i++) {
            if(sign(lits[i]))
                ofile << "-";
            ofile << var(lits[i]) << " ";
        }
        ofile << endl;
    }

    // add pCnf for the positive phase
    for ( c = 0; ; c++ )
    {
        // get the cube
        pCube = pSop1 + c * (nFanins + 3);
        if ( *pCube == 0 )
            break;
        // add the clause
        lits.clear();
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            assert(Abc_ObjIsNode(pFanin) || Abc_ObjIsPi(pFanin));
            if ( pCube[i] == '0' )
                lits.push(Lit(mVar[pFanin]));
            else if ( pCube[i] == '1' )
                lits.push(~Lit(mVar[pFanin]));
        }
        lits.push(Lit(mVar[pNode]));
#ifdef _DEBUG
        printLits(lits);
#endif
        //pSat->addAClause(lits);
        ofile << lits.size() << " ";
        for(int i = 0; i < lits.size(); i++) {
            if(sign(lits[i]))
                ofile << "-";
            ofile << var(lits[i]) << " ";
        }
        ofile << endl;
    }
}
void writeCnf(Abc_Ntk_t* r, int curVar, map<const Abc_Obj_t*, Var> & mPVar) {
    ofstream intfile;

    intfile.open("int.tcnf");

    curVar+= 3;
    //    cout << "cuvvar " << curVar << endl;
    Abc_Ntk_t *pNtk = Abc_NtkMulti( r, 0, 100, 1, 0, 0, 0 );


    Mem_Flex_t * pMmFlex;
    Abc_Obj_t * pNode;
    Vec_Str_t * vCube;
    char * pSop0, * pSop1;
    int fAllPrimes = 0;

    vec<Lit> lits;
    int i;

    map<const Abc_Obj_t*, int> mVar;

    assert( Abc_NtkIsBddLogic(pNtk) );

    pMmFlex = Mem_FlexStart();
    vCube   = Vec_StrAlloc( 100 );



    Abc_NtkForEachNode(pNtk, pNode, i) {
        mVar[pNode] = (curVar)++; 
    }
    Abc_NtkForEachPi(pNtk, pNode, i) {
        mVar[pNode] = atoi(Abc_ObjName(pNode)) + 1;
    }

    // add pCnf for each internal nodes
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // derive SOPs for both phases of the node
        Abc_NodeBddToCnf( pNode, pMmFlex, vCube, fAllPrimes, &pSop0, &pSop1 );
        writeNode(intfile, mVar, pSop0, pSop1, pNode);
        //   addNodeClauses(pSat, mVar, cVar, pSop0, pSop1, pNode);
    }

    assert(Abc_NtkPoNum(pNtk) == 1);

    // printf("R output CNF\n");
    // Abc_NtkForEachPo(pNtk, pNode, i) {

    pNode = Abc_NtkPo(pNtk, 0);
    Abc_Obj_t* pFanin = Abc_ObjFanin0(pNode);
    assert(Abc_ObjIsNode(pFanin) || Abc_ObjIsPi(pFanin));
    lits.clear();
    lits.push(Lit(mVar[pFanin]));
#ifdef _DEBUG
    printLits(lits);
#endif
    intfile << lits.size() << " ";
    for(int i = 0; i < lits.size(); i++) {
        if(sign(lits[i]))
            intfile << "-";
        intfile << var(lits[i]) << " ";
    }
    intfile << endl;
    //pSat->addAClause(lits);
    // }

#ifdef _DEBUG
    //    cout << "ctr var list: " << endl;
    //    printLits(ctrlVar);
#endif
    Vec_StrFree( vCube );
    Mem_FlexStop2( pMmFlex );
    Abc_NtkDelete(pNtk);

    intfile.close();
    return;
}
static void resolve(vec<Lit>& main, vec<Lit>& other, Var x)
{
    Lit     p;
    bool    ok1 = false, ok2 = false;
    for (int i = 0; i < main.size(); i++){
        if (var(main[i]) == x){
            ok1 = true, p = main[i];
            main[i] = main.last();
            main.pop();
            break;
        }
    }

    for (int i = 0; i < other.size(); i++){
        if (var(other[i]) != x)
            main.push(other[i]);
        else{
            if (p != ~other[i])
                printf("PROOF ERROR! Resolved on variable with SAME polarity in both clauses: %d\n", x+1);
            ok2 = true;
        }
    }

    if (!ok1 || !ok2)
        printf("PROOF ERROR! Resolved on missing variable: %d\n", x+1);

    sortUnique(main);
}

struct Mcm_Checker : public ProofTraverser 
{
    vec<vec<Lit> >  clauses;
    std::map<const Var, bool> mALoc;
    std::map<const unsigned, bool> mAClause;
    std::map<const Var, Abc_Obj_t*> mPi;
    std::vector<Abc_Obj_t*> vNode;
    Abc_Ntk_t* temp_ntk;
    Abc_Obj_t* objNew;

#ifdef out_cnf
    std::ofstream afile;
    std::ofstream bfile;

#endif 

    int maxVar;

    void root   (const vec<Lit>& c) {

        std::vector<Lit> r;		// register global var
    
        
        // print info & register global variable
#ifdef _DEBUG
        /**/printf("%d: ROOT", clauses.size()); for (int i = 0; i < c.size(); i++) printf(" %s%d", sign(c[i])?"-":"", var(c[i])); printf("\t");
#endif        
        if(!mAClause[clauses.size()]) { // b clause
#ifdef _DEBUG
            cout << "b clause" << endl;
#endif
  
#ifdef out_cnf
            bfile << c.size() << " ";
#endif
            for(int i = 0; i < c.size(); i++) {
                if(sign(c[i]))
#ifdef out_cnf
                    bfile << "-";
                bfile << var(c[i]) + 1<< " ";
#endif
                maxVar = (maxVar >= var(c[i]))? maxVar: var(c[i]);
            }
#ifdef out_cnf 
            bfile << endl;
#endif      


	    	      
            objNew = Abc_AigConst1(temp_ntk);	// const
            vNode.push_back(objNew);
            clauses.push();
            c.copyTo(clauses.last()); 
            return;
        }
#ifdef _DEBUG
        printf("a clause\n");
#endif
        for (int i = 0; i < c.size(); i++) {
            if(mPi[var(c[i])]!= NULL) {
                r.push_back(c[i]);
            }
        }
#ifdef out_cnf
            afile << c.size() << " ";
#endif
            for(int i = 0; i < c.size(); i++) {
                if(sign(c[i]))
#ifdef out_cnf
                    afile << "-";
                afile << var(c[i]) + 1<< " ";
#endif
                maxVar = (maxVar >= var(c[i]))? maxVar: var(c[i]);
            }
#ifdef out_cnf
            afile << endl;
#endif
        clauses.push();
        c.copyTo(clauses.last()); 


        // a clause 
        if(r.size() == 0) { // a clause with no global var
            objNew = Abc_ObjNotCond(Abc_AigConst1(temp_ntk),1);
        }
        else {
            if(!sign(r[0]))
                objNew = mPi[var(r[0])];
            else 
                objNew = Abc_ObjNot(mPi[var(r[0])]);

            for(unsigned i = 1; i < r.size(); i++) {

                if(sign(r[i])) {
                    objNew = Abc_AigOr((Abc_Aig_t*)temp_ntk->pManFunc, objNew, Abc_ObjNot(mPi[var(r[i])]));
                }
                else {
                    objNew = Abc_AigOr((Abc_Aig_t*)temp_ntk->pManFunc, objNew, mPi[var(r[i])]);
                }
            }
        }
        vNode.push_back(objNew);
    }

    void chain  (const vec<ClauseId>& cs, const vec<Var>& xs) {
#ifdef _DEBUG   
        printf("%d: CHAIN %d", clauses.size(), cs[0]); for (int i = 0; i < xs.size(); i++) printf(" [%d] %d", xs[i], cs[i+1]);
#endif
        objNew = vNode[cs[0]];
        for (int i = 0; i < xs.size(); i++) {
#ifdef _DEBUG
           // printf(" [%d] %d", xs[i]+1, cs[i+1]);
#endif
            if(mALoc[xs[i]] == true) {
                objNew = Abc_AigOr((Abc_Aig_t*)temp_ntk->pManFunc, objNew, vNode[cs[i+1]]);
            }
            else {
                objNew = Abc_AigAnd((Abc_Aig_t*)temp_ntk->pManFunc, objNew, vNode[cs[i+1]]);
            }
        }

        vNode.push_back(objNew);

        clauses.push();
        vec<Lit>& c = clauses.last();
        clauses[cs[0]].copyTo(c);
        for (int i = 0; i < xs.size(); i++)
            resolve(c, clauses[cs[i+1]], xs[i]);
#ifdef _DEBUG  
        printf(" =>"); for (int i = 0; i < c.size(); i++) printf(" %s%d", sign(c[i])?"-":"", var(c[i])); printf("\n");
#endif
    }

    void deleted(ClauseId c) {
        clauses[c].clear(); }
};
struct Pud_Checker : public ProofTraverser 
{
    vec<vec<Lit> >  clauses;
    std::map<const Var, bool> mALoc;
    std::map<const Var, bool> mBLoc;
    std::map<const Var, bool> mCom;
    std::map<const unsigned, bool> mAClause;
    std::map<const Var, Abc_Obj_t*> mPi;
    std::vector<Abc_Obj_t*> vNode;
    Abc_Ntk_t* temp_ntk;
    Abc_Obj_t* objNew;

#ifdef out_cnf
    std::ofstream afile;
    std::ofstream bfile;

#endif 

    int maxVar;

    void root   (const vec<Lit>& c) {


        // print info & register global variable
#ifdef _DEBUG
        /**/printf("%d: ROOT", clauses.size()); for (int i = 0; i < c.size(); i++) printf(" %s%d", sign(c[i])?"-":"", var(c[i])); printf("\t");
#endif        
        if(!mAClause[clauses.size()]) { // b clause
#ifdef _DEBUG
            cout << "b clause" << endl;
#endif

#ifdef out_cnf
            bfile << c.size() << " ";
#endif
            for(int i = 0; i < c.size(); i++) {
                if(sign(c[i]))
#ifdef out_cnf
                    bfile << "-";
                bfile << var(c[i]) + 1<< " ";
#endif
                maxVar = (maxVar >= var(c[i]))? maxVar: var(c[i]);
            }
#ifdef out_cnf 
            bfile << endl;
#endif      



            objNew = Abc_AigConst1(temp_ntk);	// const
            vNode.push_back(objNew);
            clauses.push();
            c.copyTo(clauses.last()); 
            return;
        }
#ifdef _DEBUG
        printf("a clause\n");
#endif
#ifdef out_cnf
        afile << c.size() << " ";
#endif
        for(int i = 0; i < c.size(); i++) {
            if(sign(c[i]))
#ifdef out_cnf
                afile << "-";
            afile << var(c[i]) + 1<< " ";
#endif
            maxVar = (maxVar >= var(c[i]))? maxVar: var(c[i]);
        }
#ifdef out_cnf
        afile << endl;
#endif

        // a clause 
        objNew = Abc_ObjNot(Abc_AigConst1(temp_ntk));
        vNode.push_back(objNew);
        clauses.push();
        c.copyTo(clauses.last()); 
    }

    void chain  (const vec<ClauseId>& cs, const vec<Var>& xs) {
#ifdef _DEBUG   
        printf("%d: CHAIN %d", clauses.size(), cs[0]); for (int i = 0; i < xs.size(); i++) printf(" [%d] %d", xs[i], cs[i+1]);
#endif
        objNew = vNode[cs[0]];
        for (int i = 0; i < xs.size(); i++) {
#ifdef _DEBUG
            // printf(" [%d] %d", xs[i]+1, cs[i+1]);
#endif
            if(mALoc[xs[i]] == true) {
                objNew = Abc_AigOr((Abc_Aig_t*)temp_ntk->pManFunc, objNew, vNode[cs[i+1]]);
            }
            else if(mBLoc[xs[i]] == true) {
                objNew = Abc_AigAnd((Abc_Aig_t*)temp_ntk->pManFunc, objNew, vNode[cs[i+1]]);
            }
            else {
                assert(mCom[xs[i]]);
                vec<Lit> &tLits = clauses[cs[i+1]];

                Lit v = Lit(xs[i] + 1);
                for(int j = 0; j < tLits.size(); j++) {
                    if(var(tLits[j]) == xs[i]) {
                        v = tLits[j]; 
                        break;
                    }
                }
                assert(var(v) == xs[i]);
                Abc_Obj_t *pObjA;
                Abc_Obj_t *pObjB;
                if(sign(v)) {
                    pObjA = Abc_AigAnd((Abc_Aig_t*)temp_ntk->pManFunc, objNew, Abc_ObjNot(mPi[xs[i]]));
                    pObjB = Abc_AigAnd((Abc_Aig_t*)temp_ntk->pManFunc, vNode[cs[i+1]], mPi[xs[i]]);
                }
                else {
                    pObjA = Abc_AigAnd((Abc_Aig_t*)temp_ntk->pManFunc, objNew, mPi[xs[i]]);
                    pObjB = Abc_AigAnd((Abc_Aig_t*)temp_ntk->pManFunc, vNode[cs[i+1]], Abc_ObjNot(mPi[xs[i]]));
                }
                objNew = Abc_AigOr((Abc_Aig_t*)temp_ntk->pManFunc, pObjA, pObjB);
            }
        }

        vNode.push_back(objNew);

        clauses.push();
        vec<Lit>& c = clauses.last();
        clauses[cs[0]].copyTo(c);
        for (int i = 0; i < xs.size(); i++)
            resolve(c, clauses[cs[i+1]], xs[i]);
#ifdef _DEBUG  
        printf(" =>"); for (int i = 0; i < c.size(); i++) printf(" %s%d", sign(c[i])?"-":"", var(c[i])); printf("\n");
#endif
    }

    void deleted(ClauseId c) {
        clauses[c].clear(); }
};
IntSolver::IntSolver(IntType t) { 
    IntSolver();
    type = t;
}
IntSolver::IntSolver() : Solver() {
    proof = new Proof();
    type = MCM_TYPE;
//    type = PUD_TYPE;
}
IntSolver::~IntSolver() {
    mALocal.clear();
    mBLocal.clear();
    mCom.clear();
    aClause.clear();
    delete proof;
}

void
IntSolver::addBClause(const vec<Lit> &ps) {
    for(int i = 0; i < ps.size(); i++) {
        if(mCom[var(ps[i])])
            continue;
        else if(mALocal[var(ps[i])]) {
            mALocal[var(ps[i])] = false;
            mBLocal[var(ps[i])] = false;
            mCom[var(ps[i])] = true;
        }
        else 
            mBLocal[var(ps[i])] = true;
    }

    addClause(ps);
}
void 
IntSolver::addAClause(const vec<Lit> &ps) {

    for(int i = 0; i < ps.size(); i++) {
        if(mCom[var(ps[i])])
            continue;
        else if(mBLocal[var(ps[i])]) {
            mALocal[var(ps[i])] = false;
            mBLocal[var(ps[i])] = false;
            mCom[var(ps[i])] = true;
        }
        else 
            mALocal[var(ps[i])] = true;
    }
    int n = proof->getClauseId();

    addClause(ps);
    if(n != proof->getClauseId())    
        aClause[n]= true;   

}

int 
    IntSolver::setnVars(int n) {
        while(nVars() < n) 
            newVar();

        return assigns.size();
    }

// in current version, do not compute common vars, assume common vars are the
// latchs of pNtk;

Abc_Ntk_t* 
IntSolver::getInt(map<const Var, char *> &varToname) {
    Abc_Ntk_t *intNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);

    map<const Var, Abc_Obj_t*> mPi;		// the pi is the common var
    map<const Abc_Obj_t*, Var> mPVar;		// the pi is the common var

    for(map<const Var, bool>::iterator it = mCom.begin(); it != mCom.end(); it++) {

        if(it->second) {

            Abc_Obj_t *pObjNew = Abc_NtkCreatePi(intNtk);
            char name[100];
            sprintf(name,"%s", varToname[it->first]);
            Abc_ObjAssignName(pObjNew, name, NULL);

            mPi[it->first] = pObjNew; //Abc_NtkPi(intNtk,it->first - shift);

        }
    }
    /*cout << "type: "<< type << endl;
    cout << "PUD_TYPE: "<< PUD_TYPE<< endl;
    cout << "MCM_TYPE: "<< MCM_TYPE<< endl;
*/
    if(type == PUD_TYPE) {
    Pud_Checker* trav = new Pud_Checker();
    trav->mALoc = mALocal;
    trav->mBLoc = mBLocal;
    trav->mCom = mCom;
    trav->mAClause = aClause;
    trav->temp_ntk = intNtk;
    trav->mPi = mPi;
    trav->maxVar = -1; 

    proof->traverse(*trav, ClauseId_NULL);
    vec<Lit>& c = trav->clauses.last();
    if (c.size() == 0) {
        Abc_Obj_t* out = Abc_NtkCreatePo(intNtk);
        char name[10];
        sprintf(name, "%s", "test");
        Abc_ObjAssignName(out, name, NULL);
        Abc_ObjAddFanin(out, trav->vNode.back());
    }
    else{

        Abc_Obj_t* out = Abc_NtkCreatePo(intNtk);
        char name[10];
        sprintf(name, "%s", "test");
        Abc_ObjAssignName(out, name, NULL);

        Abc_ObjAddFanin(out, trav->vNode.back());
    }

    if ( !Abc_NtkCheck( intNtk) ) {
        cout << "The Interpolant AIG check has failed" << endl;
        return NULL;
    }

    for(int i = 0; i < trav->clauses.size(); i++)
        trav->clauses[i].clear();

    trav->clauses.clear();
    trav->mALoc.clear();
    trav->mBLoc.clear();
    trav->mCom.clear();
    trav->mAClause.clear();
    trav->mPi.clear();
    }
    else if(type == MCM_TYPE) {
    Mcm_Checker* trav = new Mcm_Checker();
    trav->mALoc = mALocal;
    //trav->mBLoc = mBLocal;
    trav->mAClause = aClause;
    //trav->mCom = mCom;
    trav->temp_ntk = intNtk;
    trav->mPi = mPi;
    trav->maxVar = -1; 

    proof->traverse(*trav, ClauseId_NULL);
    vec<Lit>& c = trav->clauses.last();
    if (c.size() == 0) {
        Abc_Obj_t* out = Abc_NtkCreatePo(intNtk);
        char name[10];
        sprintf(name, "%s", "test");
        Abc_ObjAssignName(out, name, NULL);
        Abc_ObjAddFanin(out, trav->vNode.back());
    }
    else{

        Abc_Obj_t* out = Abc_NtkCreatePo(intNtk);
        char name[10];
        sprintf(name, "%s", "test");
        Abc_ObjAssignName(out, name, NULL);

        Abc_ObjAddFanin(out, trav->vNode.back());
    }

    if ( !Abc_NtkCheck( intNtk) ) {
        cout << "The Interpolant AIG check has failed" << endl;
        return NULL;
    }

    for(int i = 0; i < trav->clauses.size(); i++)
        trav->clauses[i].clear();

    trav->clauses.clear();
    trav->mALoc.clear();
    //trav->mBLoc.clear();
    //trav->mCom.clear();
    trav->mAClause.clear();
    trav->mPi.clear();
    }
    Abc_AigCleanup((Abc_Aig_t*)intNtk->pManFunc);

    vector<Abc_Obj_t*> vPi;
    int j;
    Abc_Obj_t* pObj;
    Abc_NtkForEachPi(intNtk, pObj, j) {
        if(Abc_ObjFanoutNum(pObj) == 0)
            vPi.push_back(pObj);
    }
    for(unsigned i = 0; i < vPi.size(); i++) {
        Abc_NtkDeleteObj(vPi[i]);
    }
    
    //intNtk = resyn(intNtk);
#ifdef _DEBUG
    //    Abc_NtkShow(intNtk, 0, 0, 0);
    //    getchar();
#endif  
    return intNtk;
}

