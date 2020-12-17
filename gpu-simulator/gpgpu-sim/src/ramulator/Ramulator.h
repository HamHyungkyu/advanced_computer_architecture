#ifndef RAMULATOR_H_
#define RAMULATOR_H_

#include <string>
#include <deque>
#include <map>
#include <functional>

#include "MemoryFactory.h"
//#include "Memory.h"
#include "Config.h"
//#include "MemoryFactory.h"
//#include "Memory.h"
//#include "Request.h"

#include "../gpgpu-sim/delayqueue.h"
//#include "../gpgpu-sim/mem_fetch.h"
//#include "../gpgpu-sim/l2cache.h"
#include "../gpgpu-sim/gpu-sim.h"

//extern std::vector<std::pair<unsigned long, unsigned long>> malloc_list;

namespace ramulator{
class Request;
class MemoryBase;
}

//using namespace ramulator;

class Ramulator {
public:
  Ramulator(unsigned partition_id, 
            const struct memory_config* config,
            class memory_stats_t *stats, 
            unsigned long long *gpu_sim_cycle,
            unsigned long long *gpu_tot_sim_cycle,
            unsigned num_cores,
            std::string ramulator_config,
            class memory_partition_unit *mp);
  ~Ramulator();
  // check whether the read or write queue is available
  bool full(bool is_write, unsigned long req_addr);
  void cycle();

  void finish(FILE* fp, unsigned memory_partition_id);
  void print(FILE* fp, unsigned memory_partition_id);

  // push mem_fetcth object into Ramulator wrapper
  void push(class mem_fetch* mf);

  mem_fetch* return_queue_top() const;
  mem_fetch* return_queue_pop() const;
  bool returnq_full() const; 

  double tCK;

private:
  std::string std_name;
  ramulator::MemoryBase* memory;

  fifo_pipeline<mem_fetch> *rwq;
  fifo_pipeline<mem_fetch> *returnq;
  fifo_pipeline<mem_fetch> *from_gpgpusim;

  std::map<unsigned long long, std::deque<mem_fetch*>> reads;
  std::map<unsigned long long, std::deque<mem_fetch*>> writes;

  unsigned num_cores;
  unsigned m_id;
  unsigned long long *gpu_sim_cycle;
  unsigned long long *gpu_tot_sim_cycle;

  memory_partition_unit *m_memory_partition_unit;

  // callback functions
  std::function<void(ramulator::Request&)> read_cb_func;
  std::function<void(ramulator::Request&)> write_cb_func;
  void readComplete(ramulator::Request& req);
  void writeComplete(ramulator::Request& req);

  // Config - 
  // it parses options from ramulator_config file when it is constructed
  ramulator::Config ramulator_configs;
  class memory_stats_t* m_stats;
  const struct memory_config* m_config;

  bool send(ramulator::Request req);
};


#endif
