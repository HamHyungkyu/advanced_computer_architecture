#include <string>

#include "Ramulator.h"


#include "GDDR5.h"
#include "HBM.h"
#include "PCM.h"

#include "../gpgpu-sim/l2cache.h"

// to load binary name
#include "../cuda-sim/ptx_loader.h"

#include "../gpgpu-sim/mem_latency_stat.h"

using namespace ramulator;

//static map<string, function<MemoryBase *(const Config&, int, unsigned, string, fifo_pipeline<mem_fetch>*)>> name_to_func = {
static map<string, function<MemoryBase *(const Config&, int, unsigned, fifo_pipeline<mem_fetch>*)>> name_to_func = {
    {"GDDR5", &MemoryFactory<GDDR5>::create}, 
    {"HBM", &MemoryFactory<HBM>::create},
    {"PCM", &MemoryFactory<PCM>::create},
};


Ramulator::Ramulator(unsigned partition_id, 
                     const struct memory_config* config,
                     class memory_stats_t *stats, 
                     unsigned long long *gpu_sim_cycle,
                     unsigned long long *gpu_tot_sim_cycle,
                     unsigned num_cores,
                     std::string ramulator_config,  // config file path
                     class memory_partition_unit *mp)
  : ramulator_configs(ramulator_config), m_stats(stats), m_config(config),
    read_cb_func(std::bind(&Ramulator::readComplete, this, std::placeholders::_1)),
    write_cb_func(std::bind(&Ramulator::writeComplete, this, std::placeholders::_1)),
    m_id(partition_id), num_cores(num_cores), m_memory_partition_unit(mp),
    gpu_sim_cycle(gpu_sim_cycle), gpu_tot_sim_cycle(gpu_tot_sim_cycle) {

  ramulator_configs.set_core_num(num_cores);
  std_name = ramulator_configs["standard"];
  assert(name_to_func.find(std_name) != name_to_func.end() &&
         "unrecognized standard name");

  m_id = partition_id;
  m_memory_partition_unit = mp;

  // init fifo pipelines
  rwq = 
    //new fifo_pipeline<mem_fetch>("completed read write queue", 0, 2);
    new fifo_pipeline<mem_fetch>("completed read write queue", 0, 1024);
  returnq = 
    new fifo_pipeline<mem_fetch>("ramulatorreturnq", 0, 
                                 config->gpgpu_dram_return_queue_size == 0 ?
                                 1024 : config->gpgpu_dram_return_queue_size);
  from_gpgpusim =
    new fifo_pipeline<mem_fetch>("fromgpgpusim", 0, 2);


  memory = name_to_func[std_name](ramulator_configs, (int)SECTOR_SIZE, m_id, rwq);
  tCK = memory->clk_ns();

  if (!Stats_ramulator::statlist.is_open())
    //Stats_ramulator::statlist.output("./output/ramulator." + get_binary_name() + ".stats");
    Stats_ramulator::statlist.output("./output/ramulator.stats");
}
Ramulator::~Ramulator() {}

bool Ramulator::full(bool is_write, unsigned long req_addr) {
  return memory->full(is_write, req_addr);
}

void Ramulator::cycle() {
  if (!returnq_full()) {
    mem_fetch* mf = rwq->pop();
    if (mf) {
      //if (m_config->dram_atom_size >= mf->get_data_size()) {
        mf->set_status(IN_PARTITION_MC_RETURNQ, (*gpu_sim_cycle) + (*gpu_tot_sim_cycle));
        if (mf->get_access_type() != L1_WRBK_ACC && mf->get_access_type() != L2_WRBK_ACC) {
          mf->set_reply();
          returnq->push(mf);
        } else {
          m_memory_partition_unit->set_done(mf);
          delete mf;
        }
      //}
    }
  }

  bool accepted = false;
  while (!from_gpgpusim->empty()) {
    mem_fetch* mf = from_gpgpusim->pop();

    if (mf->get_type() == READ_REQUEST) {
      assert (mf->is_write() == false);
      assert (mf->get_sid() < (unsigned) num_cores);
      // Requested data size must be 32
      assert (mf->get_data_size() == 32);

      Request req(mf->get_tlx_addr().channel_removed_addr, Request::Type::R_READ, read_cb_func, mf->get_sid(), mf);
      //req.mf = mf;
      accepted = send(req);
    } else if (mf->get_type() == WRITE_REQUEST) {
      // GPGPU-sim will send write request only for write back request
      // channel_removed_addr: the address bits of channel part are truncated
      // channel_included_addr: the address bits of channel part are not truncated
      if ((unsigned)mf->get_sid() == -1) {
        Request req(mf->get_tlx_addr().channel_removed_addr, Request::Type::R_WRITE, write_cb_func, num_cores, mf);
        //req.mf = mf;
        accepted = send(req);
      } else {
        Request req(mf->get_tlx_addr().channel_removed_addr, Request::Type::R_WRITE, write_cb_func, mf->get_sid(), mf);
        //req.mf = mf;
        accepted = send(req);
      }
    }

    assert (accepted);
  }
  
  // cycle ramulator
  memory->tick();
}


bool Ramulator::send(Request req) {
  return memory->send(req);
}

void Ramulator::push(class mem_fetch* mf) {
  mf->set_status(IN_PARTITION_MC_INTERFACE_QUEUE, (*gpu_sim_cycle) + (*gpu_tot_sim_cycle));
  from_gpgpusim->push(mf);

  // this function is related to produce average latency stat
  m_stats->memlatstat_dram_access(mf);
}

bool Ramulator::returnq_full() const {
  return returnq->full();
}
mem_fetch* Ramulator::return_queue_top() const {
  return returnq->top();
}
mem_fetch* Ramulator::return_queue_pop() const {
  return returnq->pop();
}

void Ramulator::readComplete(Request& req) {
  assert(req.mf != nullptr);
  rwq->push(req.mf);
}

void Ramulator::writeComplete(Request& req) {
  assert(req.mf != nullptr);
  rwq->push(req.mf);
}

void Ramulator::finish(FILE* fp, unsigned m_id) {
  memory->finish();
}

void Ramulator::print(FILE* fp, unsigned m_id) {
  assert (m_id == m_config->m_n_mem - 1);
  Stats_ramulator::statlist.printall(fp, m_id);
}
