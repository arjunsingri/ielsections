#include "IELSection.h"
#include "llvm/Analysis/DebugInfo.h"
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

void IELSection::printIELSection(void)
{
    if (m_isIELSection)
    {
        print();
        std::cout << std::endl;
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
    SILParameterList& silParameters = getSILParameters();
    std::cout << "Function: " << m_loop->getHeader()->getParent()->getName().str() << std::endl;
    std::cout << "Loop header: " << m_loop->getHeader()->getName().str() << "\nSil parameters: " << silParameters.size() << std::endl;
    if (MDNode* node = m_loop->getHeader()->getFirstNonPHI()->getMetadata("dbg"))
    {
        DILocation loc(node);
        std::cout << "Source file: " << loc.getFilename().str() << " line: " << loc.getLineNumber() << std::endl;
    }
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

