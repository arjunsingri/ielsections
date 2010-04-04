#include "SILParameter.h"

using namespace llvm;

const char* MapEnum2Str[] = {"NotInitialized", "True", "False", "DontKnow"};

SILParameter::SILParameter(Loop* beta, Value* value, Instruction* s)
    :   m_beta(beta), 
        m_value(value), 
        m_isValuePhiNode(false), 
        m_isValueArray(false), 
        m_s(s), 
        m_silValue(NotInitialized)
{
}

void SILParameter::printSILValue(void)
{
    switch (m_silValue)
    {
        case True:
            std::cout << "True" << std::endl;
            break;

        case False:
            std::cout << "False" << std::endl;
            break;

        case DontKnow:
            std::cout << "DontKnow" << std::endl;
            break;

        case NotInitialized:
            std::cout << "NotInitialized" << std::endl;
            break;

        default:
            assert(false && "really not initialized!");
    }
}

   //if m_value is not a phi node, then m_rd.size() iwill be atmost 1
void SILParameter::addRD(Value* value, BasicBlock* containingBlock)
{ 
    m_rd.push_back(value);
    m_rdContainingBlocks.push_back(containingBlock);
}

void SILParameter::setArrayDefinitions(std::vector<Value*>& arrayDefs, std::vector<BasicBlock*>& blocks)
{
    m_arrayDefinitions = arrayDefs;
    m_arrayDefinitionContainingBlocks = blocks;
}

#if 0
    void setRD(std::vector<std::pair<Value*, BasicBlock*> > defs)
    {
        for (std::vector<std::pair<Value*, BasicBlock*> >::iterator i = defs.begin(); i != defs.end(); ++i)
        {
            m_rd.push_back((*i).first);
            m_rdContainingBlocks.push_back((*i).second);
        }
    }
#endif
void SILParameter::setRD(std::vector<Value*> rd, std::vector<BasicBlock*> containingBlocks)
{
    m_rd = rd;
    m_rdContainingBlocks = containingBlocks;
}

