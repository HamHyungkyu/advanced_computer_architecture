#ifndef CXL_MEMORY_BUFFER
#define CXL_MEMORY_BUFFER
#include <string>
#include <deque>
#include "../ramulator/Ramulator.h"
#include "NVLink.h"
#include "cxl_page_controller.h"
#include "delayqueue.h"
class cxl_memory_buffer_config;

class cxl_memory_buffer {
 public:
  cxl_memory_buffer(cxl_memory_buffer_config *config);
  ~cxl_memory_buffer();
  void cycle();
  void set_NVLink(int link_num, NVLink *link) { nvlinks[link_num] = link; }

 private:
  unsigned long long tot_cycles;
  unsigned long long memory_cycles;
  double gpu_memory_cycle_ratio;
  cxl_memory_buffer_config *m_config;
  Ramulator **ramulators;
  NVLink **nvlinks;
  std::deque<mem_fetch*> overflow_buffer;
  std::deque<mem_fetch*> page_migration_request_buffer;
  cxl_page_controller *page_controller;
  
  bool check_memory_cycle();
  void process_mem_fetch_request_from_gpu();
  void process_mem_fetch_request_to_gpu();
  void process_migration_requests();
  void push_nvlink(int link_num, mem_fetch* mf);
  void push_migration_requests(mem_fetch** requests);
};

class cxl_memory_buffer_config {
 public:
  cxl_memory_buffer_config(std::string config_path, int num_gpus);

  int get_links_per_gpu() { return links_per_gpu; }

 private:
  int num_memories;
  int num_gpus;
  int links_per_gpu;
  int gpu_cycle_frequency;
  int memory_cycle_frequency;
  int migration_threshold;
  std::string ramulator_config_file;
  
  void parse_to_const(const string &name, const string &value);

  friend cxl_memory_buffer;
};
#endif