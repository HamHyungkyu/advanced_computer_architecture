#include "cxl_memory_buffer.h"
#include <fstream>
#include <iostream>

cxl_memory_buffer::cxl_memory_buffer(cxl_memory_buffer_config* config) {
  m_config = config;
  tot_cycles = 0;
  memory_cycles = 1;
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
  int num_links = m_config->num_gpus * m_config->links_per_gpu;
  
  for (int i = 0; i < num_links; i++) {
    // Process memory fetch request that from GPU to CXL memory buffer
    if (!nvlinks[i]->to_cxl_buffer_empty() && !ramulators[0]->from_gpgpusim_full()) {
      mem_fetch* from_gpu_mem_fetch = nvlinks[i]->to_cxl_buffer_pop();
      from_gpu_mem_fetch->set_status(mem_fetch_status::IN_CXL_MEMORY_BUFFER,
                                     tot_cycles);
      ramulators[0]->push(from_gpu_mem_fetch);
    }
  }

  for(int i = 0; i < m_config->num_memories; i++){
    // Process memory fetch request that from  CXL memory buffer to GPU until ramulator return queue empty
    while(ramulators[i]->return_queue_top()){
      mem_fetch* to_gpu_mem_fetch = ramulators[i]->return_queue_pop();
      int to_gpu_link_num = to_gpu_mem_fetch->get_gpu_id();
      to_gpu_mem_fetch->set_status(mem_fetch_status::IN_SWITCH_TO_NVLINK, tot_cycles);
      if(!nvlinks[to_gpu_link_num]->from_cxl_buffer_full()){
        nvlinks[to_gpu_link_num]->push_from_cxl_buffer(to_gpu_mem_fetch);
      }
      else{
        overflow_buffer.push_back(to_gpu_mem_fetch);
      }
    }
    while(!overflow_buffer.empty()){
      ramulators[i]->return_queue_push_back(overflow_buffer.front());
      overflow_buffer.pop_front();
    }
  }

  for (int i = 0; i < m_config->num_memories; i++) {
    // Todo: Ramulator cycle mask
    if(check_memory_cycle()){
      ramulators[i]->cycle();
      memory_cycles++;
    }
  }
  tot_cycles++;
}

bool cxl_memory_buffer::check_memory_cycle(){
  double current_ratio = (double) tot_cycles / (double) memory_cycles;
  if(gpu_memory_cycle_ratio < current_ratio){
    return true;
  }
  return false; 
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
  } else if (name == "num_memories") {
    num_memories = atoi(value.c_str());
  } else if (name == "links_per_gpu") {
    links_per_gpu = atoi(value.c_str());
  } else if (name == "gpu_cycle_frequency") {
    gpu_cycle_frequency = atoi(value.c_str());
  } else if (name == "memory_cycle_frequency"){
    memory_cycle_frequency = atoi(value.c_str());
  } else if (name == "ramulator_config") {
    ramulator_config_file = value;
  }
}