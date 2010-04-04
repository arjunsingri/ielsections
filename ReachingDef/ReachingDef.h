//===- ReachingDef.h - ReachingDef class definition -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the ReachingDef class, which is
// used for computing reaching definitions for arrays.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_REACHINGDEF_H
#define LLVM_REACHINGDEF_H

#include "llvm/Pass.h"
#include "llvm/BasicBlock.h"
#include <vector>
#include <map>

using namespace llvm;

typedef std::pair<Instruction*, bool> DownwardsExposedElementType;
typedef std::map<Instruction*, bool> DownwardsExposedMapType;

typedef std::pair<Value*, Instruction*> LastWriteElementType;
typedef std::map<Value*, Instruction*> LastWriteMapType;

typedef std::set<Instruction*> GenSetType;
typedef std::set<Instruction*> KillSetType;
typedef std::set<Instruction*> InSetType;
typedef std::set<Instruction*> OutSetType;

class BasicBlockDup
{
    public:
        BasicBlockDup(BasicBlock* originalBlock) : m_originalBlock(originalBlock) {}
    
        DownwardsExposedMapType& getDownwardsExposedMap(void) { return m_downwardsExposedMap; }
        LastWriteMapType& getLastWriteMap(void) { return m_lastWriteMap; }

        void addToGenSet(Instruction* inst) { m_genSet.insert(inst); }
        GenSetType& getGenSet(void) { return m_genSet; }
 
        void addToKillSet(std::vector<Instruction*>& killed);
        KillSetType& getKillSet(void) { return m_killSet; }

        InSetType& getInSet(void) { return m_inSet; }
        void setInSet(InSetType& inSet) { m_inSet = inSet; }
        OutSetType& getOutSet(void) { return m_outSet; }

    private:
        
        BasicBlock* m_originalBlock;
        DownwardsExposedMapType m_downwardsExposedMap;
        LastWriteMapType m_lastWriteMap;
        GenSetType m_genSet;
        KillSetType m_killSet;

        InSetType m_inSet;
        OutSetType m_outSet;
};

//===----------------------------------------------------------------------===//
//
// ReachingDef class - This class computes the reaching definitions for arrays 
//    by treating the defintion of any element of an array as a definition
//    of the array
//
class ReachingDef : public FunctionPass
{
    // type for representing control dependents - if a is control dependent on b
    // then a will be in the vector for b
    //
    typedef std::map<Instruction*, std::vector<Instruction*> > KilledMapType;
    typedef std::map<Value*, std::vector<Instruction*> > AssignmentMapType;
    typedef std::map<BasicBlock*, BasicBlockDup*> BasicBlockDupMapType;
    typedef std::pair<BasicBlock*, BasicBlockDup*> BasicBlockDupElementType;
    typedef std::pair<LoadInst*, std::vector<StoreInst*> > UDChainElementType;
    typedef std::map<LoadInst*, std::vector<StoreInst*> > UDChainMapType;

    AssignmentMapType m_assignmentMap;
    KilledMapType m_killedMap;
    BasicBlockDupMapType m_basicBlockDupMap;
    UDChainMapType m_udChain;

    public:
        static char ID;
        ReachingDef() : FunctionPass(&ID) {}

        ~ReachingDef();

        Value* findCoreOperand(Value* pointerOperand);
        void findDownwardsExposed(BasicBlock* block);
        
        void constructGenSet(BasicBlock* block);
        void constructKillSet(BasicBlock* block);
        void constructInSets(Function& function);
        void constructUDChain(Function& function);

        void findDefinitions(Value* coreOperand, BasicBlockDup* blockDup, LoadInst* loadInst);

        // Compute control dependences for all blocks in this function
        virtual bool runOnFunction(Function& F);

        virtual void getAnalysisUsage(AnalysisUsage& AU) const;

        void printa(void);

        //print - Show contents in human readable format...
        //virtual void print(std::ostream& O, const Module* = 0) const;

    private:
        void clear();
};

#endif // LLVM_REACHINGDEF_H

