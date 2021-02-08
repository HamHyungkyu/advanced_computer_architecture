#ifndef NDP_CONTROLLER
#define NDP_CONTROLLER
#include "../abstract_hardware_model.h"

class ndp_controller {
  public:
    void init();
    void cycle();
    bool can_issue_ndp_op();
    void issue_ndp_op();
    void commit_ndp_op();
    void issue_subtensor_op();
  private:
    ndp_op_table_element* ndp_op_table;
};

struct ndp_op_table_element
{
  int id;
  ndp_op_type op_type;
  new_addr_type src1_addr;
  new_addr_type src2_addr;
  new_addr_type dest_addr;
  unsigned long long size;
  unsigned op_size;
  unsigned long long remaining_subtensor_op;
};

#endif