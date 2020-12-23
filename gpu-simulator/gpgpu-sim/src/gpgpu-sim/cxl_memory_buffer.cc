#include "cxl_memory_buffer.h"
#include <fstream>
#include <iostream>

cxl_memory_buffer::cxl_memory_buffer(cxl_memory_buffer_config* config){
  m_config = config;
  tot_cycles = 0;
  memory_cycles = 1;
  page_controller = new cxl_page_controller(&tot_cycles, m_config->num_gpus, m_config->migration_threshold);
  gpu_memory_cycle_ratio = (double) m_config->gpu_cycle_frequency / (double) m_config->memory_cycle_frequency;
  nvlinks = (NVLink**)malloc(sizeof(NVLink*) * m_config->num_gpus *
                             m_config->links_per_gpu);
  ramulators = (Ramulator**)malloc(sizeof(Ramulator*) * m_config->num_memories);
  
  for (int i = 0; i < m_config->num_memories; i++) {
    ramulators[i] = new Ramulator(i, &tot_cycles, m_config->num_gpus,
                              m_config->ramulator_config_file);
  }
}

void cxl_memory_buffer::cycle() {
  process_migration_requests();
  process_mem_fetch_request_from_gpu();
  process_mem_fetch_request_to_gpu();

  for (int i = 0; i < m_config->num_memories; i++) {
    // Todo: Ramulator cycle mask
    if(check_memory_cycle()){
      ramulators[i]->cycle();
      memory_cycles++;
    }
  }
  tot_cycles++;
}

void cxl_memory_buffer::process_mem_fetch_request_from_gpu() {
  int num_links = m_config->num_gpus * m_config->links_per_gpu;
  
  for (int i = 0; i < num_links; i++) {
    // Process memory fetch request that from GPU to CXL memory buffer
    if (!nvlinks[i]->to_cxl_buffer_empty() && !ramulators[0]->from_gpgpusim_full()) {
      mem_fetch* from_gpu_mem_fetch = nvlinks[i]->to_cxl_buffer_pop();

      if(from_gpu_mem_fetch->get_type() == mf_type::CXL_READ_ONLY_MIGRATION_AGREE || 
        from_gpu_mem_fetch->get_type() == mf_type::CXL_WIRTABLE_MIGRATION_AGREE){
        //push_migration_requests(page_controller->generate_page_read_requests(from_gpu_mem_fetch));
        page_controller->set_shared_gpu(from_gpu_mem_fetch);
      }
      else {
        from_gpu_mem_fetch->set_status(mem_fetch_status::IN_CXL_MEMORY_BUFFER, tot_cycles);
        ramulators[0]->push(from_gpu_mem_fetch);
        //Page controller job
        page_controller->access_count_up(from_gpu_mem_fetch);
        if(page_controller->check_writeable_migration(from_gpu_mem_fetch)){
          push_nvlink(i, page_controller->generate_migration_request(from_gpu_mem_fetch, true));
        }
        else if(page_controller->check_readonly_migration(from_gpu_mem_fetch)){
          push_nvlink(i, page_controller->generate_migration_request(from_gpu_mem_fetch, false));
        }
      }
    }
  }
}

void cxl_memory_buffer::process_mem_fetch_request_to_gpu() {
  int num_links = m_config->num_gpus * m_config->links_per_gpu;

  for(int i = 0; i < m_config->num_memories; i++){
    // Process memory fetch request that from  CXL memory buffer to GPU until ramulator return queue empty
    while(ramulators[i]->return_queue_top()){
      mem_fetch* to_gpu_mem_fetch = ramulators[i]->return_queue_pop();
      int to_gpu_link_num = to_gpu_mem_fetch->get_gpu_id();
      to_gpu_mem_fetch->set_status(mem_fetch_status::IN_SWITCH_TO_NVLINK, tot_cycles);
      push_nvlink(to_gpu_link_num, to_gpu_mem_fetch);  
    }
    
    while(!overflow_buffer.empty()){
      ramulators[i]->return_queue_push_back(overflow_buffer.front());
      overflow_buffer.pop_front();
    }
  }
}

void cxl_memory_buffer::process_migration_requests() {
  while(!page_migration_request_buffer.empty() && !ramulators[0]->from_gpgpusim_full()) {
    ramulators[0]->push(page_migration_request_buffer.front());
    page_migration_request_buffer.pop_front();
  }
}

bool cxl_memory_buffer::check_memory_cycle(){
  double current_ratio = (double) tot_cycles / (double) memory_cycles;
  
  if(gpu_memory_cycle_ratio < current_ratio){
    return true;
  }
  return false; 
}

void cxl_memory_buffer::push_nvlink(int link_num, mem_fetch* mf) {
  if(!nvlinks[link_num]->from_cxl_buffer_full()){
    nvlinks[link_num]->push_from_cxl_buffer(mf);
  }
  else{
    overflow_buffer.push_back(mf);
  }
}

void cxl_memory_buffer::push_migration_requests(mem_fetch **requests) { 
  for(int i = 0; i < MEMFETCH_PER_PAGE; i++) { 
    page_migration_request_buffer.push_back(requests[i]);
  }
  delete requests;
}

/*
 * CXL memory buffer configs
 */
cxl_memory_buffer_config::cxl_memory_buffer_config(std::string config_path) {
  std::ifstream file(config_path);
  assert(file.good() && "Bad config file");
  std::string line;
  while (getline(file, line)) {
    char delim[] = " \t=";
    vector<string> tokens;

    while (true) {
      size_t start = line.find_first_not_of(delim);
      
      if (start == string::npos) break;
      
      size_t end = line.find_first_of(delim, start);
      
      if (end == string::npos) {
        tokens.push_back(line.substr(start));
        break;
      }
      tokens.push_back(line.substr(start, end - start));
      line = line.substr(end);
    }

    // empty line
    if (!tokens.size()) continue;

    // comment line
    if (tokens[0][0] == '#') continue;

    // parameter line
    assert(tokens.size() == 2 && "Only allow two tokens in one line");

    parse_to_const(tokens[0], tokens[1]);
  }
}

void cxl_memory_buffer_config::parse_to_const(const string& name,
                                              const string& value) {
  if (name == "num_gpus") {
    num_gpus = atoi(value.c_str());
  } 
  else if (name == "num_memories") {
    num_memories = atoi(value.c_str());
  } else if (name == "links_per_gpu") {
    links_per_gpu = atoi(value.c_str());
  } else if (name == "gpu_cycle_frequency") {
    gpu_cycle_frequency = atoi(value.c_str());
  } else if (name == "memory_cycle_frequency"){
    memory_cycle_frequency = atoi(value.c_str());
  } else if (name == "ramulator_config") {
    ramulator_config_file = value;
  } else if (name == "migration_threshold") {
    migration_threshold = atoi(value.c_str());
  }
}