#include "llvm/BasicBlock.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "ControlDependence/ControlDependence.h"
#include "ReachingDef/ReachingDef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Type.h"
#include "IELSection.h"
#include <string>
#include <iostream>
#include <fstream>

using namespace llvm;
 
#ifndef SIL_H
#define SIL_H

//class SIL : public LoopPass
class SIL : public FunctionPass
{
    std::vector<IELSection*> m_ielSections;
    std::vector<Loop*> m_loops;
    std::map<Value*, Value*> m_toArray;
    std::map<Value*, std::vector<Value*> >  m_arrayDefinitions;
    std::map<Value*, std::vector<BasicBlock*> > m_arrayDefinitionsBlocks;
    ReachingDef* m_currentReachingDef;
    int m_id;
    std::fstream m_file;
    std::map<Loop*, std::set<Loop*> > m_loopGraph;
    std::vector<int> m_histogram;

    public:
    
    struct Counts
    {
        Counts() : totalLoops(0), afterFinalCheck(0) { }
        int totalLoops;
        int afterFinalCheck;
    };
    
    Counts m_counts;

        static char ID;
        
        SIL();
        ~SIL();

        int getId(void) { return m_id++; }

        const std::vector<Loop*>& getLoops(void) { return m_loops; }
        const std::vector<Loop*>& getLoops(void) const { return m_loops; }
        const std::vector<IELSection*>& getIELSections(void) { return m_ielSections; }
        const std::vector<IELSection*>& getIELSections(void) const { return m_ielSections; }

        //perform step1 as per the paper and at the same time construct set RD for each triplet
        void runStep1(IELSection* ielSection);

        void getPhiDefinitions(PHINode* phiNode, std::vector<BasicBlock*>& udChainBlock, std::vector<Value*>& udChainInst, std::vector<PHINode*> phiNodes);

        void computeCP(IELSection* ielSection, SILParameter* silParameter, ControlDependence& cd);

        //return true if the SI/L value for this parameter changes from DontKnow to False
        bool recomputeSILValue(SILParameter* silParameter, IELSection* ielSection);
        void runStep2(IELSection* ielSection);

        void runStep3(IELSection* ielSection);

        //TODO: find suitable name
        //return NULL if this loop's body is not considered an IE/L section
        //the plan is to ignore all loops which call functions
        IELSection* createIELSection(Loop* loop);
        bool finalCheck(IELSection* ielSection);
        void checkOuterLoops(Loop* loop);

        //virtual bool runOnLoop(Loop* loop, LPPassManager &lpm);
        virtual bool runOnFunction(Function& function);
        IELSection* checkLoop(Loop* loop);

        static void isUsedInLoadStore(GetElementPtrInst* instr, bool &result);

        void printAdjacentLoops(Loop* loop, LoopInfo& loopInfo, char* id);
        void printNode(Loop* loop, char* id, bool isFilled);
        void printEdge(Loop* srcLoop, Loop* dstLoop, char* id);
        bool isAncestor(Loop* dstLoop, Loop* srcLoop);
        static int getSourceLine(Instruction* inst);

        void dump(void);
        virtual void getAnalysisUsage(AnalysisUsage& AU) const;
};

#endif //SIL_H
