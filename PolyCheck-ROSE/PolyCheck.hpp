#include "rose.h"
//#include "AstMatching.h"
#include "Statement.hpp"

using std::string;
using std::vector;

class ScopTraversal: public AstSimpleProcessing {
    public:
        int source_flag = 0; // this processes original source file
        int scop_number = 0; // record scop number
        //AstMatching matcher;
        virtual void visit(SgNode* n);
};



