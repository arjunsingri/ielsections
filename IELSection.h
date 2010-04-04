#include "llvm/BasicBlock.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ControlDependence.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Type.h"
#include "SILParameter.h"
#include <string>
#include <iostream>

using namespace llvm;

#ifndef IELSECTION_H
#define IELSECTION_H

//FIXME: should actually be PotentialIELSection or something like that
class IELSection
{
    public:
    
        IELSection(Loop* loop, int id);
        ~IELSection();
        int getId(void) { return m_id; }

        bool isIELSection(void) { return m_isIELSection; }
        void setIELSection(bool isIELSection) { m_isIELSection = isIELSection; }
        void addBlock(BasicBlock* block) {  m_blocks.push_back(block); }
        std::vector<BasicBlock*> getBlocks() { return m_blocks; }

        void addSILParameter(SILParameter* silParameter);
        SILParameter* getSILParameter(Value* value);
        
        size_t size(void) const { return m_silParameters.size(); }

        Loop* getLoop(void) { return m_loop; }
        SILParameterList& getSILParameters(void) { return m_silParameters; }

        std::pair<bool, bool> isInsideOutside(SILParameter *silParameter, 
                std::vector<Value*> definitions, std::vector<BasicBlock*> definitionBlocks);

        void printIELSection(void);
        void print(void);

    private:
        Loop* m_loop;
        SILParameterList m_silParameters;
        std::map<Value*, SILParameter*> m_silParametersMap;
        std::vector<BasicBlock*> m_blocks;
        bool m_isIELSection;
        int m_id;
};
#endif //IELSECTION_H
