#ifndef NDP_UNIT
#define NDP_UNIT
#include "ndp_controller.h"
#include "ndp_config.h"
#include "simd_unit.h"
#include "../abstract_hardware_model.h"

enum ndp_op_type {
  ADD, SUB, MUL, DIV, MAC, MAX
};

class ndp_unit {
  public:
    ndp_unit(ndp_config* config);
    void cycle();
    void print_all();
  private:
    ndp_config* config;
    ndp_controller controller;
    simd_unit* simd_units;
};

struct ndp_op_table_element
{
  int id;
  ndp_op_type op_type;
  new_addr_type src1_addr;
  new_addr_type src2_addr;
  new_addr_type dest_addr;
  unsigned size;
  unsigned op_size;
  unsigned remaining_subtensor_op;
};
#endif