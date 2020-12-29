#include <list>
#include <cassert>
#include <iostream>
#include "mem_fetch.h"
#include "icnt_wrapper.h"
#include "../abstract_hardware_model.h"

#define PAGESIZE 4096
#define MEMFETCH_SIZE 32

enum page_location {
   not_allocated=-1, GPU, CXL
};

enum page_status {
   PAGE_VALID, PENDING_WRITE_BACK, PENDING_MIGRATION
};

typedef struct {
   page_status status;
   unsigned long long vpn;
   page_location location; //0: in GPU local mem, 1: in memory buffer
   bool read_only;
   int transfer_counter;
} page_entry;

class page_manager {
public:
   bool gpu_full();
   void alloc_page(unsigned long long addr, page_location loc, bool read_only);
   page_location is_allocated(unsigned long long addr);
   bool gpu_page_access(mem_fetch *mf);
   void set_max_gpu_pages(int max);
   page_entry evict_LRU_page(); //LRU
   page_entry LRU_page_entry() {return pages.front();}; //LRU
   void save_migration(mem_fetch *mf);
   void push_write_back_requests(int cycels, mem_fetch* mf);
   bool is_write_back_buffer_empty() {return write_back_buffer.empty();};
   mem_fetch* pop_write_back_buffer();
private:
   std::list<page_entry> pages;
   std::list<mem_fetch*> write_back_buffer;
   int max_gpu_pages; //the number of pages that can be allocated to GPU memory. GPU mem size / page size
};