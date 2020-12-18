#include "page_manager.h"

bool page_manager::gpu_full() {
   if (pages.size() < max_gpu_pages)
      return false;

   return true;
}

void page_manager::alloc_page(unsigned long long addr, page_location loc) {
   unsigned long long vpn_for_req = addr >> 12;

   if (loc == page_location::GPU) {
      if (gpu_full()) {
         assert(0);//
      }
      gpu_pages++;
   }

   page_entry new_page;
   new_page.vpn = vpn_for_req;
   new_page.location = loc;
   
   pages.push_back(new_page);
}

page_location page_manager::is_allocated(unsigned long long addr) {
   unsigned long long vpn_for_req = addr >> 12;

   std::list<page_entry>::iterator it;
   for (it = pages.begin(); it != pages.end(); it++) {
      if (it->vpn == vpn_for_req) {
         return it->location;
      }
   }

   return page_location::not_allocated;
}

void page_manager::set_max_gpu_pages(int max) {
   assert(max!=-1);
   max_gpu_pages = max;
}