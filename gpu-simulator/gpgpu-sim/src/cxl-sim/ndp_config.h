#ifndef NDP_CONFIG
#define NDP_CONFIG

#include <string>
#include "simd_unit.h"

typedef struct {
  int packet_size;
  int subtensor_size;
  int ndp_table_size;
  int num_simd_units;
  int simd_width;
  int simd_op_latencies[simd_op_type::MAX];
  int scratch_pad_memory_size;
  int num_vector_op_status_regs;
  int num_packet_slots;
} ndp_config;

#endif