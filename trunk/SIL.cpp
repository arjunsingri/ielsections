#include "SIL.h"
#include "llvm/Analysis/DebugInfo.h"

using namespace llvm;

char SIL::ID = 0;
static RegisterPass<SIL> sil("iel", "find all IE/L sections");

SIL::SIL()
    :   //LoopPass(&ID),
        FunctionPass(&ID),
        m_currentReachingDef(NULL),
        m_id(0)
{
    std::string filename = "/home/singri/llvm-2.7/llvm/lib/Analysis/ielsections/Untitled1";
    m_file.open(filename.c_str(), std::fstream::out);
    m_file << "digraph{\n";
}

SIL::~SIL()
{
    m_file << "\n}";
    m_file.close();
}

//perform step1 as per the paper and at the same time construct set RD for each triplet
void SIL::runStep1(IELSection* ielSection)
{
//    std::cout << "step 1" << std::endl;
    assert(ielSection != NULL);
    SILParameterList& silParameters = ielSection->getSILParameters();

    for (SILParameterList::iterator i = silParameters.begin(); i != silParameters.end(); ++i)
    {
        SILParameter* currentParameter = *i;
        currentParameter->constructDefinitionList(m_currentReachingDef);
       
        bool isInside = false, isOutside = false;
 
        for (unsigned int j = 0; j < currentParameter->getNumDefinitions(); ++j)
        {
            Loop* loop = ielSection->getLoop();
            BasicBlock* parent = currentParameter->getDefinitionParent(j);

            if (loop->contains(parent) && loop->getHeader() != parent)
            {
                currentParameter->addRD(j);
                isInside = true;
            }
            else
            {
                isOutside = true;
            }
        }

        if (isInside && isOutside)
        {
            currentParameter->setSILValue(False);
        }
        else
        {
            currentParameter->setSILValue(DontKnow);
        }
    }
}

void SIL::computeCP(IELSection* ielSection, SILParameter* silParameter, ControlDependence& cd)
{
    assert(silParameter->getLoop() == ielSection->getLoop());

    Loop* loop = silParameter->getLoop();
    const std::vector<Instruction*>& deps = cd.getControlDependenceInstructions(silParameter->getInstruction());

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
    const std::vector<unsigned int>& rd = silParameter->getRD();
    for (unsigned int i = 0; i < rd.size(); ++i)
    {
        Instruction* inst;
        if ((inst = dyn_cast<Instruction>(silParameter->getDefinition(rd[i]))) == NULL)
        {
            continue;
        }

        assert(ielSection->getLoop()->contains(silParameter->getDefinitionParent(rd[i])));

        for (User::op_iterator j = inst->op_begin(); j != inst->op_end(); ++j)
        {
            if (isa<Constant>(*j) || !isa<Instruction>(*j)) continue;

            //this can never return NULL as all *j that are considered here are inside the loop 
            //and we have constructed an SILParameter object for all variables used inside the loop
            SILParameter* p = ielSection->getSILParameter(*j);

            if (p == NULL)
            { 
                //std::cout << "error: " << *inst << std::endl;
		std::cout << "error: ";
		inst->dump();
		std::cout << std::endl;
                //std::cout << *(j->get()) << std::endl; 
                j->get()->dump(); 
                //std::cout << *silParameter->getInstruction() << std::endl;
                silParameter->getInstruction()->dump();
                //std::cout << *silParameter->getValue() << std::endl;
                silParameter->getValue()->dump();
            }

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
//    std::cout << "step 2\n";
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

//    std::cout << std::endl;
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
    for (LoopBase<BasicBlock, Loop>::block_iterator block = loop->block_begin() + 1; block != loop->block_end(); ++block)
    {
        //std::cout << **block << std::endl;
        currentIELSection->addBlock(*block);
        for (BasicBlock::iterator instr = (*block)->begin(); instr != (*block)->end(); ++instr)
        {
            if (isa<PHINode>(instr)) //don't analyze the phi instruction by itself unless it is used in another instruction
            {
                continue;
            }

            for (User::op_iterator def = instr->op_begin(); def != instr->op_end(); ++def)
            {
                Value* v = def->get();
                if (isa<BasicBlock>(v) || !isa<Instruction>(v))
                {
                    continue;
                }
                else if (isa<Constant>(v))
                {
                    //ignore all loops containing function calls as we don't how to handle them
                    if (isa<Function>(v))
                    {
                        //std::cerr << "Can't handle function calls: " << loop->getHeader()->getName() << std::endl;
                        //delete currentIELSection;
                        //return NULL;
                    }

                    continue;
                }

                SILParameter* silParameter = new SILParameter(loop, v, instr);
                currentIELSection->addSILParameter(silParameter);
            }
        }
    }

    return currentIELSection;
}

#if 0
void decompile(IELSection* ielSection)
{
    std::vector<BasicBlock*> blocks = ielSection->getBlocks();

    BasicBlock* header = ielSection->getLoop()->getHeader();

    std::cout << "<begin> Function: " << header->getParent()->getName().str() << " header: " << header->getName().str() << std::endl;
    for (std::vector<BasicBlock*>::iterator block = blocks.begin(); block != blocks.end(); ++block)
    {
        for (BasicBlock::iterator instr = (*block)->begin(); instr != (*block)->end(); ++instr)
        {
            if (LoadInst *loadInst = dyn_cast<LoadInst>(instr))
            {
                Value* coreOperand;
                std::set<Value*> indices = ReachingDef::findCoreOperand(loadInst->getPointerOperand(), &coreOperand);
                std::cout << "array: " << *coreOperand << std::endl;
                for (std::set<Value*>::iterator index = indices.begin(); index != indices.end(); ++index)
                {
                    if (isa<Instruction>(*index))
                    {
                        std::cout << "\t" << (**index).getName().str() << std::endl;
                    }
                    else
                    {
                        std::cout << "\t" << **index << std::endl;
                    }
                }
            }
            else if (BranchInst* branchInst = dyn_cast<BranchInst>(instr))
            {
                if (branchInst->isConditional())
                {
                    if (Instruction* condition = dyn_cast<Instruction>(branchInst->getCondition()))
                    {
                        std::cout << "compare: ";
                        for (User::op_iterator j = condition->op_begin(); j != condition->op_end(); ++j)
                        {
                            if (isa<Instruction>(*j))
                            {
                                std::cout << "\t" << (**j).getName().str() << std::endl;
                            }
                            else
                            {
                                std::cout << "\t" << **j << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }

    std::cout << "<end>\n";
}
#endif
void SIL::isUsedInLoadStore(GetElementPtrInst* instr, bool &result)
{
    for (Value::use_iterator i = instr->use_begin(); i != instr->use_end(); ++i)
    {
        if (GetElementPtrInst* instr2 = dyn_cast<GetElementPtrInst>(*i))
        {
            isUsedInLoadStore(instr2, result);
        }
        else if (isa<LoadInst>(*i)) result = true;
        else if (isa<StoreInst>(*i)) result = true;
    }
}

bool SIL::checkIELSection(IELSection* ielSection)
{
    //Loop* loop = ielSection->getLoop();
    bool isIELSection = true;
    std::vector<BasicBlock*> blocks = ielSection->getBlocks();
    //std::cout << "----Loop header: " << loop->getHeader()->getName() << " blocks: " << blocks.size() << std::endl;

    for (std::vector<BasicBlock*>::iterator block = blocks.begin(); block != blocks.end()/* && isIELSection*/; ++block)
    {
        for (BasicBlock::iterator instr = (*block)->begin(); instr != (*block)->end()/* && isIELSection*/; ++instr)
        {
            if (LoadInst *loadInst = dyn_cast<LoadInst>(instr))
            {
                //std::cout << *loadInst << std::endl;
		//loadInst->dump();

                Value* coreOperand;
                std::set<Value*> indices = ReachingDef::findCoreOperand(loadInst->getPointerOperand(), &coreOperand);

                for (std::set<Value*>::iterator index = indices.begin(); index != indices.end(); ++index)
                {
                    if (isa<Constant>(*index) || !isa<Instruction>(*index)) continue;
                    
                    SILParameter* parameter = ielSection->getSILParameter(*index);
                    assert(parameter != NULL);

                    if (parameter->getSILValue() == False)
                    {
                        isIELSection = false;
                        //std::cout << parameter->getValue()->getName().str() << std::endl;
                        //parameter->printSILValue();
                    }
                }
            }
            else if (BranchInst* branchInst = dyn_cast<BranchInst>(instr))
            {
                if (branchInst->isConditional())
                {
                    //std::cout << *branchInst << std::endl;
                    if (Instruction* condition = dyn_cast<Instruction>(branchInst->getCondition()))
                    {
                        //std::cout << "\t" << *condition << std::endl;
                        for (User::op_iterator j = condition->op_begin(); j != condition->op_end(); ++j)
                        {
                            if (isa<Constant>(*j) || !isa<Instruction>(*j)) continue;

                            SILParameter* parameter = ielSection->getSILParameter(*j);
                            assert(parameter != NULL);
                            if (parameter->getSILValue() == False)
                            {
                                isIELSection = false;
                            }
                        }
                    }
                    else
                    {
                        //std::cout << "condition " << *branchInst->getCondition() << " is not an instruction\n";
			std::cout << "condition ";
			branchInst->getCondition()->dump();
			std::cout << " is not an instruction\n";
                        assert(false);
                    }
                }
            }
            else if (GetElementPtrInst *getElementPtrInst = dyn_cast<GetElementPtrInst>(instr))
            {
                bool result;
                isUsedInLoadStore(getElementPtrInst, result);
                if (!result)
                {
                    //std::cout << *instr << std::endl;
                    SILParameter* parameter = ielSection->getSILParameter(instr);
                    if (parameter->getSILValue() == False)
                    {
                        //std::cout << *parameter->getInstruction() << std::endl;
                        isIELSection = false;
                    }
                }
            }
        }
    }

    //std::cout << "----Loop header: " << loop->getHeader()->getName() << " " << isIELSection << std::endl;
    return isIELSection;
}

//bool SIL::runOnLoop(Loop* loop, LPPassManager &lpm)
bool SIL::runOnFunction(Function& function)
{
    m_currentReachingDef = &getAnalysis<ReachingDef>();
    LoopInfo& loopInfo = getAnalysis<LoopInfo>();
    std::map<Loop*, bool> visited;

    std::string functionName = function.getName().str();
    m_file << "subgraph cluster_" << functionName << "{\n";
    m_file << "label = " << functionName << std::endl;

    static int id = 0;
    ++id;
    char buffer[10];
    sprintf(buffer, "_%d", id);

    for (Function::iterator i = function.begin(); i != function.end(); ++i)
    {
        Loop* loop = loopInfo.getLoopFor(i);
        if (loop == NULL) continue;
        if (visited.find(loop) != visited.end()) continue;
        visited[loop] = true;

        assert(m_currentReachingDef->getCurrentFunction() == loop->getHeader()->getParent());
        m_loops.push_back(loop);

        if (IELSection* ielSection = addIELSection(loop))
        {
            runStep1(ielSection);
            runStep2(ielSection);
            runStep3(ielSection);

            ielSection->setIELSection(checkIELSection(ielSection));
            if (ielSection->isIELSection())
            {
                m_ielSections.push_back(ielSection);
            }

            ielSection->printIELSection();
            if (ielSection->isIELSection())
            {
                printNode(loop, buffer, true);
            }
        }

	printAdjacentLoops(loop, loopInfo, buffer);
    }

    m_file << "}\n";

    return false;
}

void SIL::printNode(Loop* loop, char* id, bool isFilled)
{
    int lineNumber = getLineNumber(loop);
    assert(lineNumber != -1);

    const std::string& name = loop->getHeader()->getName().str() + id;
    const std::string& label = loop->getHeader()->getName().str();

    m_file << name;
    
    if (isFilled)
    {
        m_file << " [style=filled, label=\"" << label + ": ";
    }
    else
    {
        m_file << " [label=\"" << label + ": ";
    }

    m_file << lineNumber;
    m_file << "\"]\n";

}

void SIL::printEdge(Loop* srcLoop, Loop* dstLoop, char* id)
{
    const std::string& src = srcLoop->getHeader()->getName().str() + id;
    const std::string& dst = dstLoop->getHeader()->getName().str() + id;
    m_file << src + "->" + dst + ";\n";
}

int SIL::getLineNumber(Loop* loop)
{
    int lineNumber = -1;
    if (MDNode* node = loop->getHeader()->getFirstNonPHI()->getMetadata("dbg"))
    {
        DILocation loc(node);
        lineNumber = loc.getLineNumber();
    }

    return lineNumber;
}

void SIL::printAdjacentLoops(Loop* srcLoop, LoopInfo& loopInfo, char* id)
{
    std::map<BasicBlock*, bool> visited;
    std::queue<BasicBlock*> blockQueue;

    blockQueue.push(srcLoop->getHeader());
    printNode(srcLoop, id, false);

    while (!blockQueue.empty())
    {
        BasicBlock* block = blockQueue.front();
        blockQueue.pop();

        visited[block] = true;
        for (succ_iterator succ = succ_begin(block); succ != succ_end(block); ++succ)
        {
            if (visited.find(*succ) == visited.end())
            {
                Loop* dstLoop = loopInfo.getLoopFor(*succ);

                if (dstLoop != NULL && dstLoop != srcLoop && !isAncestor(dstLoop, srcLoop))
                {
                    if (m_loopGraph.find(srcLoop) == m_loopGraph.end()
                            || m_loopGraph[srcLoop].find(dstLoop) == m_loopGraph[srcLoop].end())
                    {
                        printNode(dstLoop, id, false);
                        printEdge(srcLoop, dstLoop, id);
                        m_loopGraph[srcLoop].insert(dstLoop);
                    }
                }

                if (dstLoop == NULL || dstLoop == srcLoop)
                {
                    blockQueue.push(*succ);
                }
            }
        }
    }
}

bool SIL::isAncestor(Loop* dstLoop, Loop* srcLoop)
{
    Loop* parent = srcLoop;

    do
    {
        parent = parent->getParentLoop();
        
        if (parent == dstLoop) return true;

    } while (parent != NULL);

    return false;
}

void SIL::dump(void)
{
    for (std::vector<IELSection*>::iterator i = m_ielSections.begin(); i != m_ielSections.end(); ++i)
    {
        Loop* loop = (*i)->getLoop();
        std::cout << "Loop header: " << loop->getHeader()->getName().str() << "\n\n";
        
        SILParameterList& parList = (*i)->getSILParameters();

        for (SILParameterList::iterator j = parList.begin(); j != parList.end(); ++j)
        {
            SILParameter* par = *j;
            std::cout << "\t" << "Value:       ";
	    (par->getValue())->dump();
	    //std::cout << "\n\tInstruction: " << *(par->getInstruction()) << std::endl;
            std::cout << "\t------------\n";
        }
    }
}

void SIL::getAnalysisUsage(AnalysisUsage& AU) const
{
    AU.setPreservesAll();
    AU.addRequired<ReachingDef>();
    AU.addRequired<ControlDependence>();
    AU.addRequired<LoopInfo>();
}

