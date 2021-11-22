/**CFile****************************************************************
  FileName    [ssatParser.cc]
  SystemName  []
  PackageName []
  Synopsis    []
  Author      [Kuan-Hua Tu]
  
  Affiliation []
  Date        [Sun Jun 23 02:39:25 CST 2019]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "ssatelim.h"
#include <zlib.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
// #include <math.h>
#include <math.h>
using namespace std;

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static const int DEBUG_PRINT = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
static const unsigned BUFLEN = 1024000;

static void error(const char* const msg)
{
    std::cerr << msg << "\n";
    exit(255);
}

void ssat_readHeader_cnf(ssat_solver * s, string& str_in)
{
    string str = str_in.substr(6); // for the length of str "p cnf "
    std::string::size_type sz = 0;

    long long nVar = stoll (str, &sz);
    str = str.substr(sz);
    if (DEBUG_PRINT) std::cout << " nVar: " << nVar << '\n';
    ssat_solver_setnvars(s, nVar);
    long long nClause = stoll (str, &sz);
    if (DEBUG_PRINT) std::cout << " nClause: " << nClause << '\n';
}

void ssat_readHeader(ssat_solver * s, string& str)
{
    ssat_readHeader_cnf(s, str);
}

void ssat_readExistence(ssat_solver * s, string& str_in)
{
    Vec_Int_t * p = Vec_IntAlloc(0);
    string str = str_in.substr(2); // for the length of str "e "
    std::string::size_type sz = 0;
    if (DEBUG_PRINT) cout << "e:" << endl;

    while (str.length())
    {
        long long var = stoll (str, &sz);
        if (var == 0)
        {
            break;
        }

        if (DEBUG_PRINT) cout << " var:" << var << endl;
        Vec_IntPush(p, var);
        str = str.substr(sz + 1);
        while (str[0] == ' ')
        {
            str = str.substr(1);
        }
    }

    if (Vec_IntSize(p) > 0)
    {
        ssat_addexistence(s, Vec_IntArray(p), Vec_IntLimit(p));
    }
    Vec_IntFree(p);
}

void ssat_readForall(ssat_solver * s, string& str_in)
{
    Vec_Int_t * p = Vec_IntAlloc(0);
    string str = str_in.substr(2); // for the length of str "a "
    std::string::size_type sz = 0;
    if (DEBUG_PRINT) cout << "a:" << endl;

    while (str.length())
    {
        long long var = stoll (str, &sz);
        if (var == 0)
        {
            break;
        }

        if (DEBUG_PRINT) cout << " var:" << var << endl;
        Vec_IntPush(p, var);
        str = str.substr(sz + 1);
        while (str[0] == ' ')
        {
            str = str.substr(1);
        }
    }

    if (Vec_IntSize(p) > 0)
    {
        ssat_addforall(s, Vec_IntArray(p), Vec_IntLimit(p));
    }
    Vec_IntFree(p);
}

void ssat_readRandom(ssat_solver * s, string& str_in)
{
    Vec_Int_t * p = Vec_IntAlloc(0);
    string str = str_in.substr(2); // for the length of str "r "
    std::string::size_type sz = 0;
    if (DEBUG_PRINT) cout << "r:" << endl;

    double prob = stod (str, &sz);
    if (DEBUG_PRINT) cout << " prob:" << prob << endl;
    str = str.substr(sz + 1);

    while (str.length())
    {
        long long var = stoll (str, &sz);
        if (var == 0)
        {
            break;
        }

        if (DEBUG_PRINT) cout << " var:" << var << endl;
        Vec_IntPush(p, var);
        str = str.substr(sz + 1);
        while (str[0] == ' ')
        {
            str = str.substr(1);
        }
    }

    if (Vec_IntSize(p) > 0)
    {
        ssat_addrandom(s, Vec_IntArray(p), Vec_IntLimit(p), prob);
    }
    Vec_IntFree(p);
}

void ssat_readClause(ssat_solver * s, string& str_in)
{
    Vec_Int_t * p = Vec_IntAlloc(0);
    string str = str_in;
    std::string::size_type sz = 0;

    while (str.length())
    {
        long long lit = stoll (str, &sz);
        if (lit == 0)
        {
            break;
        }

        if (DEBUG_PRINT) cout << "lit:" << lit << endl;
        Vec_IntPush(p, toLitCond( abs(lit), signbit(lit) ));
        str = str.substr(sz + 1);
        while (str[0] == ' ')
        {
            str = str.substr(1);
        }
    }

    ssat_addclause(s, Vec_IntArray(p), Vec_IntLimit(p));
    Vec_IntFree(p);
}

void ssat_lineHandle(ssat_solver * s, string& in_str)
{
    if (in_str.length() == 0 || in_str[0] == '\n' || in_str[0] == '\r')
    {
        return;
    }
    else if (in_str[0] == 'c')
    {
        // skip command line
        return;
    }
    else if (in_str[0] == 'p')
    {
        ssat_readHeader(s, in_str);
    }
    else if (in_str[0] == 'e')
    {
        ssat_readExistence(s, in_str);
    }
    else if (in_str[0] == 'a')
    {
        ssat_readForall(s, in_str);
    }
    else if (in_str[0] == 'r')
    {
        ssat_readRandom(s, in_str);
    }
    else if (in_str[0] == '-' || isdigit(in_str[0]))
    {
        ssat_readClause(s, in_str);
    }
    else
    {
        cout << "Error: Unexpected symbol" << endl;
    }   
}

void process(ssat_solver * s, gzFile in)
{
    char buf[BUFLEN];
    char* offset = buf;

    for (;;) {
        int err, len = sizeof(buf)-(offset-buf);
        if (len == 0) error("Buffer to small for input line lengths");

        len = gzread(in, offset, len);

        if (len == 0) break;    
        if (len <  0) error(gzerror(in, &err));

        char* cur = buf;
        char* end = offset+len;

        for (char* eol; (cur<end) && (eol = std::find(cur, end, '\n')) < end; cur = eol + 1)
        {
            // std::cout << std::string(cur, eol) << "\n";
            string in_str = string(cur, eol);
            ssat_lineHandle(s, in_str);
        }

        // any trailing data in [eol, end) now is a partial line
        offset = std::copy(cur, end, buf);
    }

    // BIG CATCH: don't forget about trailing data without eol :)
    std::cout << std::string(buf, offset);

    if (gzclose(in) != Z_OK) error("failed gzclose");
}

void ssat_Parser(ssat_solver * s, char * filename)
{
    if (filename == NULL)
    {
        return;
    }

    process(s, gzopen(filename, "rb"));

    return;
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
