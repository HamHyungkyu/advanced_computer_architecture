#include "NVLink.h"

NVLink::NVLink(int gpu_id, int latency) {
   from_cxl_buffer = new fifo_pipeline<mem_fetch>("request queue from cxl_buffer", 0, 1024);
   from_GPU = new fifo_pipeline<mem_fetch>("request queue from GPU", 0, 1024);
   to_GPU_waiting = new fifo_pipeline<mem_fetch>("request waiting queue to GPU", 0, 1024);
   to_cxl_buffer_waiting = new fifo_pipeline<mem_fetch>("request waiting queue to GPU", 0, 1024);
   to_GPU = new fifo_pipeline<mem_fetch>("request queue to GPU", 0, 1024);
   to_cxl_buffer = new fifo_pipeline<mem_fetch>("request queue to GPU", 0, 1024);

   this->latency = latency;
   this->gpu_id = gpu_id;
}

NVLink::~NVLink() {

}

bool NVLink::from_GPU_full() {
   return from_GPU->full();
}

bool NVLink::from_cxl_buffer_full() {
   return from_cxl_buffer->full();
}


bool NVLink::from_GPU_empty() {
   return from_GPU->empty();
}

bool NVLink::from_cxl_buffer_empty() {
   return from_cxl_buffer->empty();
}

mem_fetch* NVLink::from_GPU_top() {
   return from_GPU->top();
}

mem_fetch* NVLink::from_cxl_buffer_top() {
   return from_cxl_buffer->top();
}

mem_fetch* NVLink::from_GPU_pop() {
   return from_GPU->pop();
}

mem_fetch* NVLink::from_cxl_buffer_pop() {
   return from_cxl_buffer->pop();
}

mem_fetch* NVLink::to_GPU_pop() {
   return to_GPU->pop();
}

mem_fetch* NVLink::to_cxl_buffer_pop() {
   return to_cxl_buffer->pop();
}

void NVLink::push_from_GPU(mem_fetch* mf) {
   from_GPU->push(mf);
}


bool NVLink::to_GPU_empty() {
   return to_GPU->empty();
}
bool NVLink::to_cxl_buffer_empty() {
   return to_cxl_buffer->empty();
}

void NVLink::push_from_cxl_buffer(mem_fetch* mf) {
   from_cxl_buffer->push(mf);
}

mem_fetch* NVLink::to_GPU_top() {
   return to_GPU->top();
}
mem_fetch* NVLink::to_cxl_buffer_top() {
   return to_cxl_buffer->top();
}

void NVLink::cycle() {
   clk++;
   if (!from_GPU->empty()) {
      if (!to_cxl_buffer_waiting->full()) {
         mem_fetch* mf = from_GPU->pop();
         
         mf->set_link_depart(clk + this->latency);
         to_cxl_buffer_waiting->push(mf);
      }
   }

   if (!from_cxl_buffer->empty()) {
      if (!to_GPU_waiting->full()) {
         mem_fetch* mf = from_cxl_buffer->pop();
         
         mf->set_link_depart(clk + this->latency);
         to_GPU_waiting->push(mf);
      }
   }

   if (!to_cxl_buffer_waiting->empty()) {
      if (!to_cxl_buffer->full()) {
         mem_fetch* mf = to_cxl_buffer_waiting->top();
         if (mf->get_link_depart() <= this->clk) {
            mf->set_gpu_id(this->gpu_id);
            to_cxl_buffer->push(mf);
            to_cxl_buffer_waiting->pop();
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