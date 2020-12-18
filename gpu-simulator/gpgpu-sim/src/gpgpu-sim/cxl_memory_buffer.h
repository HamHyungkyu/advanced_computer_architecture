#ifndef CXL_MEMORY_BUFFER
#define CXL_MEMORY_BUFFER
#include <string>
#include "../ramulator/Ramulator.h"
#include "delayqueue.h"
class cxl_memory_buffer_config;

class cxl_memory_buffer {
 public:
  cxl_memory_buffer(cxl_memory_buffer_config *config);
  ~cxl_memory_buffer();
  void cycle();

 private:
  unsigned long long tot_cycles;
  cxl_memory_buffer_config *m_config;
  Ramulator *ramulators;
};

class cxl_memory_buffer_config {
 public:
  cxl_memory_buffer_config(std::string config_path);
  ~cxl_memory_buffer_config();

 private:
  int num_memories;
  int num_gpus;
  int links_per_gpu;
  std::string ramulator_config_file;

  void parse_to_const(const string &name, const string &value);

  friend cxl_memory_buffer;
};

#endif