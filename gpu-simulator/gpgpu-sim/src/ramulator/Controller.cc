#include "Controller.h"
#include "HBM.h"
#include "GDDR5.h"

using namespace ramulator;

namespace ramulator
{

template<>
void Controller<GDDR5>::fake_ideal_DRAM(const Config& configs) {
    if (configs["no_DRAM_latency"] == "true") {
      no_DRAM_latency = true;
      scheduler->type = Scheduler<GDDR5>::Type::FRFCFS;
    }
    if (configs["unlimit_bandwidth"] == "true") {
      unlimit_bandwidth = true;
      printf("nBL: %d\n", channel->spec->speed_entry.nBL);
      channel->spec->speed_entry.nBL = 0;
      channel->spec->read_latency = channel->spec->speed_entry.nCL;
      channel->spec->speed_entry.nCCDS = 1;
      channel->spec->speed_entry.nCCDL = 1;
    }
}

template<>
void Controller<HBM>::fake_ideal_DRAM(const Config& configs) {
    if (configs["no_DRAM_latency"] == "true") {
      no_DRAM_latency = true;
      scheduler->type = Scheduler<HBM>::Type::FRFCFS;
    }
    if (configs["unlimit_bandwidth"] == "true") {
      unlimit_bandwidth = true;
      printf("nBL: %d\n", channel->spec->speed_entry.nBL);
      channel->spec->speed_entry.nBL = 0;
      channel->spec->read_latency = channel->spec->speed_entry.nCL;
      channel->spec->speed_entry.nCCDS = 1;
      channel->spec->speed_entry.nCCDL = 1;
    }
}


} /* namespace ramulator */
