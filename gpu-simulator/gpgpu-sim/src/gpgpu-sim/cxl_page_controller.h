#ifndef CXL_PAGE_CONTROLLER
#define CXL_PAGE_CONTROLLER
#include <map>
#include "../abstract_hardware_model.h"
#include "mem_fetch.h"


// Page sise = 4KB, mem_fetch size = 32bytes 
#define MEMFETCH_PER_PAGE 128 
#define MEMFETCH_BYTES 32

enum cxl_page_status {
  PAGE_INVALID,
  READ_SHARE,
  WRITE_SHARE,
  PENDING_WRITE_DATA,
  PENDING_READ_INVALIDATION
};

typedef struct {
  cxl_page_status status;
  bool shared_gpu[8];
  int read_counter[8];
  int write_counter[8];
  int count_write_back; // Count write back mem_fetch to determine whether writeback finished or not
} cxl_page_element;

class cxl_page_controller {
  public:
  cxl_page_controller(unsigned long long* cycles, int num_gpus, int threshold)
    :cycles(cycles), num_gpus(num_gpus), threshold(threshold){} 
  void access_count_up(mem_fetch *mf);
  void reset_page(mem_fetch *mf);
  bool check_readonly_migration(mem_fetch *mf);
  bool check_writeable_migration(mem_fetch *mf);
  mem_fetch* generate_migration_request(mem_fetch *mf, bool read_only);
  mem_fetch** generate_page_read_requests(mem_fetch *mf);
  
  private:
  unsigned long long *cycles;
  int threshold;
  const int num_gpus;
  std::map<new_addr_type, cxl_page_element> page_table;

  //Helper functions
  bool check_all_zero_except_one(int* arr, int exception);
  cxl_page_element generate_initial_page_element();
  new_addr_type mem_fetch_to_page_addr(mem_fetch *mf);
};

#endif