#ifndef NDP_CONTROLLER
#define NDP_CONTROLLER
#include "../abstract_hardware_model.h"
#include "ndp_unit.h"

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

#endif