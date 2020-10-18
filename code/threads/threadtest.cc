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
    default:
	printf("No test specified.\n");
	break;
    }
}

