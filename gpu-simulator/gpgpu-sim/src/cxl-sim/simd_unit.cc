#include "simd_unit.h"

simd_unit::simd_unit(){
  controller.init();
  alu.init();
}

void simd_unit::cycle(){
  controller.cycle();
  alu.cycle();
}