#include "simd_controller.h"
#include "simd_alu.h"

enum simd_op_type {
  ADD, SUB, MUL, DIV, MAC, SPU
};

class simd_unit {
  public:
    simd_unit();
    void cycle();

  private:
    int clk;
    int active_cycles;
    int idle_cycles;

    simd_controller controller;
    simd_alu alu;
};

