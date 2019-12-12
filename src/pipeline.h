/*
 * pipeline.h
 *
 *  Created on: 04-Dec-2019
 *      Author: prati
 */

#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "defination.h"

#define ROB_SIZE 1024

enum STAGE
{
    IF = 0,
    ID,
    IS,
    EX,
    WB,
};

typedef struct pipelineReg
{
    //sequence num
    unsigned int tagVal;
    //status
    bool isValid;
    //PC
    unsigned int progCount;
    //Destination register
    int dest_reg;
    bool dest_ready;
    //src 1 register
    int src1_reg;
    bool src1_ready;
    //src 2 register
    int src2_reg;
    bool src2_ready;
    //operation type
    unsigned int operationType;
    //mem address
    unsigned int mem_address;
    //cache L1 request done
    bool isL1RequestDone;
    //cache L2 request done
    bool isL2RequestDone;
    //latency
    unsigned int latency;
    //duration in execution stage
    unsigned int duration;
    //Present Stage
    unsigned int presentStage;
    //for stages
    unsigned int fetchArrival;
    unsigned int fetchTime;
    unsigned int decodeArrival;
    unsigned int decodeTime;
    unsigned int issueArrival;
    unsigned int issueTime;
    unsigned int executionArrival;
    unsigned int executionTime;
    unsigned int writeBackArrival;
    unsigned int writeBackTime;
} stageReg;

//Rename Map table
struct renameMapTable_t
{
    bool isValid;
    int tagVal;
};

typedef struct queue_struct
{
    int queueFront;
    int queueRear;
    int queueSize;
    int queueCapacity;
    stageReg *regVal;
}queue;


class pipeline
{
public:
    pipeline(unsigned int scheduleSize, unsigned int fetchSize);
    virtual ~pipeline();
    void retire();
    void fetchCycle();
    void decodeCycle();
    void issueCycle();
    void executeCycle();
    bool Advance_cycle();
    void writeBack(stageReg *writeBackVal);
    bool isFakeRobEmpty();
    void stageRegSort(stageReg *);
    void issueQueueSort(stageReg *issueList);
    void printFunc();
    void swap(stageReg *val1, stageReg *val2);
    queue* createQueue(unsigned int size);
    void enqueue(queue *updateQueue, stageReg temp);
    stageReg dequeue(queue *updateQueue);
    stageReg front(queue *updateQueue);

    FILE *pFileHandler;                 // File handler for input trace file
    //L1 Cache Params
    BranchBuffer *cacheL1;
    bool isL1Present;
    //L2 cache Params
    BranchBuffer *cacheL2;
    bool isL2Present;

private:
    unsigned int numOfInstructions;
    unsigned int scheduleQueueCapacity;
    unsigned int dispatchQueueSize;
    unsigned int executionQueueSize;
    unsigned int executionQueueCount;
    unsigned int issueQueueCounter;
    unsigned int queueSpace;
    //run time variable creation

    //renaming map table
    renameMapTable_t *renameMapTable;

    //fake rob pointer
    stageReg *fakeRob;
    unsigned int fakeRobSize;
    unsigned int fakeRobHead;

    //issue queue
    stageReg *issueQueue;
    unsigned int issueQueueHead;
    unsigned int issueQueueTail;

    //execution queue
    stageReg *executionQueue;
    unsigned int executionQueueHead;
    unsigned int executionQueueTail;

    //dispatch queue
    queue *dispatchQueue;

    //temp list for dispatch to issue
    stageReg *dispatchIssueList;
    unsigned int dispatchIssueListHead;
    unsigned int dispatchIssueListTail;

    //temp list for issue to execution
    stageReg *issueExecutionList;
    unsigned int issueExecutionListHead;
    unsigned int issueExecutionListTail;

    //temp list for retrie stage
    stageReg *retireList;
};

#endif /* PIPELINE_H_ */
