#ifndef CXL_MEMORY_BUFFER
#define CXL_MEMORY_BUFFER

class cxl_memory_buffer {
  public:
    cxl_memory_buffer();
    ~cxl_memory_buffer();
    cycle();

  private:
    cxl_memory_buffer_config config;


};

class cxl_memory_buffer_config {
  public:
    cxl_memory_buffer_config(/* args */);
    ~cxl_memory_buffer_config();
  private:
    /* data */
  friend cxl_memory_buffer;
};



#endif