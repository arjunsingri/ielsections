#include "SILParameter.h"

using namespace llvm;

const char* MapEnum2Str[] = {"NotInitialized", "True", "False", "DontKnow"};

SILParameter::SILParameter(Loop* beta, Value* value, Instruction* s)
    :   m_beta(beta), 
        m_value(value), 
        m_s(s), 
        m_silValue(NotInitialized)
{
}

void SILParameter::constructDefinitionList(ReachingDef* reachingDef)
{
    assert(reachingDef != NULL);

    if (PHINode* phiNode = dyn_cast<PHINode>(m_value))
    {
        findDefinitions(phiNode);
    }
    else if (LoadInst* loadInst = dyn_cast<LoadInst>(m_value))
    {
        std::vector<StoreInst*>& stores = reachingDef->getDefinitions(loadInst);

        //TODO:is this really required?
        //m_definitions.push_back(m_value);
        //m_definitionParents.push_back(loadInst->getParent());

        for (std::vector<StoreInst*>::iterator i = stores.begin(); i != stores.end(); ++i)
        {
            m_definitions.push_back(*i);
            m_definitionParents.push_back((*i)->getParent());
        }
    }
    else
    {
        m_definitions.push_back(m_value);
        m_definitionParents.push_back(dyn_cast<Instruction>(m_value)->getParent());
    }

    assert(m_definitions.size() == m_definitionParents.size());
}

void SILParameter::findDefinitions(PHINode* phiNode)
{
    std::map<PHINode*, bool> visited;

    findDefinitions(phiNode, visited);
}

void SILParameter::findDefinitions(PHINode* phiNode, std::map<PHINode*, bool>& visited)
{
    visited[phiNode] = true;
    unsigned int incomingSize = phiNode->getNumIncomingValues();
    for (unsigned int i = 0; i < incomingSize; ++i)
    {
        BasicBlock* valueParent = phiNode->getIncomingBlock(i);
        Value* value = phiNode->getIncomingValue(i);

        if (isa<UndefValue>(value)) continue;
        assert(!isa<BasicBlock>(value));

        if (PHINode* valueAsPhiNode = dyn_cast<PHINode>(value))
        {
            if (visited.find(valueAsPhiNode) == visited.end())
            {
                findDefinitions(valueAsPhiNode, visited);
            }
        }
        else
        {
            m_definitions.push_back(value);
            m_definitionParents.push_back(valueParent);
        }
    }
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
void SILParameter::addRD(unsigned int i)
{
    m_rd.push_back(i);
    assert(m_beta->contains(m_definitionParents[i]));
}

void SILParameter::printDefinitions(void)
{
    for (unsigned int i = 0; i < m_definitions.size(); ++i)
    {
        std::cout << *m_definitions[i] << std::endl;
    }
}
