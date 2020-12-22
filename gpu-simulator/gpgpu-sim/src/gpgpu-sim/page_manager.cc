#include "page_manager.h"

bool page_manager::gpu_full() {
   if (pages.size() < max_gpu_pages)
      return false;

   return true;
}

void page_manager::alloc_page(unsigned long long addr, page_location loc, bool read_only) {
   unsigned long long vpn_for_req = addr >> 12;

   if (loc == page_location::GPU) {
      if (gpu_full()) {
         assert(0);
      }
   }

   page_entry new_page;
   new_page.vpn = vpn_for_req;
   new_page.location = loc;
   new_page.read_only = read_only;
   new_page.status = page_status::PENDING_MIGRATION;
   new_page.transfer_counter = 0;
   pages.push_back(new_page);
}

page_location page_manager::is_allocated(unsigned long long addr) {
   unsigned long long vpn_for_req = addr >> 12;

   std::list<page_entry>::iterator it;
   for (it = pages.begin(); it != pages.end(); it++) {
      if (it->vpn == vpn_for_req && it->status) {
         return it->location;
      }
   }

   return page_location::CXL;
}

bool page_manager::gpu_page_access(mem_fetch *mf) {
   assert(is_allocated(mf->get_addr())==page_location::GPU);
   unsigned long long vpn_for_req = mf->get_addr() >> 12;

   std::list<page_entry>::iterator it;
   for (it = pages.begin(); it != pages.end(); it++) {
      if (it->vpn == vpn_for_req) {
         if(it->status == page_status::PAGE_VALID){
            pages.erase(it);
         }
         else {
            return false;
         }
      }
   }
   if(!(it->read_only && mf->get_type() == mf_type::WRITE_REQUEST)){
      pages.push_back(*it);
   }
   return true;
}

void page_manager::set_max_gpu_pages(int max) {
   assert(max!=-1);
   max_gpu_pages = max;
}

page_entry page_manager::evict_LRU_page(){
   assert(gpu_full());
   page_entry entry = pages.front();
   pages.pop_front(); 
   return entry;
}

void page_manager::save_migration(mem_fetch *mf) {
   assert(is_allocated(mf->get_addr())==page_location::GPU);
   unsigned long long vpn_for_req = mf->get_addr() >> 12;
   std::list<page_entry>::iterator it;

   for (it = pages.begin(); it != pages.end(); it++) {
      if (it->vpn == vpn_for_req) {
         assert(it->status == page_status::PENDING_MIGRATION);
         it->transfer_counter ++;
         if(it->transfer_counter == PAGESIZE/MEMFETCH_SIZE){
            it->status = page_status::PAGE_VALID;
         }
      }
   }
}


void page_manager::push_write_back_requests(int cycles, mem_fetch* mf) {
   assert(is_allocated(mf->get_addr())==page_location::GPU);
   unsigned long long vpn_for_req = mf->get_addr() >> 12;
   std::list<page_entry>::iterator it;

   for (it = pages.begin(); it != pages.end(); it++) {
      if (it->vpn == vpn_for_req) {
         it->status = page_status::PENDING_WRITE_BACK;
         it->location = page_location::CXL;
         for(int i = 0; i < PAGESIZE/MEMFETCH_SIZE; i++){
            new_addr_type addr = vpn_for_req << 12;
            mem_access_t *access = new mem_access_t(mem_access_type::CXL_WRITE_BACK_ACC, addr + i* MEMFETCH_SIZE,
               MEMFETCH_SIZE, false, NULL);
            mem_fetch *write_back_mf = new mem_fetch(*access, cycles, mf);
            write_back_buffer.push_back(mf);
         }
         mf->set_migration_agree();
         write_back_buffer.push_back(mf);
      }
   }
}
mem_fetch* page_manager::pop_write_back_buffer() {
   mem_fetch *result = write_back_buffer.front();
   write_back_buffer.pop_front();
   return result;
};
