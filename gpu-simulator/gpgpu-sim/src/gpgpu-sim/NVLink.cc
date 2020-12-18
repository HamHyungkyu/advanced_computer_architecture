#include "NVLink.h"

NVLink::NVLink(int gpu_id, int latency) {
   from_ramulator = new fifo_pipeline<mem_fetch>("request queue from ramulator", 0, 1024);
   from_GPU = new fifo_pipeline<mem_fetch>("request queue from GPU", 0, 1024);
   to_GPU_waiting = new fifo_pipeline<mem_fetch>("request waiting queue to GPU", 0, 1024);
   to_ramulator_waiting = new fifo_pipeline<mem_fetch>("request waiting queue to GPU", 0, 1024);
   to_GPU = new fifo_pipeline<mem_fetch>("request queue to GPU", 0, 1024);
   to_ramulator = new fifo_pipeline<mem_fetch>("request queue to GPU", 0, 1024);

   this->latency = latency;
   this->gpu_id = gpu_id;
}

NVLink::~NVLink() {

}

bool NVLink::from_GPU_full() {
   return from_GPU->full();
}

bool NVLink::from_ramulator_full() {
   return from_ramulator->full();
}


bool NVLink::from_GPU_empty() {
   return from_GPU->empty();
}

bool NVLink::from_ramulator_empty() {
   return from_ramulator->empty();
}

mem_fetch* NVLink::from_GPU_top() {
   return from_GPU->top();
}

mem_fetch* NVLink::from_ramulator_top() {
   return from_ramulator->top();
}

mem_fetch* NVLink::from_GPU_pop() {
   return from_GPU->pop();
}

mem_fetch* NVLink::from_ramulator_pop() {
   return from_ramulator->pop();
}

void NVLink::push_from_GPU(mem_fetch* mf) {
   from_GPU->push(mf);
}

void NVLink::push_from_ramulator(mem_fetch* mf) {
   from_ramulator->push(mf);
}

void NVLink::cycle() {
   clk++;
   if (!from_GPU->empty()) {
      if (!to_ramulator_waiting->full()) {
         mem_fetch* mf = from_GPU->pop();
         
         mf->set_link_depart(clk + this->latency);
         to_ramulator_waiting->push(mf);
      }
   }

   if (!from_ramulator->empty()) {
      if (!to_GPU_waiting->full()) {
         mem_fetch* mf = from_ramulator->pop();
         
         mf->set_link_depart(clk + this->latency);
         to_GPU_waiting->push(mf);
      }
   }

   if (!to_ramulator_waiting->empty()) {
      if (!to_ramulator->full()) {
         mem_fetch* mf = to_ramulator_waiting->top();
         if (mf->get_link_depart() <= this->clk) {
            mf->set_gpu_id(this->gpu_id);
            to_ramulator->push(mf);
            to_ramulator_waiting->pop();
         }
      }
   }

   if (!to_GPU_waiting->empty()) {
      if (!to_GPU->full()) {
         mem_fetch* mf = to_GPU_waiting->top();
         if (mf->get_link_depart() <= this->clk) {
            to_GPU->push(mf);
            to_GPU_waiting->pop();
         }   
      }
   }
}