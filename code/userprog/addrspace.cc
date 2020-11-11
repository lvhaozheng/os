// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
//#include "noff.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);

    

#ifdef REVERSE_PAGE
    int num = 0;
    while(num < numPages){
        bool flag = true; //用来标志是否有有效内存
        for (i = 0; i < NumPhysPages; i++) {
            if(!machine->pageTable[i].valid){
                flag=false;
                machine->pageTable[i].virtualPage = num;
                machine->pageTable[i].physicalPage = machine->allocateMemory();
                machine->pageTable[i].valid = TRUE;
                machine->pageTable[i].use = FALSE;
                machine->pageTable[i].dirty = FALSE;
                machine->pageTable[i].readOnly = FALSE;
                machine->pageTable[i].tid = currentThread->getTid(); // The additional part of inverted page table
                break;
            }
        }
        if(flag) {
            ASSERT(FALSE);
        }
        num++;
    }
        int num = 0;
    while(num < numPages){
        bool flag = true; //用来标志是否有有效内存
        for (i = 0; i < NumPhysPages; i++) {
            if(!machine->pageTable[i].valid){
                flag=false;
                machine->pageTable[i].virtualPage = num;
                machine->pageTable[i].physicalPage = machine->allocateMemory();
                machine->pageTable[i].valid = TRUE;
                machine->pageTable[i].use = FALSE;
                machine->pageTable[i].dirty = FALSE;
                machine->pageTable[i].readOnly = FALSE;
                machine->pageTable[i].tid = currentThread->getTid(); // The additional part of inverted page table
                break;
            }
        }
        if(flag) {
            ASSERT(FALSE);
        }
        num++;
    }
    if(noffH.code.size > 0){
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
        	noffH.code.virtualAddr, noffH.code.size);
        int position = noffH.code.inFileAddr;
        for(int num=0; num < noffH.code.size; num += PageSize){ //根据物理地址每次读取一页
            unsigned int vpn = (unsigned)((noffH.code.virtualAddr + num) / PageSize);
            unsigned int physicalAddr ;
            for (i = 0; i < NumPhysPages; i++) {
                if(machine->pageTable[i].virtualPage == vpn){
                    physicalAddr = (unsigned)(machine->pageTable[i].physicalPage * PageSize);
                }
            }
            executable->ReadAt(&(machine->mainMemory[physicalAddr]),PageSize,position);
            position += PageSize;
        }
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
    		noffH.initData.virtualAddr, noffH.initData.size);
        int position = noffH.initData.inFileAddr;
        for(int num=0; num < noffH.initData.size; num += PageSize){ //根据物理地址每次读取一页
            unsigned int vpn = (unsigned)((noffH.initData.virtualAddr + num) / PageSize);
            unsigned int physicalAddr;
            for (i = 0; i < NumPhysPages; i++) {
                if(machine->pageTable[i].virtualPage == vpn){
                    physicalAddr = (unsigned)(machine->pageTable[i].physicalPage * PageSize);
                }
            }
            executable->ReadAt(&(machine->mainMemory[physicalAddr]),PageSize,position);
            position += PageSize;
        }
    }
#elif USE_BITMAP
    // first, set up the translation
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
        pageTable[i].physicalPage = machine->allocateMemory();
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
                        // a separate page, we could set its
                        // pages to be read-only
    }
    bzero(machine->mainMemory, size);
        // lab2 exercise4 根据分配的物理内存将文件放到内存当中
    if(noffH.code.size > 0){
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
        	noffH.code.virtualAddr, noffH.code.size);
        int position = noffH.code.inFileAddr;
//        for(int byte=0; byte < noffH.code.size; byte++){ //根据物理地址每次读取一个字节
//            unsigned int vpn = (unsigned)((noffH.code.virtualAddr + byte) / PageSize);
//            unsigned int offset = (unsigned)((noffH.code.virtualAddr + byte) % PageSize);
//            unsigned int physicalAddr = (unsigned)(pageTable[vpn].physicalPage * PageSize + offset);
//            executable->ReadAt(&(machine->mainMemory[physicalAddr]),1,position);
//        }
        for(int num=0; num < noffH.code.size; num += PageSize){ //根据物理地址每次读取一页
            unsigned int vpn = (unsigned)((noffH.code.virtualAddr + num) / PageSize);
            unsigned int physicalAddr = (unsigned)(pageTable[vpn].physicalPage * PageSize);
            executable->ReadAt(&(machine->mainMemory[physicalAddr]),PageSize,position);
            position += PageSize;
        }
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
    		noffH.initData.virtualAddr, noffH.initData.size);
        int position = noffH.initData.inFileAddr;
        for(int num=0; num < noffH.initData.size; num += PageSize){ //根据物理地址每次读取一页
            unsigned int vpn = (unsigned)((noffH.initData.virtualAddr + num) / PageSize);
            unsigned int physicalAddr = (unsigned)(pageTable[vpn].physicalPage * PageSize);
            executable->ReadAt(&(machine->mainMemory[physicalAddr]),PageSize,position);
            position += PageSize;
        }
    }
#else
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
        pageTable[i].physicalPage = i;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
        // a separate page, we could set its
        // pages to be read-only
    }
    // zero out the entire address space, to zero the unitialized data segment
// and the stack segment
    bzero(machine->mainMemory, size);
// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
#endif
}

AddrSpace::AddrSpace(OpenFile *executable,int restructure)
{

    NoffHeader noffH;
    unsigned int i, size;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
    this->executable = executable;
    this->noffH = noffH;
//    printf("noffH.code.virtual: %d, noffH.code.size: %d,noffH.code.inFileAddr: %d,\n",noffH.code.virtualAddr,noffH.code.size,noffH.code.inFileAddr);
//    printf("noffH.init.virtual: %d, noffH.initData.size: %d,noffH.initData.inFileAddr: %d\n",noffH.initData.virtualAddr,noffH.initData.inFileAddr);
//    printf("noffH.uninitData.virtual: %d, noffH.uninitData.size: %d, noffH.uninitData.inFileAddr: %d\n",noffH.uninitData.virtualAddr,noffH.uninitData.inFileAddr);
// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
           + UserStackSize;	// we need to increase the size

    // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
    printf("size : %d\n",size);
//    ASSERT(numPages <= NumPhysPages);		// check we're not trying
//    // to run anything too big --
//    // at least until we have
//    // virtual memory
    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
          numPages, size);
    bzero(machine->mainMemory, MemorySize);
// first, set up the translation

#ifndef  REVERSE_PAGE
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
        pageTable[i].physicalPage = 0; //没有装入所以所有的物理内存都设置为0
        pageTable[i].valid = FALSE; //没有装入任何的一页
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
        // a separate page, we could set its
        // pages to be read-only
    }
#else
    int num = 0;
    while(num < numPages){
        bool flag = true; //用来标志是否有有效内存
        for (i = 0; i < NumPhysPages; i++) {
            if(!machine->pageTable[i].valid){
                flag=false;
                machine->pageTable[i].virtualPage = num;
                machine->pageTable[i].physicalPage = machine->allocateMemory();
                machine->pageTable[i].valid = TRUE;
                machine->pageTable[i].use = FALSE;
                machine->pageTable[i].dirty = FALSE;
                machine->pageTable[i].readOnly = FALSE;
                machine->pageTable[i].tid = currentThread->getTid(); // The additional part of inverted page table
                break;
            }
        }
        if(flag) {
            printf("out of memory\n");
        }
        num++;
    }
#endif
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    delete executable;
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
#ifdef USE_TLB
    for (int i = 0; i < TLBSize; i++) { //因为进行线程切换所以原本的tlb失效
        machine->tlb[i].valid = FALSE;
    }
#endif
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
#ifndef REVERSE_PAGE
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
#endif
}

void
AddrSpace::PrintState()
{
    printf("=== %s ===\n",  "Address Space Information");
    printf("numPages = %d\n", numPages);
    printf("VPN\tPPN\tvalid\tRO\tuse\tdirty\n");
    for (int i = 0; i < numPages; i++) {
        printf("%d\t", pageTable[i].virtualPage);
        printf("%d\t", pageTable[i].physicalPage);
        printf("%d\t", pageTable[i].valid);
        printf("%d\t", pageTable[i].use);
        printf("%d\t", pageTable[i].dirty);
        printf("%d\t", pageTable[i].readOnly);
        printf("\n");
    }
    machine->bitmap->Print();
    printf("=================================\n");
}