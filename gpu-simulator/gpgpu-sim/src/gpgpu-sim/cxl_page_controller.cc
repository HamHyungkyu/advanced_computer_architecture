#include "cxl_page_controller.h"
#include <iostream>

void cxl_page_controller::access_count_up(mem_fetch *mf) {
  new_addr_type addr = mem_fetch_to_page_addr(mf);
  int gpu_num = mf->get_gpu_id();

  if(page_table.find(addr) == page_table.end()) {
    page_table[addr] = generate_initial_page_element();
  } 
  else {     
    assert( page_table[addr].status < cxl_page_status::MAX);
    switch(mf->get_type()){
      case READ_REQUEST: 
        page_table[addr].read_counter[gpu_num] += 1;
        break;
      case WRITE_REQUEST:
        page_table[addr].write_counter[gpu_num] += 1;
        break;
      case CXL_WRITE_BACK:
        page_table[addr].count_write_back += 1;
        if(page_table[addr].count_write_back == MEMFETCH_PER_PAGE){
          reset_page(mf);
        }
        break;
    }
  }
}

void cxl_page_controller::reset_page(mem_fetch *mf) {
  new_addr_type addr = mem_fetch_to_page_addr(mf);
  assert(page_table.find(addr) != page_table.end());
  page_table[addr] = generate_initial_page_element();
}

bool cxl_page_controller::check_readonly_migration(mem_fetch *mf) {
  cxl_page_element element = page_table[mem_fetch_to_page_addr(mf)];
  int gpu_num = mf->get_gpu_id();
  bool read_threshold = element.read_counter[gpu_num] > threshold;
  return read_threshold && check_all_zero_except_one(element.write_counter, -1); 
}

void cxl_page_controller::set_shared_gpu(mem_fetch *mf) {
  new_addr_type addr = mem_fetch_to_page_addr(mf);
  assert(page_table.find(addr) != page_table.end());
  cxl_page_element element = page_table[addr];

  if(mf->get_type() == mf_type::CXL_READ_ONLY_MIGRATION_AGREE){
    element.shared_gpu[mf->get_gpu_id()] = true;
  }
  else if(mf->get_type() == mf_type::CXL_WIRTABLE_MIGRATION_AGREE){
    element = generate_initial_page_element();
    element.status = cxl_page_status::WRITE_SHARE;
    element.shared_gpu[mf->get_gpu_id()] = true;
  }
  else{
    assert(0);
  }
}

bool cxl_page_controller::check_writeable_migration(mem_fetch *mf){
  cxl_page_element element = page_table[mem_fetch_to_page_addr(mf)];
  int gpu_num = mf->get_gpu_id();
  bool write_threshold = true;

  for(int i = 0; i < num_gpus; i++) {
    if(i != gpu_num){
      write_threshold = 
        write_threshold && (element.write_counter[gpu_num] > element.read_counter[i] + threshold);  
    }
  }
  return write_threshold && check_all_zero_except_one(element.write_counter, gpu_num);
}

mem_fetch* cxl_page_controller::generate_migration_request(mem_fetch *mf, bool read_only) {
  mem_fetch* request;
  new_addr_type addr = mem_fetch_to_page_addr(mf);
  cxl_page_element element = page_table[mem_fetch_to_page_addr(mf)];
  if(!element.shared_gpu[mf->get_gpu_id()] ){
    const mem_access_t *access = new mem_access_t(mem_access_type::CXL_MESSAGE, addr, 0, false, mf->get_gpgpu_context());
    request = new mem_fetch(*access, *cycles, mf);

    if(read_only) {
      request->set_read_only_migartion_request();
    }
    else {
      request->set_writable_migartion_request();
    }
  }
  return request;
}

mem_fetch** cxl_page_controller::generate_page_read_requests(mem_fetch *mf) {
  new_addr_type addr = mem_fetch_to_page_addr(mf);
  mem_fetch **page_read_requests = (mem_fetch**) malloc(sizeof(mem_fetch*) * MEMFETCH_PER_PAGE);
  for(int i = 0; i < MEMFETCH_PER_PAGE; i++) {
    const mem_access_t *access = new mem_access_t(mem_access_type::CXL_MIGRATION,
      addr + MEMFETCH_BYTES* i, MEMFETCH_BYTES, true, mf->get_gpgpu_context());
    page_read_requests[i] = new mem_fetch(*access, *cycles, mf);
  }
  return page_read_requests;
}

bool cxl_page_controller::check_all_zero_except_one(int *arr, int exception) {
  bool result = true;

  for(int i = 0; i < num_gpus; i++) {
    if(i != exception){
      result = result && (arr[i] == 0);
    }
  }
  return result;
}

cxl_page_element cxl_page_controller::generate_initial_page_element() {
  cxl_page_element element;
  element.count_write_back = 0;
  element.status = cxl_page_status::READ_SHARE;

  for(int i = 0; i < num_gpus; i++) {
    element.shared_gpu[i] = false;
    element.read_counter[i] = 0;
    element.write_counter[i] = 0;
  }
  return element;
}

new_addr_type cxl_page_controller::mem_fetch_to_page_addr(mem_fetch *mf) {
  //Page 4KB
  return  (mf->get_addr() >> 12) << 12;
}