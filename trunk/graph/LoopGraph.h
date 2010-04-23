#include "llvm/BasicBlock.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Type.h"
#include "../SIL.h"
#include <iostream>
#include <fstream>

using namespace llvm;
 
#ifndef LOOPGRAPH_H
#define LOOPGRAPH_H

class LoopGraph : public FunctionPass
{
    std::map<Loop*, std::vector<Loop*> > m_loopGraph;
    std::fstream m_file;

    public:
        static char ID;
        
        LoopGraph();
        ~LoopGraph();
        
        Loop* findContainingLoop(BasicBlock* block, const std::vector<Loop*>& loops);
        void generateDotFile(std::string functionName);
        void constructLoopGraph(Function& function);

        virtual bool runOnFunction(Function& function);

        virtual void getAnalysisUsage(AnalysisUsage& AU) const;
};

#endif //SIL_H
