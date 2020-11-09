// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

int position=0;
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException)) {
        if (type == SC_Halt) {
            printf("TLB Miss: %d, TLB Hit: %d, Total Translate: %d, TLB Miss Rate: %.2lf%%\n",
                   TLBMissNumber, TLBTranslateNumber - TLBMissNumber, TLBTranslateNumber,
                   (double) (TLBMissNumber * 100) / (TLBTranslateNumber));
            interrupt->Halt();
        }else if(type == SC_Exit){
            printf("exit program\n");
            if(currentThread->space!=NULL){
                machine->freeMemory();
                delete currentThread->space;
                currentThread->space = NULL;
                currentThread->Finish();
            }
        }
    } else if(which == PageFaultException) {
        TLBMissNumber++;
        if(machine->tlb == NULL) {

        }else {
            DEBUG('m', " <-- TLB Miss -->\n");
            int addr = machine->ReadRegister(BadVAddrReg);
//            exercise2 缺页处理
//            unsigned int vpn = (unsigned) addr / PageSize;
//            position = position%TLBSize;
//            machine->tlb[position] = machine->pageTable[vpn];
//            position++;
//            if (position==0) position=1;
//            else position=0;
#ifdef TLB_LRU
        machine->TLBLRUSwap(addr);
#endif
#ifdef TLB_CLOCK
    machine->TLBClockSwap(addr);
#endif
        }
        return;
    }else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
