#include "SIL.h"

using namespace llvm;

SIL::SIL()
    :   LoopPass(&ID), 
        m_id(0)
{
}

//perform step1 as per the paper and at the same time construct set RD for each triplet
void SIL::runStep1(IELSection* ielSection)
{
    SILParameterList& silParameters = ielSection->getSILParameters();

    for (SILParameterList::iterator i = silParameters.begin(); i != silParameters.end(); ++i)
    {
        SILParameter* currentParameter = *i;

        if (currentParameter->isValueArray())
        {
            assert(false);
            std::vector<Value*> arrayDefinitions = currentParameter->getArrayDefinitions();
            std::vector<BasicBlock*> arrayDefinitionBlocks = currentParameter->getArrayDefinitionBlocks();

            std::pair<bool, bool> insideOutside = ielSection->isInsideOutside(currentParameter, arrayDefinitions, arrayDefinitionBlocks);

            if (insideOutside.first && insideOutside.second)
            {
                currentParameter->setSILValue(False);
            }
            else
            {
                currentParameter->setSILValue(DontKnow);
            }
        }
        else if (!currentParameter->isValuePhiNode())
        {
            Instruction* valueAsInstruction = dyn_cast<Instruction>(currentParameter->getValue());
            assert(valueAsInstruction != NULL);

            //check if definition of v is inside the loop
            BasicBlock* parent = valueAsInstruction->getParent();
            if (ielSection->getLoop()->contains(parent))
            {
                //construct set RD for use in step 2
                currentParameter->addRD(valueAsInstruction, parent);
            }

            currentParameter->setSILValue(DontKnow);
        }
        else
        {
            PHINode* phiNode = dyn_cast<PHINode>(currentParameter->getValue());
            assert(phiNode != NULL);

            //get all definitions reaching this phi node; have to consider the cases where phi node is made up of other phi nodes

            std::vector<BasicBlock*> phiDefinitionsBlock;
            std::vector<Value*> phiDefinitions;
            std::vector<PHINode*> phiNodes;

            getPhiDefinitions(phiNode, phiDefinitionsBlock, phiDefinitions, phiNodes);
            assert(phiDefinitionsBlock.size() == phiDefinitions.size());

            std::pair<bool, bool> insideOutside = ielSection->isInsideOutside(currentParameter, phiDefinitions, phiDefinitionsBlock);
            if (insideOutside.first && insideOutside.second)
            {
                currentParameter->setSILValue(False);
            }
            else
            {
                currentParameter->setSILValue(DontKnow);
            }
        }

        /*
           bool fromInside = false;
           bool fromOutside = false;

           for (size_t j = 0; j < udDefinitionsBlock.size(); ++j)
           {
           if (ielSection->getLoop()->contains(udDefinitionsBlock[j]))
           {
           fromInside = true;
           currentParameter->addRD(udDefinitions[j], udDefinitionsBlock[j]);
           }
           else
           {
           fromOutside = true;
           }
           }

           if (fromInside && fromOutside)
           {
           currentParameter->setSILValue(False);
           }
           else
           {
           currentParameter->setSILValue(DontKnow);
           }
         */
    }
}

void SIL::getPhiDefinitions(PHINode* phiNode, std::vector<BasicBlock*>& udChainBlock, std::vector<Value*>& udChainInst, std::vector<PHINode*> phiNodes)
{
    //std::cout << "enter" << std::endl;
    phiNodes.push_back(phiNode);
    unsigned int incomingSize = phiNode->getNumIncomingValues();
    for (unsigned int i = 0; i < incomingSize; ++i)
    {
        BasicBlock* block = phiNode->getIncomingBlock(i);
        Value* value = phiNode->getIncomingValue(i);
        if (isa<UndefValue>(value))
        {
            continue;
        }

        assert(!isa<BasicBlock>(value));

        if (PHINode* valueAsPhiNode = dyn_cast<PHINode>(value))
        {
            std::vector<PHINode*>::iterator where = std::find(phiNodes.begin(), phiNodes.end(), valueAsPhiNode);
            if (where == phiNodes.end())
            {
                //std::cout << *valueAsPhiNode << std::endl;
                getPhiDefinitions(valueAsPhiNode, udChainBlock, udChainInst, phiNodes);
            }
            else
            {
                //std::cout << "skipping" << std::endl;
            }
        }
        else
        {
            udChainBlock.push_back(block);
            udChainInst.push_back(value);
        }
    }
    //std::cout << "leave" << std::endl;
}

void SIL::computeCP(IELSection* ielSection, SILParameter* silParameter, ControlDependence& cd)
{
    assert(silParameter->getLoop() == ielSection->getLoop());
    Loop* loop = silParameter->getLoop();
    const std::vector<Instruction*>& deps = 
        cd.getControlDependenceInstructions(silParameter->getInstruction());

    for (std::vector<Instruction*>::const_iterator j = deps.begin(); j != deps.end(); ++j)
    {
        BasicBlock* instructionParent = (*j)->getParent();

        if (loop->contains(instructionParent) && loop->getHeader() != instructionParent)
        {
            silParameter->addCP(*j);
        }
    }
}

//return true if the SI/L value for this parameter changes from DontKnow to False
bool SIL::recomputeSILValue(SILParameter* silParameter, IELSection* ielSection)
{
    const std::vector<Value*>& rd = silParameter->getRD();
    for (std::vector<Value*>::const_iterator i = rd.begin(); i != rd.end(); ++i)
    {
        Instruction* inst;
        if ((inst = dyn_cast<Instruction>(*i)) == NULL)
        {
            continue;
        }

        for (User::op_iterator j = inst->op_begin(); j != inst->op_end(); ++j)
        {
            if (isa<Constant>(*j) || !isa<Instruction>(*j))
                continue;

            //this can never return NULL as all *j that are considered here are inside the loop 
            //and we have constructed an SILParameter object for all variables used inside the loop
            SILParameter* p = ielSection->getSILParameter(*j);
            /*
               if (p == NULL)
               { 
               std::cout << "error: " << *inst << std::endl; 
               std::cout << *(j->get()) << std::endl; 
               std::cout << *silParameter->getInstruction() << std::endl;
               }
             */  
            assert(p != NULL);

            if (p->getSILValue() == False)
            {
                silParameter->setSILValue(False);
                return true;
            }
        }
    }

    const std::vector<Instruction*>& cp = silParameter->getCP();
    for (std::vector<Instruction*>::const_iterator i = cp.begin(); i != cp.end(); ++i)
    {
        for (User::op_iterator j = (*i)->op_begin(); j != (*i)->op_end(); ++j)
        {
            if (isa<Constant>(*j))
                continue;

            //this can never return NULL as all *j that are considered here are inside the loop
            //and we have constructed an SILParameter object for all variables used inside the loop
            SILParameter* p = ielSection->getSILParameter(*j);
            assert(p != NULL);

            if (p->getSILValue() == False)
            {
                silParameter->setSILValue(False);
                return true;
            }
        }
    }

    return false;
}

void SIL::runStep2(IELSection* ielSection)
{
    ControlDependence& cd = getAnalysis<ControlDependence>();

    SILParameterList& silParameters = ielSection->getSILParameters();
    int changed = 0;
    int iteration = 0;

    do
    {
        changed = 0;

        for (SILParameterList::iterator i = silParameters.begin(); i != silParameters.end(); ++i)
        {
            SILParameter* currentParameter = *i;

            if (currentParameter->getSILValue() == DontKnow)
            {
                //TODO: can be made faster by storing the control dependences on an instruction basis than storing it in SILParameter
                //otherwise we will end up computing CPs multiple times

                computeCP(ielSection, currentParameter, cd);
                if (recomputeSILValue(currentParameter, ielSection))
                {
                    ++changed;
                }
            }
        }

        //std::cout << "iteration: " << iteration << " changed: " << changed << std::endl;
        ++iteration;

    } while (changed != 0);

    std::cout << std::endl;
}

void SIL::runStep3(IELSection* ielSection)
{
    SILParameterList& silParameters = ielSection->getSILParameters();

    for (SILParameterList::iterator i = silParameters.begin(); i != silParameters.end(); ++i)
    {
        if ((*i)->getSILValue() == DontKnow)
        {
            (*i)->setSILValue(True);
        }
    }
}

//TODO: find suitable name
//return NULL if this loop's body is not considered an IE/L section
//the plan is to ignore all loops which call functions
IELSection* SIL::addIELSection(Loop* loop)
{
    assert (loop->getHeader() == *loop->block_begin() && "First node is not the header!");

    IELSection* currentIELSection = new IELSection(loop, getId());
    std::map<Value*, int> opcount;

    //skip the header of the loop
    for (LoopBase<BasicBlock>::block_iterator block = loop->block_begin() + 1; block != loop->block_end(); ++block)
    {
        //std::cout << **block << std::endl;
        currentIELSection->addBlock(*block);
        for (BasicBlock::iterator instr = (*block)->begin(); instr != (*block)->end(); ++instr)
        {
            if (isa<PHINode>(instr)) //don't analyze the phi instruction by itself unless it is used in another instruction
            {
                continue;
            }

            if (StoreInst* storeInst = dyn_cast<StoreInst>(instr))
            {
                Value* storeOperand = storeInst->getPointerOperand();
                BasicBlock::iterator prev = instr;
                --prev;
                if (GetElementPtrInst* getElementPtrInst = dyn_cast<GetElementPtrInst>(prev))
                {
                    const Type* elementType = getElementPtrInst->getPointerOperandType()->getElementType();
                    if (elementType->getTypeID() == Type::ArrayTyID)
                    {
                        if (storeOperand == getElementPtrInst)
                        {
                            //std::cout << "array store found! discarding..." << std::endl;
                            //std::cout << *getElementPtrInst << std::endl;
                            delete currentIELSection;
                            return NULL;
                        }
                    }
                }
            }
            /*
               if (GetElementPtrInst* getElementPtrInst = dyn_cast<GetElementPtrInst>(instr))
               {
               Value* operand = getElementPtrInst->getPointerOperand();
               const Type* elementType = getElementPtrInst->getPointerOperandType()->getElementType();

               if (elementType->getTypeID() == Type::ArrayTyID)
               {
               BasicBlock::iterator next = instr;
               if (StoreInst* storeInst = dyn_cast<StoreInst>(++next))
               {
               Value* storeOperand = storeInst->getPointerOperand();
               if (storeOperand == getElementPtrInst)
               {
               m_toArray[storeInst] = operand;
               m_arrayDefinitions[operand].push_back(storeInst);
               m_arrayDefinitionsBlocks[operand].push_back(storeInst->getParent());
               std::cout << "store op: " << *operand << std::endl;
               }
               }
               else if (LoadInst* loadInst = dyn_cast<LoadInst>(next))
               {
               Value* loadOperand = loadInst->getPointerOperand();
               if (loadOperand == getElementPtrInst)
               {
               m_toArray[loadInst] = operand;
               }
               }
               else
               {
               std::cout << *instr << std::endl << *next << std::endl;
               assert(0 && "no load or store!");
               }
               }
               }
             */
            for (User::op_iterator def = instr->op_begin(); def != instr->op_end(); ++def)
            {
                Value* v = def->get();
                if (isa<BasicBlock>(v))
                {
                    continue;
                }

                if (isa<Constant>(v))
                {
                    //ignore all loops containing function calls as we don't how to handle them
                    if (isa<Function>(v))
                    {
                        //std::cerr << "Can't handle function calls: " << loop->getHeader()->getName() << std::endl;
                        delete currentIELSection;
                        return NULL;
                    }

                    continue;
                }

                if (!isa<Instruction>(v))
                {
                    //std::cout << "not inst: " << *v << std::endl;
                    continue;
                }

                if (isa<GetElementPtrInst>(v))
                {
                    continue;
                }

                SILParameter* silParameter = new SILParameter(loop, v, instr);

                if (isa<PHINode>(v))
                {
                    silParameter->setValuePhiNode(true);
                }

                std::map<Value*, Value*>::iterator where = m_toArray.find(v);
                if (where != m_toArray.end())
                {
                    silParameter->setArrayDefinitions(m_arrayDefinitions[where->second], m_arrayDefinitionsBlocks[where->second]);
                    silParameter->setValueArray(true);
                }

                currentIELSection->addSILParameter(silParameter);
            }
        }
    }

    return currentIELSection;
}

bool SIL::checkIELSection(IELSection* ielSection)
{
    Loop* loop = ielSection->getLoop();
    bool isIELSection = true;
    std::vector<BasicBlock*> blocks = ielSection->getBlocks();
    //std::cout << "----Loop header: " << loop->getHeader()->getName() << " blocks: " << blocks.size() << std::endl;

    for (std::vector<BasicBlock*>::iterator block = blocks.begin(); block != blocks.end()/* && isIELSection*/; ++block)
    {
        for (BasicBlock::iterator instr = (*block)->begin(); instr != (*block)->end()/* && isIELSection*/; ++instr)
        {
            if (GetElementPtrInst* getElementPtrInst = dyn_cast<GetElementPtrInst>(instr))
            {
                const Type* elementType = getElementPtrInst->getPointerOperandType()->getElementType();

                if (elementType->getTypeID() == Type::ArrayTyID)
                {
                    //std::cout << *getElementPtrInst << std::endl;
                    for (User::op_iterator index = getElementPtrInst->idx_begin(); index != getElementPtrInst->idx_end(); ++index)
                    {
                        if (!isa<Constant>(*index))
                        {
                            //std::cout << "\t" << *(index->get()) << std::endl;
                            SILParameter* parameter = ielSection->getSILParameter(*index);
                            if (parameter->getSILValue() == False)
                            {
                                isIELSection = false;
                                //parameter->printSILValue();
                            }
                        }
                    }
                }
                else
                {
                    //std::cout << *elementType << std::endl;
                }
            }
            else if (BranchInst* branchInst = dyn_cast<BranchInst>(instr))
            {
                if (branchInst->isConditional())
                {
                    //std::cout << *branchInst << std::endl;
                    Value* condition = branchInst->getCondition();
                    //std::cout << *condition << std::endl;
                    SILParameter* parameter = ielSection->getSILParameter(condition);
                    if (parameter->getSILValue() == False)
                    {
                        isIELSection = false;
                        //parameter->printSILValue();
                    }
                }
            }
        }
    }

    //std::cout << "----Loop header: " << loop->getHeader()->getName() << " " << isIELSection << std::endl;
    return isIELSection;
}

bool SIL::runOnLoop(Loop* loop, LPPassManager &lpm)
{
    if (IELSection* ielSection = addIELSection(loop))
    {
        runStep1(ielSection);
        runStep2(ielSection);
        runStep3(ielSection);

        ielSection->setIELSection(checkIELSection(ielSection));
        ielSection->printIELSection();
    }

    //dump();

    return false;
}

void SIL::dump(void)
{
    for (std::vector<IELSection*>::iterator i = m_ielSections.begin(); i != m_ielSections.end(); ++i)
    {
        Loop* loop = (*i)->getLoop();
        std::cout << "Loop header: " << loop->getHeader()->getName() << "\n\n";
        SILParameterList& parList = (*i)->getSILParameters();

        for (SILParameterList::iterator j = parList.begin(); j != parList.end(); ++j)
        {
            SILParameter* par = *j;
            std::cout << "\t" << "Value:       " << *(par->getValue()) << "\n\tInstruction: " << *(par->getInstruction()) << std::endl;
            std::cout << "\t------------\n";
        }
    }
}

void SIL::getAnalysisUsage(AnalysisUsage& AU) const
{
    AU.setPreservesAll();
    AU.addRequired<LoopInfo>();
    AU.addRequired<ControlDependence>();
}

char SIL::ID = 0;
RegisterPass<SIL> sil("iel", "find all IE/L sections");
