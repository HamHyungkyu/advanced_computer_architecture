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
#include <cstdlib>
#include <thread>
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
int cycle_sync = 0;
gpgpu_sim *gpgpu_trace_sim_init_perf_model(int argc, const char *argv[],
                                           gpgpu_context *m_gpgpu_context,
                                           class trace_config *m_config, int num_gpu);
int parse_num_gpus(int argc, const char**argv);
void do_gpu_perf(int gpu_num, trace_config tconfig, gpgpu_context *m_gpgpu_context, gpgpu_sim *m_gpgpu_sim);
int main(int argc, const char **argv)
{
  int num_gpus = parse_num_gpus(argc, argv);
  //int num_gpus = 1;
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
    m_trace_configs[i] =  new trace_config(i, output_files[i]);
    m_gpgpu_sims[i] = gpgpu_trace_sim_init_perf_model(argc, argv, m_gpgpu_contexts[i], m_trace_configs[i], i);
    m_gpgpu_sims[i]->init();
  }

  for(int i = 0; i<num_gpus; i++){
      threads.push_back(std::thread(do_gpu_perf, i, *m_trace_configs[i], m_gpgpu_contexts[i], m_gpgpu_sims[i]));
  }
  for (auto& th : threads) th.join();
  // we print this message to inform the gpgpu-simulation stats_collect script
  // that we are done
  printf("GPGPU-Sim:  *** simulation thread exiting ***\n");
  printf("GPGPU-Sim: *** exit detected ***\n");

  return 1;
}

int parse_num_gpus(int argc, const char** argv)
{
  int num_gpus = 1;
  for (int i = 1; i < argc - 1; i++){
    if(std::string(argv[i]) == "-num_gpus"){
      num_gpus = atoi(argv[i+1]);
      return num_gpus;
    }
  }
  return num_gpus;
}

void do_gpu_perf(int gpu_num, trace_config tconfig, gpgpu_context *m_gpgpu_context, gpgpu_sim *m_gpgpu_sim)
{
  // for each kernel
  // load file
  // parse and create kernel info
  // launch
  // while loop till the end of the end kernel execution
  // prints stats
  std::stringstream stream;

  std::string file_name = tconfig.get_traces_filename();
  trace_parser tracer(file_name.c_str(), m_gpgpu_sim,
                      m_gpgpu_context);
  tconfig.parse_config();

  std::vector<trace_command> commandlist = tracer.parse_commandlist_file();

  for (unsigned i = 0; i < commandlist.size(); ++i)
  {
    trace_kernel_info_t *kernel_info = NULL;
    if (commandlist[i].m_type == command_type::cpu_gpu_mem_copy)
    {
      size_t addre, Bcount;
      tracer.parse_memcpy_info(commandlist[i].command_string, addre, Bcount);
      stream << "launching memcpy command : " << commandlist[i].command_string << std::endl;
      fprintf(m_gpgpu_sim->get_output_file(), stream.str().c_str());
      stream.clear();
      m_gpgpu_sim->perf_memcpy_to_gpu(addre, Bcount);
      continue;
    }
    else if (commandlist[i].m_type == command_type::kernel_launch)
    {
      kernel_info = tracer.parse_kernel_info(commandlist[i].command_string, &tconfig);
      stream << "launching kernel command : " << commandlist[i].command_string << std::endl;
      fprintf(m_gpgpu_sim->get_output_file(), stream.str().c_str());
      stream.clear();
      m_gpgpu_sim->launch(kernel_info);
    }
    else
      assert(0);

    bool active = false;
    bool sim_cycles = false;
    bool break_limit = false;

    do
    {
      if (!m_gpgpu_sim->active())
        break;

      // performance simulation
      if (m_gpgpu_sim->active())
      {
        m_gpgpu_sim->cycle();
        sim_cycles = true;
        m_gpgpu_sim->deadlock_check();
      }
      else
      {
        if (m_gpgpu_sim->cycle_insn_cta_max_hit())
        {
          m_gpgpu_context->the_gpgpusim->g_stream_manager
              ->stop_all_running_kernels();
          break_limit = true;
        }
      }

      active = m_gpgpu_sim->active();

    } while (active);

    if (kernel_info)
    {
      tracer.kernel_finalizer(kernel_info);
      m_gpgpu_sim->print_stats();
    }

    if (sim_cycles)
    {
      m_gpgpu_sim->update_stats();
      m_gpgpu_context->print_simulation_time();
    }

    if (break_limit)
    {
      printf("GPGPU-Sim: ** break due to reaching the maximum cycles (or "
             "instructions) **\n");
      fflush(stdout);
      exit(1);
    }
  }
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
