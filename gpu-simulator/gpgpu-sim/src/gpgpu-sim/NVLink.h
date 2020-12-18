//#include "Request.h"
#include "mem_fetch.h"

#include "delayqueue.h"

#include "icnt_wrapper.h"


//50 GB/s throughput, 25GB/s per direction.
//Packet size: 32B -> 800M Packet per second per direction, 1600M packet per second
//Assume unidirection with 1 packet per cycle, 800 MHZ
//Assume bidirection with 1 packet in and one packet out, 800 MHZ
class NVLink {
public:
   NVLink(int gpu_id, int latency=2); //NVLink flit size 16byte, packet 32 byte
   ~NVLink();

   bool from_GPU_full(); 
   bool from_ramulator_full();
   
   bool from_GPU_empty(); 
   bool from_ramulator_empty();

   mem_fetch* from_GPU_top(); 
   mem_fetch* from_ramulator_top();

   mem_fetch* from_GPU_pop(); 
   mem_fetch* from_ramulator_pop();

   void push_from_GPU(mem_fetch* mf);
   void push_from_ramulator(mem_fetch* mf);

   void cycle();

private:
   fifo_pipeline<mem_fetch> *from_GPU;
   fifo_pipeline<mem_fetch> *from_ramulator;

   fifo_pipeline<mem_fetch> *to_GPU_waiting;
   fifo_pipeline<mem_fetch> *to_ramulator_waiting;

   fifo_pipeline<mem_fetch> *to_GPU;
   fifo_pipeline<mem_fetch> *to_ramulator;

   int gpu_id; //GPU id
   unsigned long long clk = 0;
   unsigned long long latency = 2;
   //int clk = 0;
};
