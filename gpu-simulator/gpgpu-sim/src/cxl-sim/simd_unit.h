#ifndef SIMD_UNIT
#define SIMD_UNIT
#include<deque>
#include<functional>
#include "ndp_config.h"
#include "simd_controller.h"
#include "ndp_packet_operation.h"
#include "../ramulator/Request.h"

#define NO_VALID_SLOT -1

enum sub_tensor_op_type {
  NO_LOAD_MAX,
  UNARY_ADD, UNARY_SUB, UNARY_MUL, UNARY_DIV, UNARY_MAX,
  BINARY_ADD, BINARY_SUB, BINARY_MUL, BINARY_DIV, BINARY_MAX,
  MAC, SPU, THIRNARY_MAX,
  MAX
};

enum simd_op_type {
  ADD, SUB, MUL, DIV, SPU, MAC, MAX
};

typedef struct {
  bool valid;
  simd_op_type op_type;
  int ready_count;
  int required_read;
  new_addr_type dest_addr;
  unsigned long long arrive;
  unsigned long long depart;    
} packet_slot_elemnt;

class simd_unit {
  public:
    simd_unit(ndp_config config);
    ~simd_unit();
    void cycle();
    void issue_subtensor_operation(
      sub_tensor_op_type type,
      new_addr_type src1_base, new_addr_type src2_base, new_addr_type dest_base,
      unsigned size, unsigned stride );
    bool can_issue_subtensor_operation();
  private:
    const ndp_config config;
    int clock;
    
    //Stall types
    int packet_op_stall;
    int alu_busy_stall;
    int no_rd_op_stall;
    //Requests
    std::deque<ramulator::Request> read_requests;
    std::deque<ramulator::Request> write_requests;
    
    int current_vector_op_stat_reg_id;
    int current_slot_id;
    float write_high_watermark;
    float write_low_watermark;
    
    simd_alu alu;
    ndp_packet** vector_op_status_regs;
    packet_slot_elemnt* packet_operation_slots;
    int vector_op_status_reg_max_id;

    int find_free_slot_id();
    int get_executable_slot_id();
};

class simd_alu {
  public:
    void init(ndp_config config, std::deque<ramulator::Request> *write_requests);
    void cycle();
    void issue(packet_slot_element slot);
    bool can_issue(packet_slot_elemnt slot);
  
  private:
    unsigned long long clock;
    unsigned long long active_cycles;
    unsigned long long idle_cycles;

    ndp_config config;
    int op_latencies[simd_op_type::MAX];
    int remaining_section;
    simd_op_type current_op;
    std::deque<packet_slot_elemnt> pending_queue;
    std::deque<ramulator::Request> *write_requests;
    void write_result();
};

class vector_op_status_reg {
  public:
    vector_op_status_reg( 
      int packet_size,
      std::deque<ramulator::Request> *read_requests,
      sub_tensor_op_type type,
      new_addr_type src1_base, new_addr_type src2_base, new_addr_type dest_base,
      unsigned size, unsigned stride);
    packet_slot_elemnt generate_packet();
    void issue_packet_operation(packet_slot_elemnt* slot);
  private:
    bool valid;
    sub_tensor_op_type type;
    new_addr_type src_base[3]; 
    new_addr_type dest_base;
    unsigned count;
    unsigned size;
    unsigned stride;

    int packet_size;
    std::deque<ramulator::Request> *read_requests,

    simd_op_type translate_operation();
    int count_operands();
};
#endif