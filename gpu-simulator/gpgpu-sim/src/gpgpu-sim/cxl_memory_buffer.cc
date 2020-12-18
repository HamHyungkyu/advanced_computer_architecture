#include "cxl_memory_buffer.h"
#include <fstream>
#include <iostream>

cxl_memory_buffer::cxl_memory_buffer(cxl_memory_buffer_config* config) {
  m_config = config;
  nvlinks = (NVLink**)malloc(sizeof(NVLink*) * m_config->num_gpus *
                             m_config->links_per_gpu);
  ramulators = (Ramulator*)malloc(sizeof(Ramulator) * m_config->num_memories);
  for (int i = 0; i < m_config->num_memories; i++) {
    ramulators[i] = Ramulator(i, &tot_cycles, m_config->num_gpus,
                              m_config->ramulator_config_file);
  }
}

void cxl_memory_buffer::cycle() {
  int num_links = m_config->num_gpus * m_config->links_per_gpu;
  for (int i = 0; i < num_links; i++) {
    // Process memory fetch request that from GPU to CXL memory buffer
    mem_fetch* from_gpu_mem_fetch = nvlinks[i]->from_GPU_pop();
    if (from_gpu_mem_fetch) {
      from_gpu_mem_fetch->set_status(mem_fetch_status::IN_CXL_MEMORY_BUFFER,
                                     tot_cycles);
      ramulators[0].push(from_gpu_mem_fetch);
    }

    // Process memory fetch request that from CXL memory buffer to GPU
    mem_fetch* to_gpu_mem_fetch = ramulators[0].return_queue_top();
    if (to_gpu_mem_fetch) {
      to_gpu_mem_fetch->set_status(mem_fetch_status::IN_SWITCH_TO_NVLINK,
                                   tot_cycles);
      nvlinks[i]->push_from_ramulator(to_gpu_mem_fetch);
    }
    ramulators[0].return_queue_pop();
  }
  for (int i = 0; i < m_config->num_memories; i++) {
    // Todo: Ramulator cycle mask
    ramulators[i].cycle();
  }
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
  } else if (name == "ramulator_config") {
    ramulator_config_file = value;
  }
}