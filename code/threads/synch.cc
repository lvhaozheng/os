// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    }
    value--; 					// semaphore available,
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
    mutex = new Semaphore(debugName,1);
    name = debugName;
}
Lock::~Lock() {
    delete mutex;
}
void Lock::Acquire() {

    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    DEBUG('s',"Acquire lock name %s, thread name %s\n", name, currentThread->getName());
    mutex->P();
    holderThread = currentThread;
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
void Lock::Release() {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    DEBUG('s',"Release lock name %s, thread name %s\n", name, currentThread->getName());
    mutex->V();
    holderThread = NULL;
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
bool Lock::isHeldByCurrentThread() {
    return holderThread == currentThread;
}

Condition::Condition(char* debugName) {
    name = debugName;
    blockingQueue = new List;
}
Condition::~Condition() {
    delete blockingQueue;
}

//根据老师上课所讲，wait分为三步 1、解锁 2、等待 3、上锁
void Condition::Wait(Lock* conditionLock) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

    ASSERT(conditionLock->isLocked()) //after lock ,then call wait function
    ASSERT(conditionLock->isHeldByCurrentThread()) //when the lock is holded by current thread, it can execute wait

    //
    // 1. Release the lock while it waits
    conditionLock->Release();
    // 2. wait
    blockingQueue->Append((void *)currentThread);
    currentThread->Sleep();
    // 3.acquire lock again
    conditionLock->Acquire();

    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
void Condition::Signal(Lock* conditionLock) {

    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

    ASSERT(conditionLock->isHeldByCurrentThread()) //when the lock is holded by current thread, it can execute wait

    Thread *thread = (Thread *)blockingQueue->Remove();
    if (thread != NULL)	   // signal the thread
        scheduler->ReadyToRun(thread);

    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
void Condition::Broadcast(Lock* conditionLock) {

    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

    ASSERT(conditionLock->isHeldByCurrentThread()) //when the lock is holded by current thread, it can execute wait

    while(!blockingQueue->IsEmpty()) {
        Thread *thread = (Thread *) blockingQueue->Remove();
        if (thread != NULL)       // signal the thread
            scheduler->ReadyToRun(thread);
    }

    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}



Barrier::Barrier(char *debugName, int parties) {
    name = debugName;
    if(parties<0) {
        DEBUG('s',"the barrier num invalid\n");
        ASSERT(parties > 0);
    }
    this->parties = parties;
    index = 0;
    mutex = new Lock("barrier lock");
    barrierCondition = new Condition("barrier condition");

}

Barrier::~Barrier() {
    delete mutex;
    delete barrierCondition;
}

void Barrier::Await() {

    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

    mutex->Acquire();
    index++;
    DEBUG('s',"Thread name: %s add to barrier, there are %d threads in the barrier\n",currentThread->getName(), index);

    if(index == parties){
        DEBUG('s',"Everyone has arrived\n");
        barrierCondition->Broadcast(mutex);
        index = 0;
    }else{
        barrierCondition->Wait(mutex);
    }
    mutex->Release();

    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

int Barrier::getNumberWaiting() {
    return index;
}


RWLock::RWLock(char *debugName) {
    name = debugName;

    mutex = new Lock("rw lock");

    readerCnt = 0;

    rCondition = new Condition("read condition");
    wCondition = new Condition("write condition");
}

RWLock::~RWLock() {
    delete mutex;
    delete rCondition;
    delete wCondition;
}

void RWLock::ReadAcquire() {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    mutex->Acquire();
    while (writeCnt > 0){
        DEBUG('s',"******  There is a writer to write again, unable to read  ******\n");
        rCondition->Wait(mutex);
    }
    readerCnt++;
    mutex->Release();
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

void RWLock::ReadRelease() {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    mutex->Acquire();
    readerCnt--;
    if (readerCnt == 0) wCondition->Signal(mutex);
    mutex->Release();
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

void RWLock::WriteAcquire() {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    mutex->Acquire();
    while (writeCnt > 0 || readerCnt > 0){
        DEBUG('s',"******  There is a writer to write again, unable to write  ******\n");
        wCondition->Wait(mutex);
    }
    writeCnt++;
    mutex->Release();
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

void RWLock::WriteRelease() {

    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    mutex->Acquire();
    writeCnt--;
    rCondition->Broadcast(mutex);
    wCondition->Signal(mutex);
    mutex->Release();
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts

}