#ifndef __REQUEST_H
#define __REQUEST_H

#include <vector>
#include <functional>
#include "../gpgpu-sim/mem_fetch.h"

using namespace std;

namespace ramulator
{

class Request
{
public:
    typedef enum {NORMAL_READ, NORMAL_WRITE, MISS_READ, DIRTY_READ, COPY_WRITE, EVICT_WRITE, MAX} Status;

    Status stat = Status::NORMAL_READ;

    mem_fetch* mf = nullptr;

    // used to calculate stat
    int counter = 0;
    bool is_miss_req = false;
    bool is_first_command;
    bool is_sector_miss = false;
    bool is_line_miss = false;
    bool wasEvict = false;
    bool is_miss_read_req = false;
    bool is_dirty_read_req = false;


    unsigned long targetSCMPageAddr = -1;
    unsigned long targetDRAMPageAddr = -1;
    unsigned long tagAddr = -1;
    unsigned long setAddr = -1;
    unsigned long way = -1;
    unsigned long dirtyTag = -1;

    vector<unsigned long> next_addr;
    unsigned long addr;
    // long addr_row;
    vector<int> addr_vec;


    long reqid = -1;
    // specify which core this request sent from, for virtual address translation
    int coreid = -1;

    bool fromMSHR = false;
    bool toDRAM = false;
    bool toSCM = false;

    bool isFirstCopy = true;
    bool hasDependency = false;

    int dram_row_hit = 0;
    int scm_row_hit = 0;

    enum class Type
    {
        R_READ,     // dram read
        R_WRITE,    // dram write
        REFRESH,
        POWERDOWN,
        SELFREFRESH,
        EXTENSION,
        MAX
    } type;

    long arrive = 0;
    long depart;
    long arrive_hmc;
    long depart_hmc;

    int burst_count = 0;
    int transaction_bytes = 0;
    function<void(Request&)> callback; // call back with more info

    Request(unsigned long addr, Type type, int coreid)
        : is_first_command(true), addr(addr), coreid(coreid), type(type),
      callback([](Request& req){}) {
        if (type == Type::R_READ)
          stat = Status::NORMAL_READ;
        else if (type == Type::R_WRITE)
          stat = Status::NORMAL_WRITE;
      }

    Request(unsigned long addr, Type type, function<void(Request&)> callback, int coreid)
        : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback) {
        if (type == Type::R_READ)
          stat = Status::NORMAL_READ;
        else if (type == Type::R_WRITE)
          stat = Status::NORMAL_WRITE;
      }
    Request(unsigned long addr, Type type, function<void(Request&)> callback, int coreid, mem_fetch *mf)
        : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback), mf(mf) {
        if (type == Type::R_READ) {
          assert(mf->get_dirty_mask().none());
          assert(mf->get_data_size() == SECTOR_SIZE);
          stat = Status::NORMAL_READ;
        } else if (type == Type::R_WRITE) {
          stat = Status::NORMAL_WRITE;
          auto dirty_mask = mf->get_dirty_mask();
          assert(dirty_mask.any());
          for (unsigned long i = 0; i < dirty_mask.size(); ++i) {
            if (dirty_mask.test(i)) {
              unsigned long blockAddr = addr & (~(unsigned long)((SECTOR_CHUNCK_SIZE * SECTOR_SIZE) - 1));
              unsigned long nextAddr = blockAddr | (i * SECTOR_SIZE);
              next_addr.push_back(nextAddr);
            }
          }
          assert(dirty_mask.count() * SECTOR_SIZE == mf->get_data_size());
        }
      }

    Request(vector<int>& addr_vec, Type type, function<void(Request&)> callback, int coreid)
        : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), callback(callback) {
        if (type == Type::R_READ)
          stat = Status::NORMAL_READ;
        else if (type == Type::R_WRITE)
          stat = Status::NORMAL_WRITE;
        }

    Request() {}

    bool hasNextAddr() {
      return next_addr.size() != 0;
    }
    unsigned long getNextAddr() {
      unsigned long nextAddr = next_addr.back();
      next_addr.pop_back();
      return nextAddr;
    }
};

} /*namespace ramulator*/

#endif /*__REQUEST_H*/

