#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "ReachingDef/ReachingDef.h"
#include "utils.h"
#include <string>
#include <iostream>

using namespace llvm;

#ifndef SILPARAMETER_H
#define SILPARAMETER_H

enum SILValue
{
    NotInitialized,
    True,
    False,
    DontKnow
};

typedef std::pair<Value*, BasicBlock*> DefinitionParentPair;
typedef std::vector<DefinitionParentPair> DefinitionParentPairs;

class SILParameter
{
public:

    enum RejectedStep
    {
        Step1,
        Step2a,
        Step2b
    };

    SILParameter(Loop* beta, Value* value, Instruction* s);
    void printSILValue(void);
    Loop* getLoop(void) { return m_beta; }

    Instruction* getInstruction(void) { return m_s; }

    Value* getValue(void) { return m_value; }
    
    void addRD(unsigned int i);
    void printRD(void);
    void addRDPair(Instruction* inst, BasicBlock* parent) { m_definitionParentPairs.push_back(DefinitionParentPair(inst, parent)); }
    void addRDPair(DefinitionParentPair pair) { m_definitionParentPairs.push_back(pair); }
    DefinitionParentPairs getRDPairs(void) { return m_rdPairs; }
    std::vector<unsigned int> getRD(void) { return m_rd; }
   
    void printCP(void);
    void addCP(Instruction* inst) { m_cp.push_back(inst); }
    std::vector<Instruction*> getCP(void) { return m_cp; }
    void setCP(std::vector<Instruction*> cp) { m_cp = cp; }

    SILValue getSILValue(void) { assert(m_silValue != NotInitialized); return m_silValue; }
    void setSILValue(SILValue silValue);
    void setSILValue(SILValue silValue, RejectedStep step);
    void setSILValue(SILValue silValue, RejectedStep step, Instruction* step2Inst, SILParameter* rejectionSource); 

    void constructDefinitionList(ReachingDef* reachingDef);

    unsigned int getNumDefinitions(void) { return m_definitions.size(); }
    std::vector<Value*> getDefinitions(void) { return m_definitions; }
    DefinitionParentPairs getDefinitionParentPairs(void) { return m_definitionParentPairs; }
    Value* getDefinition(unsigned int i) { return m_definitions[i]; }
    BasicBlock* getDefinitionParent(unsigned int i) { return m_definitionParents[i]; }

    void printDefinitions(void);
    void print(void);
    void printRejectionPath(void);

private:

    void findDefinitions(PHINode* phiNode);
    void findDefinitions(PHINode* phiNode, std::map<PHINode*, bool>& visited);

private:

    Loop* m_beta;
    
    Value* m_value;
    
    Instruction* m_s;
    SILValue m_silValue;

    std::vector<unsigned int> m_rd;
    DefinitionParentPairs m_rdPairs;
    std::vector<Instruction*> m_cp;

    std::vector<Value*> m_definitions;
    std::vector<BasicBlock*> m_definitionParents;
    DefinitionParentPairs m_definitionParentPairs;
    std::vector<int> m_rejectedPath;
    RejectedStep m_step;
    Instruction* m_step2Inst; //for lack of a better name
    SILParameter* m_rejectionSource;
};

typedef std::vector<SILParameter*> SILParameterList;


#endif //SILPARAMETER_H
