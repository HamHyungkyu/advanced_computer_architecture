#include "ndp_packet_operation.h"

ndp_packet_operation::ndp_packet_operation( sub_tensor_op_type type, 
  new_addr_type src1_base, new_addr_type src2_base, new_addr_type dest_base,
  unsigned count, unsigned size, unsigned stride)
  : op_type(type), src1_base(src1_base), src2_base(src2_base), dest_base(dest_base),
  count(count), size(size), stride(stride) 
  {

  }

void unary_packet_operation::issue(const std::deque<Request> *read_requests){
  
}