// developed by Mahmoud Khairy, Purdue Univ
// abdallm@purdue.edu

#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <vector>
#include <queue>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "gpgpu_context.h"
#include "abstract_hardware_model.h"
#include "cuda-sim/cuda-sim.h"
#include "gpgpu-sim/gpu-sim.h"
#include "gpgpu-sim/icnt_wrapper.h"
#include "gpgpusim_entrypoint.h"
#include "option_parser.h"
#include "ISA_Def/trace_opcode.h"
#include "trace_driven.h"
#include "accelsim_version.h"

/* TO DO:
 * NOTE: the current version of trace-driven is functionally working fine,
 * but we still need to improve traces compression and simulation speed.
 * This includes:
 *
 * 1- Prefetch concurrent thread that prefetches traces from disk (to not be
 * limited by disk speed)
 *
 * 2- traces compression format a) cfg format and remove
 * thread/block Id from the head and b) using zlib library to save in binary format
 *
 * 3- Efficient memory improvement (save string not objects - parse only 10 in
 * the buffer)
 *
 * 4- Seeking capability - thread scheduler (save tb index and warp
 * index info in the traces header)
 *
 * 5- Get rid off traces intermediate files -
 * change the tracer
 */
#define MAX_GPUS 16
bool finished_gpu_jobs[MAX_GPUS];
bool wating_gpus[MAX_GPUS];
bool exit_gpus[MAX_GPUS];

std::queue<schedule_command> schedule_commands;
std::mutex cycle_mtx; // mutex for cycle synchronize
std::mutex kernel_mtx; // mutex for kernel scheduling
std::mutex exit_mtx; // mutex for exit checking
std::condition_variable cycle_sync; // conditoin variable for cycle synchronize
static int ready_counter = 0;
static int cycle_counter = 0;

gpgpu_sim *gpgpu_trace_sim_init_perf_model(int argc, const char *argv[],
                                           gpgpu_context *m_gpgpu_context,
                                           class trace_config *m_config, int num_gpu);
int parse_num_gpus(int argc, const char **argv);
bool check_scheduling(std::string kernel_name);
bool check_sync_device_finished(int num_gpus);
bool check_all_gpu_sim_active(int num_gpus);
void cycle_synchronizer(int num_gpus, int *local_cycle);
void do_gpu_perf(int num_gpus, trace_config tconfig, gpgpu_context *m_gpgpu_context, gpgpu_sim *m_gpgpu_sim);

int main(int argc, const char **argv)
{
  int num_gpus = parse_num_gpus(argc, argv);
  FILE *output_files[num_gpus];
  gpgpu_context *m_gpgpu_contexts[num_gpus];
  trace_config *m_trace_configs[num_gpus];
  gpgpu_sim *m_gpgpu_sims[num_gpus];
  std::vector<std::thread> threads;
  for (int i = 0; i < num_gpus; i++)
  {
    std::string name = "GPU_" + std::to_string(i) + ".out";
    output_files[i] = fopen(name.c_str(), "w");
    m_gpgpu_contexts[i] = new gpgpu_context();
    m_trace_configs[i] = new trace_config(i, output_files[i]);
    m_gpgpu_sims[i] = gpgpu_trace_sim_init_perf_model(argc, argv, m_gpgpu_contexts[i], m_trace_configs[i], i);
    m_gpgpu_sims[i]->init();
  }
  schedule_commands = trace_parser::parse_schedule_file(m_trace_configs[0]->get_traces_filename());
  for (int i = 0; i < num_gpus; i++)
  {
    exit_gpus[false];
    threads.push_back(std::thread(do_gpu_perf, num_gpus, *m_trace_configs[i], m_gpgpu_contexts[i], m_gpgpu_sims[i]));
  }
  for (auto &th : threads)
    th.join();
  // we print this message to inform the gpgpu-simulation stats_collect script
  // that we are done
  printf("GPGPU-Sim:  *** simulation thread exiting ***\n");
  printf("GPGPU-Sim: *** exit detected ***\n");

  return 1;
}

void do_gpu_perf(int num_gpus, trace_config tconfig, gpgpu_context *m_gpgpu_context, gpgpu_sim *m_gpgpu_sim)
{
  // for each kernel
  // load file
  // parse and create kernel info
  // launch
  // while loop till the end of the end kernel execution
  // prints stats
  std::stringstream stream;
  int gpu_num = m_gpgpu_sim->get_gpu_num();
  std::string file_name = tconfig.get_traces_filename();
  trace_parser tracer(file_name.c_str(), m_gpgpu_sim,
                      m_gpgpu_context);
  tconfig.parse_config();
  std::vector<trace_command> commandlist = tracer.parse_commandlist_file();
  int local_cycle = 0;
  bool next_launch = true;
  bool sim_active = true;
  bool all_gpu_sim_active = true;
  trace_kernel_info_t *kernel_info;
  unsigned int i = 0;
  do // do cycle 
  {
    cycle_synchronizer(num_gpus, &local_cycle);

    //if current gpu is finished
    if (!sim_active)
      continue;

    //show next command list
    if (next_launch)
    {
      kernel_info = NULL;

      if (commandlist[i].m_type == command_type::cpu_gpu_mem_copy)
      {
        if (check_scheduling(commandlist[i].command_string))
        {
          size_t addre, Bcount;
          tracer.parse_memcpy_info(commandlist[i].command_string, addre, Bcount);
          stream << "launching memcpy command : " << commandlist[i].command_string << std::endl;
          fprintf(m_gpgpu_sim->get_output_file(), stream.str().c_str());
          stream.clear();
          m_gpgpu_sim->perf_memcpy_to_gpu(addre, Bcount);
          i++;
        }
        else
        {
          printf("scheduler not match\n");
          m_gpgpu_sim->cycle();
        }
        continue;
      }
      else if (commandlist[i].m_type == command_type::kernel_launch)
      {
        if (check_scheduling(commandlist[i].command_string))
        {
          std::cout << "launching kernel command : " << commandlist[i].command_string << std::endl;
          kernel_info = tracer.parse_kernel_info(commandlist[i].command_string, &tconfig);
          stream << "launching kernel command : " << commandlist[i].command_string << std::endl;
          fprintf(m_gpgpu_sim->get_output_file(), stream.str().c_str());
          stream.clear();
          m_gpgpu_sim->launch(kernel_info);
          finished_gpu_jobs[gpu_num] = false;
        }
        else
        {
          printf("scheduler not match\n");
          m_gpgpu_sim->cycle();
          continue;
        }
      }
      else
        assert(0);
    }
    
    bool active = false;
    bool sim_cycles = false;
    
    // performance simulation
    if (m_gpgpu_sim->active())
    {
      m_gpgpu_sim->cycle();
      sim_cycles = true;
      m_gpgpu_sim->deadlock_check();
    }
    //finish kernel
    if (!m_gpgpu_sim->active() && !finished_gpu_jobs[gpu_num])
    {
      std::unique_lock<std::mutex> lck(kernel_mtx);
      finished_gpu_jobs[gpu_num] = true;
      if (wating_gpus[gpu_num])
        wating_gpus[gpu_num] = false;
      lck.unlock();
      if (kernel_info)
      {
        tracer.kernel_finalizer(kernel_info);
        m_gpgpu_sim->print_stats();
      }
    }

    active = m_gpgpu_sim->active();

    next_launch = !active && check_sync_device_finished(num_gpus);
    if (next_launch)
      i++;
  
    sim_active = !(i == commandlist.size() && next_launch);
    if (!sim_active)
    {
      std::unique_lock<std::mutex> lck(exit_mtx);
      exit_gpus[m_gpgpu_sim->get_gpu_num()] = true;
      lck.unlock();
    }

  } while (check_all_gpu_sim_active(num_gpus));
  m_gpgpu_sim->update_stats();
  m_gpgpu_context->print_simulation_time();
  printf("FINISHED - %d\n", m_gpgpu_sim->get_gpu_num());
}

gpgpu_sim *gpgpu_trace_sim_init_perf_model(int argc, const char *argv[],
                                           gpgpu_context *m_gpgpu_context,
                                           trace_config *m_config, int gpu_num)
{
  srand(1);
  print_splash();
  option_parser_t opp = option_parser_create();
  m_gpgpu_context->ptx_reg_options(opp);
  m_gpgpu_context->func_sim->ptx_opcocde_latency_options(opp);

  icnt_reg_options(opp);

  m_gpgpu_context->the_gpgpusim->g_the_gpu_config =
      new gpgpu_sim_config(m_gpgpu_context, gpu_num, m_config->get_output_file());
  m_gpgpu_context->the_gpgpusim->g_the_gpu_config->reg_options(
      opp); // register GPU microrachitecture options
  m_config->reg_options(opp);
  option_parser_cmdline(opp, argc, argv); // parse configuration options
  fprintf(stdout, "GPGPU-Sim: Configuration options:\n\n");
  option_parser_print(opp, stdout);
  // Set the Numeric locale to a standard locale where a decimal point is a
  // "dot" not a "comma" so it does the parsing correctly independent of the
  // system environment variables
  assert(setlocale(LC_NUMERIC, "C"));
  m_gpgpu_context->the_gpgpusim->g_the_gpu_config->init();

  m_gpgpu_context->the_gpgpusim->g_the_gpu = new trace_gpgpu_sim(
      *(m_gpgpu_context->the_gpgpusim->g_the_gpu_config), m_gpgpu_context);

  m_gpgpu_context->the_gpgpusim->g_stream_manager =
      new stream_manager((m_gpgpu_context->the_gpgpusim->g_the_gpu),
                         m_gpgpu_context->func_sim->g_cuda_launch_blocking);

  m_gpgpu_context->the_gpgpusim->g_simulation_starttime = time((time_t *)NULL);

  return m_gpgpu_context->the_gpgpusim->g_the_gpu;
}

void cycle_synchronizer(int num_gpus, int *local_cycle){
    //loop synchronizer
    std::unique_lock<std::mutex> lck(cycle_mtx);
    ready_counter++;
    while (ready_counter == num_gpus && *local_cycle == cycle_counter)
    {
      cycle_sync.notify_all();
      cycle_counter++;
      ready_counter = 0;
    }
    while (ready_counter < num_gpus && *local_cycle == cycle_counter)
    {
      cycle_sync.wait(lck);
    }
    lck.unlock();
    (*local_cycle)++;
}

int parse_num_gpus(int argc, const char **argv)
{
  int num_gpus = 1;
  for (int i = 1; i < argc - 1; i++)
  {
    if (std::string(argv[i]) == "-num_gpus")
    {
      num_gpus = atoi(argv[i + 1]);
      return num_gpus;
    }
  }
  return num_gpus;
}

//Checking kernel scheduling
//If schedule commands have decvice_sync, set Wating_gpus 
bool check_scheduling(std::string kernel_name)
{
  kernel_name = kernel_name.erase(0, kernel_name.find_last_of('/') + 1);
  bool kenel_can_run = kernel_name == schedule_commands.front().command_string;

  if (kenel_can_run)
  {
    std::unique_lock<std::mutex> lck(kernel_mtx);
    schedule_commands.pop();
    while (schedule_commands.front().m_type == command_type::device_sync)
    {
      wating_gpus[schedule_commands.front().gpu_num] = true;
      schedule_commands.pop();
    }
    lck.unlock();
  }
  return kenel_can_run;
}

//Return true when all device_sync gpu finished
bool check_sync_device_finished(int num_gpus)
{
  bool finished = false;
  for (int i = 0; i < num_gpus; i++)
  {
    finished = finished || wating_gpus[i];
  }
  return !finished;
}

//return false when all gpu exited 
bool check_all_gpu_sim_active(int num_gpus)
{
  bool finished = true;
  for (int i = 0; i < num_gpus; i++)
  {
    finished = finished && exit_gpus[i];
  }
  return !finished;
}
