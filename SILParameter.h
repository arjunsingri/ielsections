#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/LoopPass.h"
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
    bool isValuePhiNode(void) { return m_isValuePhiNode; }
    void setValuePhiNode(bool isPhiNode) { m_isValuePhiNode = isPhiNode; }

    bool isValueArray(void) { return m_isValueArray; }
    void setValueArray(bool isValueArray) { m_isValueArray = isValueArray; }
   
    //if m_value is not a phi node, then m_rd.size() iwill be atmost 1
    void addRD(Value* value, BasicBlock* containingBlock);
    std::vector<Value*> getRD(void) { return m_rd; }
    std::vector<BasicBlock*> getRDBlocks(void) { return m_rdContainingBlocks; }

    std::vector<Value*> getArrayDefinitions(void) { return m_arrayDefinitions; }
    void setArrayDefinitions(std::vector<Value*>& arrayDefs, std::vector<BasicBlock*>& blocks);
    std::vector<BasicBlock*> getArrayDefinitionBlocks(void) { return m_arrayDefinitionContainingBlocks; }

    void setRD(std::vector<Value*> rd, std::vector<BasicBlock*> containingBlocks);
   
    void addCP(Instruction* inst) { m_cp.push_back(inst); }
    std::vector<Instruction*> getCP(void) { return m_cp; }
    void setCP(std::vector<Instruction*> cp) { m_cp = cp; }

    SILValue getSILValue(void) { assert(m_silValue != NotInitialized); return m_silValue; }
    void setSILValue(SILValue silValue) { m_silValue = silValue; }

private:

    Loop* m_beta;
    
    Value* m_value;
    bool m_isValuePhiNode;
    bool m_isValueArray;
    
    Instruction* m_s;
    SILValue m_silValue;
    std::vector<Value*> m_rd;
    std::vector<BasicBlock*> m_rdContainingBlocks;
    std::vector<Value*> m_arrayDefinitions;
    std::vector<BasicBlock*> m_arrayDefinitionContainingBlocks;
    std::vector<Instruction*> m_cp;
};

typedef std::vector<SILParameter*> SILParameterList;


#endif //SILPARAMETER_H
