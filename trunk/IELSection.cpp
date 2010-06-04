#include "IELSection.h"
#include "llvm/Analysis/DebugInfo.h"
#include "utils.h"
#include <string>

using namespace llvm;

IELSection::IELSection(Loop* loop, int id)
    :   m_loop(loop), 
        m_isIELSection(false), 
        m_id(id)
{
}

IELSection::~IELSection()
{
    for (unsigned int i = 0; i < m_silParameters.size(); ++i)
    {
        delete m_silParameters[i];
    }
}

void IELSection::addSILParameter(SILParameter* silParameter)
{
    assert(silParameter != NULL);
    m_silParameters.push_back(silParameter);
    m_silParametersMap[silParameter->getValue()] = silParameter;
}

SILParameter* IELSection::getSILParameter(Value* value) 
{ 
    std::map<Value*, SILParameter*>::iterator where = m_silParametersMap.find(value);
    if (where != m_silParametersMap.end())
    {
        return where->second;
    }

    return NULL;
}

bool IELSection::usedInLoadStore(GetElementPtrInst* instr)
{
    bool result = false;
    usedInLoadStore(instr, result);
    return result;
}

void IELSection::usedInLoadStore(GetElementPtrInst* instr, bool &result)
{
    for (Value::use_iterator i = instr->use_begin(); i != instr->use_end(); ++i)
    {
        //Instruction* use = dyn_cast<Instruction>(*i);
        //assert(use != NULL);

        if (GetElementPtrInst* instr2 = dyn_cast<GetElementPtrInst>(*i))
        {
            usedInLoadStore(instr2, result);
        }
        else if (isa<LoadInst>(*i) || isa<StoreInst>(*i))
        {
            //return true only if the value has uses in the body of the loop
            //if (use->getParent() != m_loop->getHeader())
            {
//                assert(m_loop->contains(use));
                result = true;
            }
        }
    }
}

void IELSection::printIELSection(void)
{
    if (m_isIELSection)
    {
        print();
        std::cerr << std::endl;
    }
}

void IELSection::generateGraphVizFile(std::fstream& file)
{
    if (m_isIELSection)
    {
        Loop* parent = m_loop;
        std::string previousLabel = m_loop->getHeader()->getName().str();
        std::string functionName = m_loop->getHeader()->getParent()->getName().str();
        file << "subgraph cluster_"  << m_id << "{\n";
        file << "label = " + functionName << std::endl;
        file << previousLabel + " [style=filled, label=\"" + previousLabel + "\"]\n";

        do
        {
            parent = parent->getParentLoop();
            if (parent != NULL)
            {
                std::string currentLabel = parent->getHeader()->getName().str();
                file << currentLabel + " [label=\"" + currentLabel + "\"]\n";
                file << currentLabel << "->" << previousLabel << std::endl;
            }

        } while (parent != NULL);

        file << "}\n";
    }
}

void IELSection::print(void)
{
    //SILParameterList& silParameters = getSILParameters();
    std::pair<int, int> range = getLineNumber(m_loop);
    
    std::cerr << "Line no: " << range.first << std::endl;
    std::cerr << "Range: " << range.first << "-" << range.second << "\nSource file: " << getSourceFile(m_loop) << std::endl;
    std::cerr << "Loop header: " << m_loop->getHeader()->getName().str() << std::endl;
    std::cerr << "Function: " << m_loop->getHeader()->getParent()->getName().str() << std::endl;
    //std::cout << "\nSil parameters: " << silParameters.size() << std::endl;

    /*
       for (SILParameterList::iterator i = silParameters.begin(); i != silParameters.end(); ++i)
       {
       SILParameter* currentParameter = *i;
       std::cout << "Value: " << *currentParameter->getValue() << "\n"
       << "Instruction: " << *currentParameter->getInstruction() << "\n"
       << "SI/L: " << MapEnum2Str[currentParameter->getSILValue()] << "\n"
       << "--------------------\n";
       }
     */
}

