#ifndef NDP_PACKET_OPERATION
#define NDP_PACKET_OPERATION
#include "simd_controller.h"
#include "../abstract_hardware_model.h"

//abstract packet operation class
class ndp_packet_operation {
  public:
    ndp_packet_operation();
    virtual void issue()
  protected:
    bool valid;
    int index;
    bool ready1;
    bool ready2;
    new_addr_type dest_addr;    
  friend simd_controller;
};

class unary_packet_operation : public ndp_packet_operation {
  public:
    void issue();
}

class unary_packet_operation : public ndp_packet_operation {
  public:
    void issue();
}



#endif