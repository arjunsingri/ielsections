#include "llvm/BasicBlock.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ControlDependence.h"
#include "llvm/Analysis/ReachingDef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Type.h"
#include "IELSection.h"
#include <string>
#include <iostream>

using namespace llvm;
 
#ifndef SIL_H
#define SIL_H

class SIL : public LoopPass
//class SIL : public FunctionPass
{
    std::vector<IELSection*> m_ielSections;
    std::map<Value*, Value*> m_toArray;
    std::map<Value*, std::vector<Value*> >  m_arrayDefinitions;
    std::map<Value*, std::vector<BasicBlock*> > m_arrayDefinitionsBlocks;
    ReachingDef* m_currentReachingDef;
    int m_id;

    public:
        static char ID;
        
        SIL();

        int getId(void) { return m_id++; }

        const std::vector<IELSection*>& getIELSections(void) { return m_ielSections; }

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
        IELSection* addIELSection(Loop* loop);
        bool checkIELSection(IELSection* ielSection);
        virtual bool runOnLoop(Loop* loop, LPPassManager &lpm);
//        virtual bool runOnFunction(Function &function);

        void dump(void);
        virtual void getAnalysisUsage(AnalysisUsage& AU) const;
};

#endif //SIL_H
