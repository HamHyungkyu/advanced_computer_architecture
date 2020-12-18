#include <list>
#include <cassert>

#define PAGESIZE 4096

enum class page_location {
   not_allocated=-1, GPU, CXL
};

typedef struct {
   unsigned long long vpn;
   page_location location; //0: in GPU local mem, 1: in memory buffer
} page_entry;

class page_manager {
public:
   bool gpu_full();

   void alloc_page(unsigned long long addr, page_location loc=page_location::GPU);

   page_location is_allocated(unsigned long long addr);

   void set_max_gpu_pages(int max);


private:
   std::list<page_entry> pages;
   int gpu_pages = 0; //the number of pages allocated in GPU memory.
   int max_gpu_pages; //the number of pages that can be allocated to GPU memory. GPU mem size / page size
};