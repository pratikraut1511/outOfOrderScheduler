/*
 * pipeline.cpp
 *
 *  Created on: 04-Dec-2019
 *      Author: prati
 */

#include "pipeline.h"

static unsigned int operationCount = 0;
//system cycle
static unsigned int cycle = 0;
//index for fake rob
static unsigned int fakeRobIndex = 0;
//index for renaming
static unsigned int bufferIndex = 0;

struct StageRegComparator
{
    // Compare 2 Player objects using name
    bool operator ()(const stageReg & stageReg1, const stageReg & stageReg2)
    {
        if (stageReg2.tagVal > stageReg1.tagVal)
            return stageReg2.tagVal < stageReg1.tagVal;
        return stageReg2.tagVal > stageReg1.tagVal;
    }
};

pipeline::pipeline(unsigned int scheduleSize, unsigned int fetchSize)
{
    pFileHandler = NULL;
    fakeRobSize = 1024;
    numOfInstructions = fetchSize;
    scheduleQueueCapacity = scheduleSize;
    dispatchQueueSize = fetchSize * 2;
    executionQueueSize = fetchSize * 5;
    issueQueueCounter = 0;
    executionQueueCount = 0;
    renameMapTable = new renameMapTable_t[67];
    queueSpace = 0;
    for (unsigned int i = 0; i < 67; i++)
    {
        renameMapTable->isValid = false;
        renameMapTable->tagVal = 0;
    }

    issueQueue = new stageReg[scheduleQueueCapacity];

    for (unsigned int i = 0; i < scheduleQueueCapacity; i++)
    {
        issueQueue[i] =
        {   0};
    }

    fakeRob = new stageReg[fakeRobSize];
    fakeRobHead = 0;

    dispatchQueue = createQueue(2 * numOfInstructions);
    dispatchIssueList = new stageReg[numOfInstructions];
    issueExecutionList = new stageReg[numOfInstructions];
    executionQueue = new stageReg[executionQueueSize];
    retireList = new stageReg[executionQueueSize * 2];
    isL1Present = false;
    cacheL1 = NULL;
    isL2Present = false;
    cacheL2 = NULL;
}

pipeline::~pipeline()
{
    delete retireList;
    delete executionQueue;
    delete issueExecutionList;
    delete dispatchIssueList;
    delete issueQueue;
    delete dispatchQueue;
    delete fakeRob;
    delete renameMapTable;
}

bool pipeline::isFakeRobEmpty()
{
    bool retVal = true;

    for (unsigned int i = 0; i < fakeRobSize; i++)
    {
        if (fakeRob[i].isValid == true)
        {
            retVal = false;
            break;
        }
    }

    return retVal;
}

bool pipeline::Advance_cycle()
{
    bool retval = false;
    //increment global count variable
    cycle++;
    // if (feof(pFileHandler) && !issueQueueCounter && !executionQueueSize && !dispatchQueue->size())
    if (feof(pFileHandler) && isFakeRobEmpty())
    {
        retval = true;
    }
    //cout << "retVal ::" << retval << endl;
    return retval;
}

void pipeline::swap(stageReg *val1, stageReg *val2)
{
    stageReg temp = { 0 };
    if (val1->isValid && val2->isValid)
    {
        //if(val1->isValid && !)
        temp.isValid = val1->isValid;
        temp.tagVal = val1->tagVal;
        temp.progCount = val1->progCount;
        temp.dest_reg = val1->dest_reg;
        temp.dest_ready = val1->dest_ready;
        temp.src1_reg = val1->src1_reg;
        temp.src1_ready = val1->src1_reg;
        temp.src2_reg = val1->src2_reg;
        temp.src2_ready = val1->src2_ready;
        temp.operationType = val1->operationType;
        temp.latency = val1->latency;
        temp.duration = val1->duration;
        temp.presentStage = val1->presentStage;
        temp.fetchArrival = val1->fetchArrival;
        temp.fetchTime = val1->fetchTime;
        temp.decodeArrival = val1->decodeArrival;
        temp.decodeTime = val1->decodeTime;
        temp.issueArrival = val1->issueArrival;
        temp.issueTime = val1->issueTime;
        temp.executionArrival = val1->executionArrival;
        temp.executionTime = val1->executionTime;
        temp.writeBackArrival = val1->writeBackArrival;
        temp.writeBackTime = val1->writeBackTime;

        *val1 = *val2;

        val2->isValid = temp.isValid;
        val2->tagVal = temp.tagVal;
        val2->progCount = temp.progCount;
        val2->dest_reg = temp.dest_reg;
        val2->dest_ready = temp.dest_ready;
        val2->src1_reg = temp.src1_reg;
        val2->src1_ready = temp.src1_ready;
        val2->src2_reg = temp.src2_reg;
        val2->src2_ready = temp.src2_ready;
        val2->operationType = temp.operationType;
        val2->latency = temp.latency;
        val2->duration = temp.duration;
        val2->presentStage = temp.presentStage;
        val2->fetchArrival = temp.fetchArrival;
        val2->fetchTime = temp.fetchTime;
        val2->decodeArrival = temp.decodeArrival;
        val2->decodeTime = temp.decodeTime;
        val2->issueArrival = temp.issueArrival;
        val2->issueTime = temp.issueTime;
        val2->executionArrival = temp.executionArrival;
        val2->executionTime = temp.executionTime;
        val2->writeBackArrival = temp.writeBackArrival;
        val2->writeBackTime = temp.writeBackTime;

    }
}

void pipeline::stageRegSort(stageReg *issueList)
{

    for (unsigned int i = 0; i < numOfInstructions; i++)
    {
        for (unsigned int j = 0; j < numOfInstructions - i - 1; j++)
        {
            stageReg temp = { 0 };
            if (issueList[j].tagVal > issueList[j + 1].tagVal)
            {
                //swap(&issueList[j], &issueList[j + 1]);
                temp = issueList[j];
                issueList[j] = issueList[j + 1];
                issueList[j + 1] = temp;
            }
        }
    }
}

void pipeline::issueQueueSort(stageReg *issueList)
{
    for (unsigned int i = 0; i < scheduleQueueCapacity; i++)
    {
        for (unsigned int j = 0; j < scheduleQueueCapacity - i - 1; j++)
        {
            if (issueList[j].tagVal > issueList[j + 1].tagVal)
            {
                stageReg temp;
                //swap(&issueList[j], &issueList[j + 1]);
                temp = issueList[j];
                issueList[j] = issueList[j + 1];
                issueList[j + 1] = temp;
            }
        }
    }
}

void pipeline::writeBack(stageReg *writeBackVal)
{
    for (unsigned int i = 0; i < scheduleQueueCapacity; i++)
    {
        if (writeBackVal->dest_reg == issueQueue[i].src1_reg)
        {
            issueQueue[i].src1_ready = true;
            switch (issueQueue[i].operationType)
            {
                case 0:
                    issueQueue[i].latency = 1;
                    issueQueue[i].duration = 1;
                    break;
                case 1:
                    issueQueue[i].latency = 2;
                    issueQueue[i].duration = 2;
                    break;
                case 2:
                    issueQueue[i].latency = 5;
                    issueQueue[i].duration = 5;
                    break;
            }
        }
        if (writeBackVal->dest_reg == issueQueue[i].src2_reg)
        {
            issueQueue[i].src2_ready = true;
            switch (issueQueue[i].operationType)
            {
                case 0:
                    issueQueue[i].latency = 1;
                    issueQueue[i].duration = 1;
                    break;
                case 1:
                    issueQueue[i].latency = 2;
                    issueQueue[i].duration = 2;
                    break;
                case 2:
                    issueQueue[i].latency = 5;
                    issueQueue[i].duration = 5;
                    break;
            }
        }
    }
#if DEBUG
    cout << "***writeBack ***  " << writeBackVal->dest_reg << endl;
#endif
    for (unsigned int i = 0; i < 67; i++)
    {
        if (writeBackVal->dest_reg == renameMapTable[i].tagVal)
        {
            renameMapTable[i].isValid = false;
            break;
        }
    }
}

void pipeline::retire()
{
    stageReg temp = { 0 };
#if DEBUG
    cout << "*** retrie ****  " << cycle << endl;
#endif
    for (unsigned int i = 0; i < executionQueueSize; i++)
    {
        if (retireList[i].isValid)
        {
            for (unsigned int j = 0; j < fakeRobSize; j++)
            {
                if (retireList[i].tagVal == fakeRob[j].tagVal
                        && fakeRob[j].isValid)
                {
                    retireList[i].dest_reg = fakeRob[j].dest_reg;
                    retireList[i].src1_reg = fakeRob[j].src1_reg;
                    retireList[i].src2_reg = fakeRob[j].src2_reg;
                    fakeRob[j] = retireList[i];
#if DEBUG
                    cout << retireList[i].tagVal << endl;
#endif
                    //temp = retireList[i];
                    retireList[i] =
                    {   0};
                    break;
                }
            }
        }
    }
    while (fakeRob[fakeRobHead].presentStage == WB)
    {
        temp = fakeRob[fakeRobHead];
        cout << temp.tagVal << " " << "fu{" << temp.operationType << "} "
                << "src{" << temp.src1_reg << "," << temp.src2_reg << "} "
                << "dst{" << temp.dest_reg << "} " << "IF{" << temp.fetchArrival
                << "," << temp.fetchTime << "} ID{" << temp.decodeArrival << ","
                << temp.decodeTime << "} IS{" << temp.issueArrival << ","
                << temp.issueTime << "} EX{" << temp.executionArrival << ","
                << temp.executionTime << "} WB{" << temp.writeBackArrival << ","
                << temp.writeBackTime << "}" << endl;
        fakeRob[fakeRobHead] =
        {   0};
        fakeRobHead++;
        if (fakeRobHead == fakeRobSize)
            fakeRobHead = 0;
    }
}

void pipeline::executeCycle()
{
    stageReg temp = { 0 };
#if DEBUG
    cout << "**** execute cycle **** " << cycle << endl;
    cout << "**** Execution Queue ****" << endl;
    for (unsigned int i = 0; i < executionQueueSize; i++)
    {
        if (executionQueue[i].isValid)
        {
            temp = executionQueue[i];

            cout << i << "  " << temp.tagVal << " " << "fu{"
            << temp.operationType << "} " << "src{" << temp.src1_reg
            << "," << temp.src2_reg << "} " << "dst{" << temp.dest_reg
            << "} " << "IF{" << temp.fetchArrival << ","
            << temp.fetchTime << "} ID{" << temp.decodeArrival << ","
            << temp.decodeTime << "} IS{" << temp.issueArrival << ","
            << temp.issueTime << "} EX{" << temp.executionArrival
            << "} DURATION" << temp.duration << "  src1 ready :: "
            << temp.src1_ready << "  src2 ready :: " << temp.src2_ready
            << endl;

            temp =
            {   0};
        }

    }
#endif
    //only L1 is Present
    if (isL1Present && !isL2Present)
    {
        unsigned int mem_addr;
        bool status;
        for (unsigned int i = 0; i < executionQueueSize; i++)
        {
            if (executionQueue[i].operationType == 2
                    && !executionQueue[i].isL1RequestDone)
            {
                mem_addr = executionQueue[i].mem_address;
                status = cacheL1->bufferPredict(mem_addr);
                executionQueue[i].isL1RequestDone = true;
                if (status)
                {
                    executionQueue[i].latency = 5;
                    executionQueue[i].duration = 5;
                }
                else
                {
                    executionQueue[i].latency = 20;
                    executionQueue[i].duration = 20;
                }
            }
        }
    }
    //L1 and L2 is Present
    if (isL1Present && isL2Present)
    {
        unsigned int mem_addr;
        bool status;
        for (unsigned int i = 0; i < executionQueueSize; i++)
        {
            if (executionQueue[i].operationType == 2
                    && !executionQueue[i].isL1RequestDone)
            {
                mem_addr = executionQueue[i].mem_address;
                status = cacheL1->bufferPredict(mem_addr);
                executionQueue[i].isL1RequestDone = true;
                if (status)
                {
                    executionQueue[i].latency = 5;
                    executionQueue[i].duration = 5;
                }
                else
                {
                    //its L1 miss now check for L2 status
                    status = cacheL2->bufferPredict(mem_addr);
                    executionQueue[i].isL2RequestDone = true;
                    if (status)
                    {
                        executionQueue[i].latency = 10;
                        executionQueue[i].duration = 10;
                    }
                    else
                    {
                        executionQueue[i].latency = 20;
                        executionQueue[i].duration = 20;
                    }
                }
            }
        }
    }

    unsigned int j = 0;
    for (unsigned int i = 0; i < executionQueueSize; i++)
    {
        if (executionQueue[i].duration == 1)
        {
            executionQueue[i].duration = 0;
            temp = executionQueue[i];
            temp.presentStage = WB;
            temp.writeBackArrival = cycle + 1;
            temp.writeBackTime = 1;
            temp.executionTime = temp.writeBackArrival - temp.executionArrival;
            retireList[j++] = temp;
            if (temp.dest_reg > 99)
                writeBack(&temp);
            temp =
            {   0};
            executionQueue[i] =
            {   0};
        }
        else
        {
            executionQueue[i].duration--;
        }
    }
}

void pipeline::issueCycle()
{
    stageReg temp = { 0 };
    unsigned int num = 0;
#if DEBUG
    cout << "*** issue stage " << cycle << "***" << endl;
#endif
    //scan issue queue for ready bits of src1 and src2
    //and create temp_list for ready instruction
    //change of stage

    issueQueueSort(issueQueue);
    for (unsigned int j = 0; j < scheduleQueueCapacity; j++)
    {
        if (issueQueue[j].isValid)
        {
            temp = issueQueue[j];
#if DEBUG
            cout << j << "  " << temp.tagVal << " " << "fu{"
            << temp.operationType << "} " << "src{" << temp.src1_reg
            << "," << temp.src2_reg << "} " << "dst{" << temp.dest_reg
            << "} " << "IF{" << temp.fetchArrival << ","
            << temp.fetchTime << "} ID{" << temp.decodeArrival << ","
            << temp.decodeTime << "} IS{" << temp.issueArrival
            << "  src1 ready :: " << temp.src1_ready
            << "  src2 ready :: " << temp.src2_ready << "} DURATION"
            << temp.duration << endl;

#endif
            temp =
            {   0};
        }

    }

    for (unsigned int i = 0; i < numOfInstructions; i++)
    {
        for (unsigned int j = 0; j < scheduleQueueCapacity; j++)
        {
            temp =
            {   0};
            if (issueQueue[j].isValid)
            {
                if (issueQueue[j].src1_ready && issueQueue[j].src2_ready)
                {
                    temp = issueQueue[j];
                    temp.presentStage = IS;
                    temp.executionArrival = cycle + 1;
                    temp.issueTime = temp.executionArrival - temp.issueArrival;
                    temp.decodeTime = temp.issueArrival - temp.decodeArrival;
                    //issueExecutionList->push_back(temp);
                    issueExecutionList[i] = temp;
                    temp =
                    {   0};
                    issueQueue[j] =
                    {   0};
                    //issueQueue[j].tagVal = 10000000;
                    issueQueueCounter--;
                    num++;
                    break;
                }
            }
        }
    }

    //sort the temp
    // stageRegSort(issueExecutionList);
    for (unsigned int i = 0; i < numOfInstructions; i++)
    {
        temp = issueExecutionList[i];
#if DEBUG
        cout << i << "  " << temp.tagVal << " " << "fu{" << temp.operationType
        << "} " << "src{" << temp.src1_reg << "," << temp.src2_reg
        << "} " << "dst{" << temp.dest_reg << "} " << "IF{"
        << temp.fetchArrival << "," << temp.fetchTime << "} ID{"
        << temp.decodeArrival << "," << temp.decodeTime << "} IS{"
        << temp.issueArrival << "  src1 ready :: " << temp.src1_ready
        << "  src2 ready :: " << temp.src2_ready << "} DURATION"
        << temp.duration << endl;

#endif
        temp =
        {   0};
    }

    //cout << "issue to execution list " << endl;
    for (unsigned int i = 0; i < num; i++)
    {
        temp = issueExecutionList[i];
        if (temp.isValid)
        {
            issueExecutionList[i]=
            {   0};

            for (unsigned int i = 0; i < executionQueueSize; i++)
            {
                if (!executionQueue[i].isValid)
                {
                    executionQueue[i] = temp;
                    temp =
                    {   0};
#if DEBUG
                    if (executionQueue[i].isValid)
                    {

                        //0 fu{2} src{16,-1} dst{4} IF{0,1} ID{1,1} IS{2,1} EX{3,5} WB{8,1}
                        cout << executionQueue[i].tagVal << " " << "fu{"
                        << executionQueue[i].operationType << "} "
                        << "src{" << executionQueue[i].src1_reg << ","
                        << executionQueue[i].src2_reg << "} " << "dst{"
                        << executionQueue[i].dest_reg << "} " << "IF{"
                        << executionQueue[i].fetchArrival << ","
                        << executionQueue[i].fetchTime << "} ID{"
                        << executionQueue[i].decodeArrival << ","
                        << executionQueue[i].decodeTime << "} IS{"
                        << executionQueue[i].issueArrival << ","
                        << executionQueue[i].issueTime << "} EX{"
                        << executionQueue[i].executionArrival << "} DURATION" << executionQueue[i].duration << endl;
                    }
#endif
                    executionQueueCount++;
                    break;
                }
            }
        }
    }
    num = 0;
}

void pipeline::decodeCycle()
{
    stageReg temp = { 0 };
    queueSpace = (dispatchQueue->queueCapacity - dispatchQueue->queueSize);
#if DEBUG
    cout << "*** decode stage " << cycle << "***" << endl;
    cout << "capacity :: " << dispatchQueue->queueCapacity << "  size :: "
    << dispatchQueue->queueSize << endl;
    cout << "queue space " << queueSpace << endl;
#endif
    unsigned int diff = scheduleQueueCapacity - issueQueueCounter;
#if DEBUG
    cout << "diff :: " << diff << endl;
#endif
    if (diff <= scheduleQueueCapacity)
    {
        for (unsigned int i = 0; i < min(numOfInstructions, diff); i++)
        {
            // if (!feof(pFileHandler))
            {
                temp =
                {   0};
                temp = front(dispatchQueue);
                if (temp.isValid)
                {
                    temp = dequeue(dispatchQueue);
                    if (temp.presentStage == IF)
                    {
                        //change present state
                        temp.presentStage = ID;
                        ///temp.decodeTime = temp.decodeArrival;

                        //rename
                        //src1
                        if (temp.src1_reg > -1)
                        {
                            if (renameMapTable[temp.src1_reg].isValid)
                            {
                                temp.src1_reg =
                                renameMapTable[temp.src1_reg].tagVal;
                                temp.src1_ready = false;
                            }
                            else
                            {
                                temp.src1_ready = true;
                            }
                        }
                        else
                        {
                            temp.src1_ready = true;
                        }

                        //src2
                        if (temp.src2_reg > -1)
                        {
                            if (renameMapTable[temp.src2_reg].isValid)
                            {
                                temp.src2_reg =
                                renameMapTable[temp.src2_reg].tagVal;
                                temp.src2_ready = false;
                            }
                            else
                            {
                                temp.src2_ready = true;
                            }
                        }
                        else
                        {
                            temp.src2_ready = true;
                        }

                        //dest reg
                        if (temp.dest_reg > -1)
                        {
                            renameMapTable[temp.dest_reg].isValid = true;
                            renameMapTable[temp.dest_reg].tagVal =
                            (bufferIndex++) + 100;
                            temp.dest_reg =
                            renameMapTable[temp.dest_reg].tagVal;
                        }
                    }
                    //put temp in list
                    dispatchIssueList[i] = temp;
                    temp =
                    {   0};
                }
            }
        }
    }

    //sort the temp
    //stageRegSort(dispatchIssueList);
    for (unsigned int i = 0; i < min(numOfInstructions, diff); i++)
    {
        temp = dispatchIssueList[i];
        if (temp.isValid)
        {
            dispatchIssueList[i] =
            {   0};
            temp.issueArrival = cycle + 1;
#if DEBUG1
            cout << temp.tagVal << " " << "fu{" << temp.operationType << "} "
            << "src{" << temp.src1_reg << "," << temp.src2_reg << "} "
            << "dst{" << temp.dest_reg << "} " << "IF{"
            << temp.fetchArrival << "," << temp.fetchTime << "} ID{"
            << temp.decodeArrival << "," << temp.decodeTime << "} IS{"
            << temp.issueArrival << endl;
#endif
            for (unsigned int i = 0; i < scheduleQueueCapacity; i++)
            {
                if (!issueQueue[i].isValid)
                {
                    issueQueue[i] = temp;
#if DEBUG
                    cout << temp.tagVal << " " << "fu{" << temp.operationType << "} "
                    << "src{" << temp.src1_reg << "," << temp.src2_reg << "} "
                    << "dst{" << temp.dest_reg << "} " << "IF{"
                    << temp.fetchArrival << "," << temp.fetchTime << "} ID{"
                    << temp.decodeArrival << "," << temp.decodeTime << "} IS{"
                    << temp.issueArrival << "  src1 ready :: " << temp.src1_ready
                    << "  src2 ready :: " << temp.src2_ready << "} DURATION"
                    << temp.duration << endl;
#endif
                    temp =
                    {   0};
                    issueQueueCounter++;
                    break;
                }
            }
        }
    }
}

void pipeline::fetchCycle()
{
    unsigned int address;
    unsigned int operationType;
    int src_register1;
    int src_register2;
    int dest_register;
    unsigned int mem_addr;
    stageReg temp = { 0 };
#if DEBUG
    cout << " **** fetch stage operation count  ::  " << cycle << "****"
    << endl;
#endif

    for (unsigned int i = 0; i < min(numOfInstructions, queueSpace); i++)
    {
        if (fakeRobIndex == fakeRobSize)
            fakeRobIndex = 0;
        //file input
        //<PC> <operation type> <dest reg #> <src1 reg #> <src2 reg #> <mem address>
        fscanf(pFileHandler, "%x %x %d %d %d %x", &address, &operationType,
                &dest_register, &src_register1, &src_register2, &mem_addr);
        if (!feof(pFileHandler))
        {
            fakeRob[fakeRobIndex].isValid = true;
            fakeRob[fakeRobIndex].tagVal = operationCount++;
            fakeRob[fakeRobIndex].progCount = address;
            fakeRob[fakeRobIndex].mem_address = mem_addr;
            fakeRob[fakeRobIndex].operationType = operationType;
            fakeRob[fakeRobIndex].dest_reg = dest_register;
            fakeRob[fakeRobIndex].src1_reg = src_register1;
            fakeRob[fakeRobIndex].src2_reg = src_register2;
            fakeRob[fakeRobIndex].presentStage = IF;
            fakeRob[fakeRobIndex].fetchArrival = cycle;
            fakeRob[fakeRobIndex].fetchTime = 1;
            fakeRob[fakeRobIndex].decodeArrival = cycle + 1;
            switch (operationType)
            {
                case 0:
                    fakeRob[fakeRobIndex].latency = 1;
                    fakeRob[fakeRobIndex].duration = 1;
                    break;
                case 1:
                    fakeRob[fakeRobIndex].latency = 2;
                    fakeRob[fakeRobIndex].duration = 2;
                    break;
                case 2:
                    fakeRob[fakeRobIndex].latency = 5;
                    fakeRob[fakeRobIndex].duration = 5;
                    break;
            }
#if DEBUG
            if (fakeRob[fakeRobIndex].isValid)
            {
                //0 fu{2} src{16,-1} dst{4} IF{0,1} ID{1,1} IS{2,1} EX{3,5} WB{8,1}
                cout << fakeRob[fakeRobIndex].tagVal << " " << "fu{"
                << fakeRob[fakeRobIndex].operationType << "} " << "src{"
                << fakeRob[fakeRobIndex].src1_reg << ","
                << fakeRob[fakeRobIndex].src2_reg << "} " << "dst{"
                << fakeRob[fakeRobIndex].dest_reg << "} " << "IF{"
                << fakeRob[fakeRobIndex].fetchArrival << ","
                << fakeRob[fakeRobIndex].fetchTime << "}" << endl;
            }
#endif
            //move to dispatch queue
            temp = fakeRob[fakeRobIndex];
            enqueue(dispatchQueue, temp);
            fakeRobIndex++;
        }
    }
}

void pipeline::printFunc()
{
    cout << "CONFIGURATION" << endl;
    cout << " superscalar bandwidth (N) = " << dec << numOfInstructions << endl;
    cout << " dispatch queue size (2*N) = " << dec << dispatchQueueSize << endl;
    cout << " schedule queue size (S)   = " << dec << scheduleQueueCapacity
            << endl;
    cout << "RESULTS" << endl;
    cout << " number of instructions = " << dec << operationCount << endl;
    cout << " number of cycles       = " << dec << cycle << endl;
    float ipc = operationCount / (cycle * 1.0);
    cout << " IPC                    = " << setprecision(3) << ipc << endl;
}

queue* pipeline::createQueue(unsigned int size)
{
    queue *newQueue;
    newQueue = new queue;
    //error checking
    if (newQueue == NULL)
        return NULL;
    //initalize queue params
    newQueue->queueCapacity = size;
    newQueue->queueFront = 0;
    newQueue->queueSize = 0;
    newQueue->queueRear = newQueue->queueCapacity - 1;
    newQueue->regVal = new pipelineReg[newQueue->queueCapacity];
    return newQueue;
}

void pipeline::enqueue(queue *updateQueue, stageReg temp)
{
    updateQueue->queueRear = (updateQueue->queueRear + 1)
            % updateQueue->queueCapacity;
    updateQueue->regVal[updateQueue->queueRear] = temp;
    updateQueue->queueSize++;
}

stageReg pipeline::dequeue(queue *updateQueue)
{
    stageReg retVal;
    retVal = updateQueue->regVal[updateQueue->queueFront];
    updateQueue->regVal[updateQueue->queueFront] =
    {   0};
    updateQueue->queueFront = (updateQueue->queueFront + 1)
            % updateQueue->queueCapacity;
    updateQueue->queueSize--;
    return retVal;
}

stageReg pipeline::front(queue *updateQueue)
{
    stageReg retVal;
    retVal = updateQueue->regVal[updateQueue->queueFront];
    return retVal;
}
