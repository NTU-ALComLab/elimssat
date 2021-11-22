/**CFile****************************************************************
  FileName    [utilParser.cc]
  SystemName  [ABC: Logic synthesis and verification system.]
  PackageName []
  Synopsis    [Utility box for parsing]
  Author      [Kuan-Hua Tu(isuis3322@gmail.com)]
  
  Affiliation [NTU]
  Date        [2019.03.08]
***********************************************************************/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "utilParser.h"
#include "misc/zlib/zlib.h"
#include <iostream>
#include <algorithm>
#include <string>

ABC_NAMESPACE_IMPL_START
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
static const unsigned BUFLEN = 1048576;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
static void error(const char* const msg)
{
    std::cerr << msg << "\n";
    exit(255);
}

static void process(Util_ParseHandller& handler, gzFile in)
{
    char buf[BUFLEN];
    char* offset = buf;

    for (;;) {
        int err, len = sizeof(buf)-(offset-buf);
        if (len == 0) error("Erroe: Buffer to small for input line lengths");

        len = gzread(in, offset, len);

        if (len == 0) break;    
        if (len <  0) error(gzerror(in, &err));

        char* cur = buf;
        char* end = offset+len;

        for (char* eol; (cur<end) && (eol = std::find(cur, end, '\n')) < end; cur = eol + 1)
        {
            // std::cout << std::string(cur, eol) << "\n";
            std::string in_str = std::string(cur, eol);
            handler.line(in_str);
        }

        // any trailing data in [eol, end) now is a partial line
        offset = std::copy(cur, end, buf);
    }

    // BIG CATCH: don't forget about trailing data without eol :)
    // std::cout << std::string(buf, offset);

    if (gzclose(in) != Z_OK) error("Erroe: failed gzclose");
}

void Util_Parser(char * filename, Util_ParseHandller& handler)
{
    if (filename == NULL)
    {
        return;
    }
    gzFile in_file = gzopen (filename, "rb");

    process(handler, in_file);
}

ABC_NAMESPACE_IMPL_END
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
