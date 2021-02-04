#include "ndp_controller.h"
#include "simd_unit.h"

enum ndp_op_type {
  ADD, SUB, MUL, DIV, MAC, MAX
};

enum sut_tensor_op_type {
  ADD, SUB, MUL, DIV, MAC, MAX
};

class ndp_unit {
  public:
    ndp_unit();
    void cycle();
  private:
    ndp_controller controller;

};