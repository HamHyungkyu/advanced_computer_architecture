#ifndef CXL_NDP_FETCH_H
#define CXL_NDP_FETCH_H
#include "mem_fetch.h"

enum ndp_type_t {
  ADD_REQ,
  ADD_ACK,
  MUL_REQ,
  MUL_ACK,
  MAC_REQ,
  MAC_ACK
};

class cxl_ndp_fetch : public mem_fetch {
  public: 
    cxl_ndp_fetch(ndp_type_t ndp_type);
    
  private:
    ndp_type_t m_ndp_type;
}

#endif