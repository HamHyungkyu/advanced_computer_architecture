#ifndef NDP_CONFIG
#define NDP_CONFIG

#include <string>
#include "simd_unit.h"

class ndp_config {
  public:
    ndp_config(std::string config_file);
  private:
    int packet_size;
    int subtensor_size;
    int ndp_table_size;
    
    //SIMD config
    int num_simd_units;
    int simd_width;
    int simd_op_latencies[simd_op_type::MAX];
    int scratch_pad_memory_size;
    
    friend ndp_unit;
};

#endif