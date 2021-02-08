#ifndef NDP_UNIT
#define NDP_UNIT
#include "ndp_config.h"
#include "ndp_controller.h"
#include "simd_unit.h"

enum ndp_op_type {
  ADD, SUB, MUL, DIV, MAC, MAX
};

enum sub_tensor_op_type {
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

#endif