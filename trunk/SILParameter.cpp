#include "SILParameter.h"
#include "utils.h"

using namespace llvm;

const char* MapEnum2Str[] = {"NotInitialized", "True", "False", "DontKnow"};

SILParameter::SILParameter(Loop* beta, Value* value, Instruction* s)
    :   m_beta(beta), 
        m_value(value), 
        m_s(s), 
        m_silValue(NotInitialized),
        m_rejectionSource(NULL)
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
        m_definitions.push_back(m_value);
        m_definitionParents.push_back(loadInst->getParent());

        m_definitionParentPairs.push_back(DefinitionParentPair(m_value, loadInst->getParent()));

        for (std::vector<StoreInst*>::iterator i = stores.begin(); i != stores.end(); ++i)
        {
            m_definitions.push_back(*i);
            m_definitionParents.push_back((*i)->getParent());
        
            m_definitionParentPairs.push_back(DefinitionParentPair(*i, (*i)->getParent()));
        }
 
/*
        if (m_beta->getHeader()->getParent()->getName().str() == "mainQSort3")
        {
            Value* coreOperand = NULL;
            ReachingDef::findCoreOperand(loadInst->getPointerOperand(), &coreOperand);

            if (coreOperand->getName().str() == "budget")
            {
                std::cerr << "Line: " << getLineNumber(dyn_cast<Instruction>(m_value)) << " No: " << stores.size() << std::endl;
                for (std::vector<StoreInst*>::iterator i = stores.begin(); i != stores.end(); ++i)
                {
                    (*i)->dump();
                    std::cout << getLineNumber(*i) << std::endl;
                }
            }
        }
*/
    }
    else
    {
        m_definitions.push_back(m_value);
        m_definitionParents.push_back(dyn_cast<Instruction>(m_value)->getParent());
        
        m_definitionParentPairs.push_back(DefinitionParentPair(m_value, dyn_cast<Instruction>(m_value)->getParent()));
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
        Value* value = phiNode->getIncomingValue(i);
        BasicBlock* valueParent = phiNode->getIncomingBlock(i);

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
            if (Instruction* inst = dyn_cast<Instruction>(value))
            {
                m_definitions.push_back(value);
                m_definitionParents.push_back(inst->getParent());
        
                m_definitionParentPairs.push_back(DefinitionParentPair(value, inst->getParent()));
            }
            else
            {
                m_definitions.push_back(value);
                m_definitionParents.push_back(valueParent);

                m_definitionParentPairs.push_back(DefinitionParentPair(value, valueParent));
            }
        }
    }
}

void SILParameter::setSILValue(SILValue silValue)
{
    m_silValue = silValue;
}

void SILParameter::setSILValue(SILValue silValue, RejectedStep step)
{
    assert(silValue == False);
    m_silValue = silValue; 
    m_step = step;
}

void SILParameter::setSILValue(SILValue silValue, RejectedStep step, Instruction* step2Inst, SILParameter* rejectionSource)
{
    assert(silValue == False);
    m_silValue = silValue;
    m_step = step; 
    m_rejectionSource = rejectionSource; 
    m_step2Inst = step2Inst;
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

void SILParameter::printCP(void)
{
    print();
    for (int i = 0; i < m_cp.size(); ++i)
    {
        std::cerr << "cp\n";
        m_cp[i]->dump();
    }
    std::cerr << std::endl;
    std::cerr << std::endl;
}

void SILParameter::printRD(void)
{
    print();
    for (int i = 0; i < m_rd.size(); ++i)
    {
        std::cerr << "rd\t";
        m_definitions[m_rd[i]]->dump();
    }
    std::cerr << std::endl;
    std::cerr << std::endl;
}

void SILParameter::print(void)
{
    std::cerr << "Line: " << getLineNumber(m_beta->getHeader()->getFirstNonPHI()) << std::endl;
    std::cerr << "Loop: " << m_beta->getHeader()->getName().str() << std::endl;
    std::cerr << "Line: " << getLineNumber(m_s) << std::endl;
    std::cerr << "Value: ";
    std::cerr.flush();
    m_value->dump();
    //std::cerr << "Instruction: ";
    //std::cerr.flush();
    //m_s->dump();
    //printRejectionPath();
    printSILValue();
}

void SILParameter::printRejectionPath(void)
{
    if (m_step == Step1)
    {
        std::cerr << "Step1: ";
        for (unsigned int i = 0; i < m_definitions.size(); ++i)
        {
            if (Instruction* inst = dyn_cast<Instruction>(m_definitions[i]))
            {
                std::cerr << getLineNumber(inst) << "\t";
            }
        }

        std::cerr << std::endl;
    }
    else
    {
        if (m_rejectionSource != NULL)
        {
            if (m_step == Step2a)
            {
                std::cerr << "Step2a: ";
            }
            else
            {
                std::cerr << "Step2b: ";
            }

            std::cerr << getLineNumber(m_step2Inst) << std::endl;
            m_rejectionSource->printRejectionPath();
        }
    }
}

void SILParameter::printDefinitions(void)
{
    for (unsigned int i = 0; i < m_definitions.size(); ++i)
    {
         m_definitions[i]->dump();
    }
}
