#include "IELSection.h"
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

void IELSection::generateGraphVizFile(void)
{
	if (m_isIELSection)
	{
		Loop* parent = m_loop;
		std::string previousLabel = m_loop->getHeader()->getParent()->getName().str();
		std::cout << previousLabel + " [label=\"" + previousLabel + " " + "\"]\n";

		do
		{
			parent = parent->getParentLoop();
			if (parent != NULL)
			{
				std::string currentLabel = parent->getHeader()->getParent()->getName().str();
				std::cout << currentLabel + " [label=\"" + currentLabel + " " + "\"]\n";
				std::cout << currentLabel << "->" << previousLabel << std::endl;
			}
			
		} while (parent != NULL);
	}
}

void IELSection::print(void)
{
    SILParameterList& silParameters = getSILParameters();
    std::cout << "Function: " << m_loop->getHeader()->getParent()->getName().str() << std::endl;
    std::cout << "Loop header: " << m_loop->getHeader()->getName().str() << "\nSil parameters: " << silParameters.size() << std::endl;
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

