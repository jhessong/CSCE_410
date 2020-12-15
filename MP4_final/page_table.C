#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"
#include "vm_pool.H"

#define PAGE_DIRECTORY_FRAME_SIZE 1

#define PAGE_PRESENT	1
#define PAGE_WRITE	2
#define PAGE_LEVEL	4

#define PD_SHIFT	22
#define PT_SHIFT	12

#define PDE_MASK	0xFFFFF000
#define PT_MASK		0x3FF

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   PageTable::kernel_mem_pool = _kernel_mem_pool;
   PageTable::process_mem_pool = _process_mem_pool;
   PageTable::shared_size = _shared_size;

   Console::puts("Initialized Paging System\n");
}
/*
	bit 0 - if page is present
	bit 1 - read/write mode
	bit 2 - kernel/user
*/
PageTable::PageTable()
{
   page_directory = (unsigned long*)(kernel_mem_pool->get_frames(PAGE_DIRECTORY_FRAME_SIZE)*PAGE_SIZE);
   unsigned long address_mask = 0;
   unsigned long* page_table_direct = (unsigned long*)(kernel_mem_pool->get_frames(PAGE_DIRECTORY_FRAME_SIZE)*PAGE_SIZE);
   unsigned long sh_frames = (PageTable::shared_size/PAGE_SIZE);

   //mark shared memory with kernel mode, read/write, and present
   for(unsigned long i = 0; i < sh_frames; i++) {
      page_table_direct[i] = address_mask | PAGE_WRITE | PAGE_PRESENT;
      address_mask += PAGE_SIZE;
   }

   page_directory[0] = (unsigned long)page_table_direct | PAGE_WRITE | PAGE_PRESENT;

   address_mask = 0;

   for(unsigned long i = 1; i < sh_frames; i++) {
      page_directory[i] = address_mask | PAGE_WRITE;
   }

   page_directory[sh_frames-1] = (unsigned long)(page_directory) | PAGE_WRITE | PAGE_PRESENT;

   for(int i = 0; i < VM_POOL_SIZE; i++) {
      reg_vm_pool[i] = NULL;
   }

   vm_pool_no = 0;

   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   current_page_table = this;
   write_cr3((unsigned long)page_directory);
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   paging_enabled = 1;
   //set bit for paging - source:http://www.osdever.net/tutorials/view/implementing-basic-paging
   write_cr0(read_cr0() | 0x80000000);
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
   unsigned long* curr_pd = (unsigned long*)0xFFFFF000;
   unsigned long page = read_cr2();
   unsigned long address_pd = page >> PD_SHIFT;
   unsigned long address_pt = page >> PT_SHIFT;
   unsigned long* table = NULL;
   unsigned long error = _r->err_code;
   unsigned long address_mask = 0;
   //directory - 10 bits, table - 10 bits, offset - 12 bits
   if ((error & PAGE_PRESENT) == 0) {
      int j = -1;
      VMPool** vm_pool = current_page_table->reg_vm_pool;
      for(int i = 0; i < current_page_table->vm_pool_no; i++) {
         if(vm_pool[i] != NULL) {
            if(vm_pool[i]->is_legitimate(page)) {
               j = i;
               break;
            }
         }
      }
      assert(j >= 0);
      if((curr_pd[address_pd] & PAGE_PRESENT) == 1) {
         table = (unsigned long*)(0xFFC00000 | (address_pd << PT_SHIFT));
         table[address_pt & PT_MASK] = (PageTable::process_mem_pool->get_frames(PAGE_DIRECTORY_FRAME_SIZE)*PAGE_SIZE) | PAGE_WRITE | PAGE_PRESENT;
      }
      else {
         curr_pd[address_pd] = (unsigned long)((kernel_mem_pool->get_frames(PAGE_DIRECTORY_FRAME_SIZE)*PAGE_SIZE) | PAGE_WRITE | PAGE_PRESENT);
         table = (unsigned long*)(0xFFC00000 | (address_pd << PT_SHIFT));
         
         for(int i = 0; i < 1024; i++) {
            table[i] = address_mask | PAGE_LEVEL;
         }

         table[address_pt & PT_MASK] = (PageTable::process_mem_pool->get_frames(PAGE_DIRECTORY_FRAME_SIZE)*PAGE_SIZE | PAGE_WRITE | PAGE_PRESENT);
      }
   }
   Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool* _vm_pool) {
   if(vm_pool_no < VM_POOL_SIZE) {
      reg_vm_pool[vm_pool_no++] = _vm_pool;
      Console::puts("VM pool registered\n");
   }
   else {
      Console::puts("VM pool already full, pool could not be registered");
   }
}

void PageTable::free_page(unsigned long _page_no) {
   unsigned long address_pt = _page_no >> PT_SHIFT;
   unsigned long address_pd = _page_no >> PD_SHIFT;

   unsigned long* pt = (unsigned long*)(0xFFC00000 | (address_pd << PT_SHIFT));

   unsigned long frame_number = pt[address_pt & PT_MASK] / (Machine::PAGE_SIZE);
   process_mem_pool->release_frames(frame_number);

   pt[address_pt & PT_MASK] = 0 | PAGE_WRITE;
}

