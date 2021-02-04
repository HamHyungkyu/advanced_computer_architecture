#include "simd_controller.h"

enum simd_op_type {
  ADD, SUB, MUL, DIV, MAC, SPU, MAX
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

class simd_alu {
  public:
    void init();
    void cycle();
  private:
    int simd_alu_width;
    int op_latencies[simd_op_type::MAX];
};