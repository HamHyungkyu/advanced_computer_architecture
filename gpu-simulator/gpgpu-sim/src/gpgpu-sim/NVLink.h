#ifndef NVLINK_H
#define NVLINK_H
//#include "Request.h"
#include <iostream>

#include "mem_fetch.h"

#include "delayqueue.h"

#include "icnt_wrapper.h"

// 50 GB/s throughput, 25GB/s per direction.
// Packet size: 32B -> 800M Packet per second per direction, 1600M packet per
// second Assume unidirection with 1 packet per cycle, 800 MHZ Assume bidirection
// with 1 packet in and one packet out, 800 MHZ
class NVLink {
 public:
  NVLink(int gpu_id,
         int latency = 2);  // NVLink flit size 16byte, packet 32 byte
  ~NVLink();

  bool from_GPU_full();
  bool from_cxl_buffer_full();

  bool from_GPU_empty();
  bool from_cxl_buffer_empty();

  mem_fetch* from_GPU_top();
  mem_fetch* from_cxl_buffer_top();

  mem_fetch* from_GPU_pop();
  mem_fetch* from_cxl_buffer_pop();

  mem_fetch* to_GPU_pop();
  mem_fetch* to_cxl_buffer_pop();

  void push_from_GPU(mem_fetch* mf);
  void push_from_cxl_buffer(mem_fetch* mf);

  bool to_GPU_empty();
  bool to_cxl_buffer_empty();

  mem_fetch* to_GPU_top();
  mem_fetch* to_cxl_buffer_top();

  void cycle();

 private:
  fifo_pipeline<mem_fetch>* from_GPU;
  fifo_pipeline<mem_fetch>* from_cxl_buffer;

  fifo_pipeline<mem_fetch>* to_GPU_waiting;
  fifo_pipeline<mem_fetch>* to_cxl_buffer_waiting;

  fifo_pipeline<mem_fetch>* to_GPU;
  fifo_pipeline<mem_fetch>* to_cxl_buffer;

  int gpu_id;  // GPU id
  unsigned long long clk = 0;
  unsigned long long latency = 2;
  // int clk = 0;
};
#endif