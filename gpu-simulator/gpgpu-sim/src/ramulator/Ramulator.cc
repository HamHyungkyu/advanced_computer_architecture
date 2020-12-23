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

// static map<string, function<MemoryBase *(const Config&, int, unsigned,
// string, fifo_pipeline<mem_fetch>*)>> name_to_func = {
static map<string, function<MemoryBase*(const Config&, int, unsigned,
                                        fifo_pipeline<mem_fetch>*)>>
    name_to_func = {
        {"GDDR5", &MemoryFactory<GDDR5>::create},
        {"HBM", &MemoryFactory<HBM>::create},
        {"PCM", &MemoryFactory<PCM>::create},
};

Ramulator::Ramulator(unsigned memory_id, unsigned long long* cycles,
                     unsigned num_cores,
                     std::string ramulator_config  // config file path
                     )
    : ramulator_configs(ramulator_config),
      read_cb_func(
          std::bind(&Ramulator::readComplete, this, std::placeholders::_1)),
      write_cb_func(
          std::bind(&Ramulator::writeComplete, this, std::placeholders::_1)),
      m_id(memory_id),
      num_cores(num_cores),
      cycles(cycles) {
  int cxl_dram_return_queue_size = 0;
  ramulator_configs.set_core_num(num_cores);
  std_name = ramulator_configs["standard"];
  
  assert(name_to_func.find(std_name) != name_to_func.end() &&
         "unrecognized standard name");

  m_id = memory_id;
  // Todo : quesize
  // init fifo pipelines
  rwq =
      // new fifo_pipeline<mem_fetch>("completed read write queue", 0, 2);
      new fifo_pipeline<mem_fetch>("completed read write queue", 0, 2048);
  returnq = new fifo_pipeline<mem_fetch>(
      "ramulatorreturnq", 0,
      cxl_dram_return_queue_size == 0 ? 1024 : cxl_dram_return_queue_size);
  from_gpgpusim = new fifo_pipeline<mem_fetch>("fromgpgpusim", 0, 1024);

  memory =
      name_to_func[std_name](ramulator_configs, (int)SECTOR_SIZE, m_id, rwq);
  tCK = memory->clk_ns();

  if (!Stats_ramulator::statlist.is_open())
    // Stats_ramulator::statlist.output("./output/ramulator." +
    // get_binary_name() + ".stats");
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
      // Todo :: after define migration access type 
      if (mf->get_access_type() != L1_WRBK_ACC &&
          mf->get_access_type() != L2_WRBK_ACC &&
          mf->get_access_type() != CXL_WRITE_BACK_ACC) {
        mf->set_reply();
        returnq->push(mf);
      } else {
        assert(0);
        // Todo :: after define migration access type 
        delete mf;
      }
    }
  }

  // bool accepted = false;
  // while (!from_gpgpusim->empty()) {
  //   mem_fetch* mf = from_gpgpusim->pop();
  //   if (mf->get_type() == READ_REQUEST) {
  //     assert(mf->is_write() == false);
  //     assert(mf->get_gpu_id() < (unsigned)num_cores);
  //     // Requested data size must be 32
  //     assert(mf->get_data_size() == 32);

  //     Request req(mf->get_addr(),
  //                 Request::Type::R_READ, read_cb_func, mf->get_gpu_id(), mf);
  //     req.mf = mf;
  //     accepted = send(req);
  //   } else if (mf->get_type() == WRITE_REQUEST) {
  //     // GPGPU-sim will send write request only for write back request
  //     // channel_removed_addr: the address bits of channel part are truncated
  //     // channel_included_addr: the address bits of channel part are not
  //     // truncated
  //     Request req(mf->get_addr(),
  //                 Request::Type::R_WRITE, write_cb_func, mf->get_gpu_id(),
  //                 mf);
  //     accepted = send(req);
  //   }

  //   assert(accepted);
  // }

  bool accepted = false;
  while (!from_gpgpusim->empty()) {
    //mem_fetch* mf = from_gpgpusim->pop();
    mem_fetch* mf = from_gpgpusim->top();
    if (mf) {
      if (mf->get_type() == READ_REQUEST || mf->get_type() == CXL_WRITE_BACK) {
      assert(mf->is_write() == false);
      assert(mf->get_gpu_id() < (unsigned)num_cores);
      // Requested data size must be 32
      assert(mf->get_data_size() == 32);

      Request req(mf->get_addr(),
                  Request::Type::R_READ, read_cb_func, mf->get_gpu_id(), mf);
      req.mf = mf;
      accepted = send(req);
      } else if (mf->get_type() == WRITE_REQUEST) {
        // GPGPU-sim will send write request only for write back request
        // channel_removed_addr: the address bits of channel part are truncated
        // channel_included_addr: the address bits of channel part are not
        // truncated
        Request req(mf->get_addr(),
                    Request::Type::R_WRITE, write_cb_func, mf->get_gpu_id(),
                    mf);
        accepted = send(req);
      }

      if (accepted)
        from_gpgpusim->pop();

      
    }    
    if (!accepted)
      break;
  }

  // cycle ramulator
  memory->tick();
}

bool Ramulator::send(Request req) {
  if (rwq->get_length() < rwq->get_max_len() * 0.8f)
    return memory->send(req);
  return false;
}

void Ramulator::push(class mem_fetch* mf) {
  from_gpgpusim->push(mf);

  // Todo:memory stats
  // this function is related to produce average latency stat
  // m_stats->memlatstat_dram_access(mf);
}

bool Ramulator::returnq_full() const { return returnq->full(); }
mem_fetch* Ramulator::return_queue_top() const { return returnq->top(); }
mem_fetch* Ramulator::return_queue_pop() const { return returnq->pop(); }
void Ramulator::return_queue_push_back(mem_fetch* mf) { returnq->push(mf);}
void Ramulator::readComplete(Request& req) {
  assert(req.mf != nullptr);
  assert(!rwq->full());
  rwq->push(req.mf);
}

void Ramulator::writeComplete(Request& req) {
  assert(req.mf != nullptr);
  rwq->push(req.mf);
}

void Ramulator::finish(FILE* fp, unsigned m_id) { memory->finish(); }

void Ramulator::print(FILE* fp, unsigned m_id) {
  Stats_ramulator::statlist.printall(fp, m_id);
}
