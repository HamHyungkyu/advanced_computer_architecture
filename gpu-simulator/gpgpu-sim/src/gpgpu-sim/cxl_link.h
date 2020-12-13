#ifndef CXL_LINK_H
#define CXL_LINK_H
#include "mem_fetch.h"
#include "icnt_wrapper.h"

class cxl_link {
  public:
    cxl_link();
    ~cxl_link();

    bool push(mem_fetch mf);
    mem_fetch pop();
    bool is_busy();

  private:
};

#endif