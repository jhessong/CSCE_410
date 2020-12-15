/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#include "page_table.H"
#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    to_alloc = (struct _to_alloc*)(base_address);
    region = 0;
    page_table->register_pool(this);
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    unsigned long address;
    if(size == 0) {
       Console::puts("0 invalid for allocation");
       return 0;
    }

    assert(region < MAX_REGIONS);

    unsigned b = _size % (Machine::PAGE_SIZE);
    unsigned long frame_num = _size / (Machine::PAGE_SIZE);
    if(b > 0) {
       frame_num++;
    }
    if(region == 0) {
       address = base_address;
       to_alloc[region].base = address + Machine::PAGE_SIZE;
       to_alloc[region].length = frame_num*(Machine::PAGE_SIZE);
       region++;
       return address + Machine::PAGE_SIZE;
    }
    else {
       address = to_alloc[region - 1].base + to_alloc[region - 1].length;
    }
    to_alloc[region].length = frame_num*(Machine::PAGE_SIZE);
    to_alloc[region].base = address;
    region++;
    Console::puts("Allocated region of memory.\n");
    return address;
}

void VMPool::release(unsigned long _start_address) {
    int curr = -1;
    for(int i = 0; i < MAX_REGIONS; i++) {
       if(to_alloc[i].base == _start_address) {
          curr = i;
          break;
       }
    }
    assert(!(curr < 0));
    unsigned int pages = ((to_alloc[curr].length)/(Machine::PAGE_SIZE));
    for(int i = 0; i < pages; i++) {
       page_table->free_page(_start_address);
       _start_address += Machine::PAGE_SIZE;
    }
    for(int i = curr; i < region - 1; i++) {
       to_alloc[i] = to_alloc[i+1];
    }
    region--;
    page_table->load();
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    int min = base_address;
    int max = base_address + size;
    if((_address < max) && (_address >= min)) {
       return true;
    }
    return false;
}

