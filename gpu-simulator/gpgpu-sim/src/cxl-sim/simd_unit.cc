#include "simd_unit.h"

/*
* SIMD unit
*/
simd_unit::simd_unit(ndp_config config): config(config) {
  alu.init(config, write_requests);
  vector_op_status_regs = malloc(sizeof(vector_op_status_reg *) * config.num_vector_op_status_regs);
  vector_op_status_reg_max_id = 0;
  current_vector_op_stat_reg_id = 0;
  packet_operation_slots = malloc(sizeof(packet_slot_elemnt) * config.num_packet_slots);
}

simd_unit::~simd_unit(){
  delete vector_op_status_regs;
  delete packet_operation_slots;
}

void simd_unit::cycle(){
  clock++;
  alu.cycle();

  if(vector_op_status_reg_max_id > 0){
    int slot_id = find_free_slot_id();
    if(slot_id != NO_VALID_SLOT){
      vector_op_status_regs[current_vector_op_stat_reg_id]
        .issue_packet_operation(&packet_operation_slots[slot_id]);
    }
  }

  int executable_slot_id = get_executable_slot_id();
  if(executable_slot_id != NO_VALID_SLOT && alu.can_issue(packet_operation_slots[executable_slot_id])){
    packet_operation_slots[executable_slot_id].valid = false;
    alu.issue(packet_operation_slots[executable_slot_id]);
  }
}

void simd_unit::issue_subtensor_operation(
  sub_tensor_op_type type,
  new_addr_type src1_base, new_addr_type src2_base, new_addr_type dest_base,
  unsigned size, unsigned stride ) {
  vector_op_status_regs[vector_op_status_reg_max_id] = new vector_op_status_reg(
    config.packet_size, &read_requests, 
    type, src1_base, src2_base, dest_base, size, stride);
  vector_op_status_reg_max_id ++;
}

bool simd_unit::can_issue_subtensor_operation(){
  if(vector_op_status_reg_max_id < config.num_vector_op_status_regs)
    return true;
  else
    return false;
}

//Private helper functions
int simd_unit::find_free_slot_id(){
  int result = NO_VALID_SLOT;
  for(int i = 0; i < config.num_packet_slots; i++){
    int id = (current_slot_id + i) % config.num_packet_slots;
    if(packet_operation_slots[id].valid == false){
      result = id;
      current_slot_id = id;
      break;
    }
  }
  return result;
}

int simd_unit::get_executable_slot_id(){
  int result = NO_VALID_SLOT;
  for(int i = 0; i < config.num_packet_slots; i++){
    int id = (current_slot_id + i) % config.num_packet_slots;
    if(packet_operation_slots[id].ready_count == packet_operation_slots[id].required_read){
      result = id;
      break;
    }
  }
  return result;
}

/*
* SIMD ALU 
*/
void simd_alu::init(const ndp_config config, std::deque<ramulator::Request> *write_requests){
  config = config;
  for(int i = 0; i < simd_op_type::MAX; i++){
    op_latencies[i] = config.simd_op_latencies[i];
  }
  remaining_section = 0;
  this->write_requests = write_requests;
}

void simd_alu::cycle(){
  //Increase cycles
  clock++;
  if(pending_queue.size() > 0){
    active_cycles++;
  }
  else {
    idle_cycles++;
  }
  
  if(remaining_section > 0)
    remaining_section--;

  write_result();
}

void simd_alu::issue(packet_slot_elemnt slot){
  slot.arrive = clock;
  slot.depart = clock + op_latencies[slot.op_type];
  pending_queue.push_back(packet_slot_elemnt);
  remaining_section = config.packet_size / (config.simd_width / 8);
  current_op = slot.op_type;
}

bool simd_alu::can_issue(packet_slot_elemnt slot){
  if(remaining_section == 0 && (current_op == slot.op_type || pending_queue.empty()) )
    return true;
  else
    return false;
}

void simd_alu::write_result(){
  packet_slot_elemnt front = pending_queue.front();
  if(front.depart >= clock){
    auto write_callback = [](ramulator::Request& r){};
    ramulator::Request write_request(front.dest_addr, ramulator::Request::Type::R_WRITE, write_callback);
    write_requests->push_back(write_request);
  }
}

/*
* Vector operaation status register
*/
vector_op_status_reg::vector_op_status_reg( 
  int packet_size, std::deque<ramulator::Request> *read_requests,
  sub_tensor_op_type type, 
  new_addr_type src1_base, new_addr_type src2_base, new_addr_type dest_base,
  unsigned count, unsigned size, unsigned stride)
) : packet_size(packet_size), read_requests(read_requests)
  op_type(type), src1_base(src1_base), src2_base(src2_base), dest_base(dest_base),
  size(size), stride(stride)
{
  count = 0;
}

void vector_op_status_reg::issue_packet_operation(packet_slot_elemnt* slot) {
  slot->valid = true;
  slot->op_type = translate_operation(op_type);
  slot->ready_count = 0;
  slot->required_read = count_operands();
  slot->dest_addr = dest_base + count * stride * size;

  auto callback = [slot](ramulator::Request& r){
    slot->ready_count++;
  };

  for(int i = 0; i < count_operands(); i++){
    new_addr_type src_addr = src_base[i] + count * stride * size;
    ramulator::Request request(src_addr, ramulator::Request::Type::R_READ, callback);
    read_requests->push_back(request);
  }
}

//Private helper function
simd_op_type vector_op_status_reg::translate_operation(){
  simd_op_type result;
  switch (op_type)
  {
    break; 
  default:
    assert(0 & "INVALID subtensor operation");
    break;
  }
  return result;
}

int vector_op_status_reg::count_operands(){
  int result;
  if(op_type < sub_tensor_op_type::NO_LOAD_MAX)
    result = 0;
  else if(op_type < sub_tensor_op_type::UNARY_MAX)
    result = 1;
  else if(op_type < sub_tensor_op_type::BINARY_MAX)
    result = 2;
  else if(op_type < sub_tensor_op_type::THIRNARY_MAX)
    return = 3
  else 
    assert(0 & "INVALID subtensor operation");
  return result;
}


