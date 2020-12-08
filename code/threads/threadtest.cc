// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// create a simple thread
//----------------------------------------------------------------------
void
TheThread(int which){
    printf("test thread tid = %d, priority = %d\n",which,currentThread->getPriority());
//    currentThread->Yield();
}

//----------------------------------------------------------------------
// Test print all thread message
//----------------------------------------------------------------------
void
TestThread(int which){
    IntStatus oldLevel;
    printf("tid is %d,operating is --->> ",which);
    if(which==1){
        oldLevel = interrupt->SetLevel(IntOff);	// before sleep need disable interrupts ,or error
        printf("** Sleep  **\n");
        currentThread->Sleep();
        (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
    }
    else if(which==2){
        printf("** Finish **\n");
        currentThread->Finish();
    }else if(which==3){
        printf("** Yield  **\n");
        currentThread->Yield();
    }
    currentThread->Yield();
}
void
PrintAllThread()
{
    printf("\n------------------------------------------------------\n");
    for(int i=0;i<4;i++){
        Thread *t1 = new Thread("thread");
        if(t1->getTid()>0 && i!=3)
            t1->Fork(TestThread,t1->getTid());
    }
    currentThread->Yield();
    printf("\n------------------------------------------------------\n");
    scheduler->PrintThreads();
    printf("\n-----------------------------------------------------\n");
    currentThread->Yield();
}

//----------------------------------------------------------------------
// Test create max 128 threads
//----------------------------------------------------------------------
void
createMaxThread()
{
    for(int i=1;i<=130;i++){
        printf("create thread,tid = %d\t",i);
        Thread *t1 = new Thread("thread");
        if(t1->getTid()>0)
            t1->Fork(TheThread,i);
    }
    currentThread->Yield();
}

//----------------------------------------------------------------------
// Test priority scheduling algorithm
//----------------------------------------------------------------------

void DoNothingthread(int which){
    printf("execute %s\n",currentThread->getName());
}
void prioThread(int which){
    printf("thread priority: %d, tid is %d,operating is --->> ",currentThread->getPriority(),which);
    if(which == 1){
        Thread* t1= new Thread("thread1");
        printf("Fork thread1, priority = 5\n");
        t1->Fork(DoNothingthread,t1->getTid());
        printf("execute currentThread\n");
    }else if(which == 2){
        Thread* t2= new Thread("thread2");
        t2->setPriority(4);
        printf("Fork thread2, priority = 4\n");
        t2->Fork(DoNothingthread,t2->getTid());
        printf("execute currentThread\n");
    }else{
        Thread* t3= new Thread("thread3");
        t3->setPriority(6);
        printf("Fork thread3, priority = 6\n");
        t3->Fork(DoNothingthread,t3->getTid());
        printf("execute currentThread\n");
    }
}
void
testPriorityAlgorithm()
{
    currentThread->setPriority(7);
    for(int i=0;i<=2;i++){
        printf("------------------------------------------------\n");
        Thread *t1 = new Thread("thread0");
        if(t1->getTid()>0){
            t1->Fork(prioThread,t1->getTid());
        }
    }
}


//----------------------------------------------------------------------
// Test time slice rotation algorithm
//----------------------------------------------------------------------
void
TimeSliceThread(int which)
{
    int cnt = 0;
    while(cnt++<4){
        printf("-------------------------------------------------------------\n");
//        printf("totalTicks : %d, userTicks: %d, SystemTicks: %d \n",stats->totalTicks,stats->userTicks,stats->systemTicks);
        printf("current thread tid: %d, excute : %d time(s)\n",currentThread->getTid(), cnt);
//               ,currentThread->getTimeSlice()
//               ,currentThread->getPriority());
//               );
        //Manual analog clock
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
    }
}

void
TestTimeSliceThread()
{
    int cntThread = 3;
    for(int i=0;i<cntThread;i++){
        Thread* t = new Thread("time slice thread");
        if( t->getTid()>0) {
            t->Fork(TimeSliceThread, t->getTid());
        }
    }

}


//----------------------------------------------------------------------
// Test multistage feedback queue algorithm
//----------------------------------------------------------------------
void
QueueThread(int which)
{
    int cnt = 0;
    while(cnt++<6){
        printf("-------------------------------------------------------------\n");
//        printf("totalTicks : %d, userTicks: %d, SystemTicks: %d \n",stats->totalTicks,stats->userTicks,stats->systemTicks);
        printf("current thread tid: %d, excute : %d time(s), timeSlice: %d, priority: %d\n",currentThread->getTid(), cnt
               ,currentThread->getTimeSlice()
               ,currentThread->getPriority());
        //Manual analog clock
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
    }
}

void
TestQueueThread()
{
    int cntThread = 3;
    for(int i=0;i<cntThread;i++){
        Thread* t = new Thread("time slice thread");
        if( t->getTid()>0) {
            t->setPriority(1);
            t->Fork(QueueThread, t->getTid());
        }
    }

}
//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");
    if(t->getTid()>0)
        t->Fork(SimpleThread, 1);
    SimpleThread(0);
}


int readCount = 0;
int writerCount = 0;
char buffer[80];

Lock* mutex = new Lock("readCount");
Semaphore *w=new Semaphore("writer",1);
Lock* rwMutex = new Lock("readAndWriterLock");
Condition *writerCondition = new Condition("writerCondition");
Condition *readerCondition = new Condition("readerCondition");

void readerBySemaphore(int which){
    int num=1;
    while(num-->0){
        mutex->Acquire();
        readCount++;
        if(readCount == 1) w->P();
        mutex->Release();
        printf("%s\n",buffer);
        currentThread->Yield();
        mutex->Acquire();
        readCount--;
        if(readCount==0) w->V();
        mutex->Release();
        printf("************************************************\n");
        currentThread->Yield();
    }
}

void writerBySemaphore(int which){
    int num=4;
    while(num-->0){
        w->P();
        printf("------  writer begin work ，num: %d-------\n",num);
        printf("************************************************\n");
        sprintf(buffer,"writer num :%d",num);
        w->V();
        currentThread->Yield();
    }
}

void readerByCondition(int which){
    int num=1;
    while(num-->0){
        rwMutex->Acquire();
        while (writerCount>0){
            printf("******  There is a writer to write again, unable to read  ******\n");
            readerCondition->Wait(rwMutex);
        }
        readCount++;
        rwMutex->Release();
        printf("%s\n",buffer);
        currentThread->Yield();
        rwMutex->Acquire();
        readCount--;
        if(readCount==0) writerCondition->Signal(rwMutex);
        rwMutex->Release();
        currentThread->Yield();
    }
}

void writerByCondition(int which){
    int num=4;
    while(num-->0){
        rwMutex->Acquire();
        while (writerCount > 0 || readCount > 0) { //有读者和写者，则释放锁，等待读者和写者结束
            printf("****** There is a writer or read to do, unable to write ******\n");
            writerCondition->Wait(rwMutex);
        }
        writerCount++;
        rwMutex->Release();
        sprintf(buffer,"writer num :%d",num);
        rwMutex->Acquire();
        writerCount--;
        readerCondition->Broadcast(rwMutex);
        writerCondition->Signal(rwMutex);
        rwMutex->Release();
        currentThread->Yield();
    }
}

RWLock *rwLock = new RWLock("rwLock");
void readerByRWLock(int which){
    int num=1;
    while(num-->0){
        rwLock->ReadAcquire();
        printf("%s\n",buffer);
        currentThread->Yield();
        rwLock->ReadRelease();
        currentThread->Yield();
    }
}

void writerByRWLock(int which){
    int num=4;
    while(num-->0){
        rwLock->WriteAcquire();
        sprintf(buffer,"writer num :%d",num);
        currentThread->Yield();
        rwLock->WriteRelease();
        currentThread->Yield();
    }
}

void
ProcessSynchronizationTest1(){
    sprintf(buffer, "initial %s","data");
    Thread *reader1= new Thread("reader1");
    reader1->Fork(readerBySemaphore,1);
    Thread *reader2= new Thread("reader2");
    reader2->Fork(readerBySemaphore,2);
    Thread *writer1= new Thread("writer");
    writer1->Fork(writerBySemaphore,1);
    Thread *reader3= new Thread("reader3");
    reader3->Fork(readerBySemaphore,3);
    Thread *reader4= new Thread("reader4");
    reader4->Fork(readerBySemaphore,4);
}

void
ProcessSynchronizationTest2(){
    sprintf(buffer, "initial %s","data");
    Thread *reader1= new Thread("reader1");
    reader1->Fork(readerByCondition,1);
    Thread *reader2= new Thread("reader2");
    reader2->Fork(readerByCondition,2);
    Thread *writer1= new Thread("writer1");
    writer1->Fork(writerByCondition,1);
    Thread *writer2= new Thread("writer2");
    writer2->Fork(writerByCondition,2);
    Thread *reader3= new Thread("reader3");
    reader3->Fork(readerByCondition,3);
    Thread *reader4= new Thread("reader4");
    reader4->Fork(readerByCondition,4);
}

void
ProcessSynchronizationTest3(){
    sprintf(buffer, "initial %s","data");
    Thread *reader1= new Thread("reader1");
    reader1->Fork(readerByRWLock,1);
    Thread *reader2= new Thread("reader2");
    reader2->Fork(readerByRWLock,2);
    Thread *writer1= new Thread("writer1");
    writer1->Fork(writerByRWLock,1);
    Thread *reader3= new Thread("reader3");
    reader3->Fork(readerByRWLock,3);
    Thread *reader4= new Thread("reader4");
    reader4->Fork(readerByRWLock,4);

}

Barrier *barrier=new Barrier("barrier",4);
void barrierThread(int which){
    printf("thread name (%s) start with %d threads waiting\n",currentThread->getName(),barrier->getNumberWaiting());
    barrier->Await();
    printf("thread name (%s) execute\n", currentThread->getName());
}

void barrierTest(){
    Thread *t1=new Thread("thread1");
    t1->Fork(barrierThread,t1->getTid());
    Thread *t2=new Thread("thread2");
    t2->Fork(barrierThread,t1->getTid());
    Thread *t3=new Thread("thread3");
    t3->Fork(barrierThread,t1->getTid());
    Thread *t4=new Thread("thread4");
    t4->Fork(barrierThread,t1->getTid());
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();break;
	case 2:
	    createMaxThread();break;
	case 3:
        PrintAllThread();break;
    case 4:
        testPriorityAlgorithm();break;
    case 5:
        TestTimeSliceThread();break;
    case 6:
        TestQueueThread();break;
    case 7:
        ProcessSynchronizationTest1();break;
        case 8:
            ProcessSynchronizationTest2();break;
        case 9:
            barrierTest();break;
        case 10:
            ProcessSynchronizationTest3();break;
    default:
	printf("No test specified.\n");
	break;
    }
}

