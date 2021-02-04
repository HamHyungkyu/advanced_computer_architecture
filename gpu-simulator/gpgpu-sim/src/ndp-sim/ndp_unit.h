#include "ndp_controller.h"
#include "simd_unit.h"

class ndp_unit {
  public:
    ndp_unit();
    void cycle();
  private:
    ndp_controller controller;

};