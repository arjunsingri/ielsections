#include "IELSection.h"

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

std::pair<bool, bool> IELSection::isInsideOutside(SILParameter *silParameter, 
        std::vector<Value*> definitions, std::vector<BasicBlock*> definitionBlocks)
{
    bool fromInside = false;
    bool fromOutside = false;

    for (size_t j = 0; j < definitions.size(); ++j)
    {
        if (getLoop()->contains(definitionBlocks[j]))
        {
            //                    std::cout << "inside " << *instr << std::endl;
            fromInside = true;
            silParameter->addRD(definitions[j], definitionBlocks[j]);
        }
        else
        {
            //                  std::cout << "outside " << *instr << std::endl;
            fromOutside = true;
        }
    }

    //        std::cout << "\n\n";
    return std::pair<bool, bool>(fromInside, fromOutside);
}

void IELSection::printIELSection(void)
{
    if (m_isIELSection)
    {
        std::cout << "Found IE/L Section" << std::endl;
        print();
    }
}

void IELSection::print(void)
{
    SILParameterList& silParameters = getSILParameters();
    std::cout << "Loop header: " << m_loop->getHeader()->getName() << "\nSil parameters: " << silParameters.size() << std::endl;
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

