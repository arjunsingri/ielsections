#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "ReachingDef/ReachingDef.h"
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


class SILParameter
{
    public:

    SILParameter(Loop* beta, Value* value, Instruction* s);
    void printSILValue(void);
    Loop* getLoop(void) { return m_beta; }

    Instruction* getInstruction(void) { return m_s; }

    Value* getValue(void) { return m_value; }
    
    void addRD(unsigned int i);
    std::vector<unsigned int> getRD(void) { return m_rd; }
   
    void addCP(Instruction* inst) { m_cp.push_back(inst); }
    std::vector<Instruction*> getCP(void) { return m_cp; }
    void setCP(std::vector<Instruction*> cp) { m_cp = cp; }

    SILValue getSILValue(void) { assert(m_silValue != NotInitialized); return m_silValue; }
    void setSILValue(SILValue silValue) { m_silValue = silValue; }

    void constructDefinitionList(ReachingDef* reachingDef);

    unsigned int getNumDefinitions(void) { return m_definitions.size(); }
    Value* getDefinition(unsigned int i) { return m_definitions[i]; }
    BasicBlock* getDefinitionParent(unsigned int i) { return m_definitionParents[i]; }

private:

    void findDefinitions(PHINode* phiNode);
    void findDefinitions(PHINode* phiNode, std::map<PHINode*, bool>& visited);

private:

    Loop* m_beta;
    
    Value* m_value;
    
    Instruction* m_s;
    SILValue m_silValue;

    std::vector<unsigned int> m_rd;
    std::vector<Instruction*> m_cp;

    std::vector<Value*> m_definitions;
    std::vector<BasicBlock*> m_definitionParents;
};

typedef std::vector<SILParameter*> SILParameterList;


#endif //SILPARAMETER_H
