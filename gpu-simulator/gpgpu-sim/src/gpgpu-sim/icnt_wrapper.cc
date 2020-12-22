// Copyright (c) 2009-2011, Tor M. Aamodt, Wilson W.L. Fung, Ali Bakhoda
// The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution. Neither the name of
// The University of British Columbia nor the names of its contributors may be
// used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "icnt_wrapper.h"
#include <assert.h>
#include "../intersim2/globals.hpp"
#include "../intersim2/interconnect_interface.hpp"
#include "local_interconnect.h"
#define MAX_GPUS 8

icnt_create_p icnt_create;
icnt_init_p icnt_init;
icnt_has_buffer_p icnt_has_buffer;
icnt_push_p icnt_push;
icnt_pop_p icnt_pop;

//hyunuk
// icnt_top_p inct_top;

icnt_transfer_p icnt_transfer;
icnt_busy_p icnt_busy;
icnt_display_stats_p icnt_display_stats;
icnt_display_overall_stats_p icnt_display_overall_stats;
icnt_display_state_p icnt_display_state;
icnt_get_flit_size_p icnt_get_flit_size;

unsigned g_network_mode;
char *g_network_config_filename;

struct inct_config g_inct_config;
LocalInterconnect *g_localicnt_interface[MAX_GPUS];
InterconnectInterface *g_icnt_interfaces[MAX_GPUS];
FILE *output_files[MAX_GPUS];

#include "../option_parser.h"

// Wrapper to intersim2 to accompany old icnt_wrapper
// TODO: use delegate/boost/c++11<funtion> instead

static void intersim2_create(int gpu_num, unsigned int n_shader, unsigned int n_mem)
{
  assert(gpu_num < MAX_GPUS);
  g_icnt_interfaces[gpu_num]->CreateInterconnect(n_shader, n_mem);
}

static void intersim2_init(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  g_icnt_interfaces[gpu_num]->Init();
}

static bool intersim2_has_buffer(int gpu_num, unsigned input, unsigned int size)
{
  assert(gpu_num < MAX_GPUS);
  return g_icnt_interfaces[gpu_num]->HasBuffer(input, size);
}

static void intersim2_push(int gpu_num, unsigned input, unsigned output, void *data,
                           unsigned int size)
{
  assert(gpu_num < MAX_GPUS);
  g_icnt_interfaces[gpu_num]->Push(input, output, data, size);
}

static void *intersim2_pop(int gpu_num, unsigned output)
{
  assert(gpu_num < MAX_GPUS);
  return g_icnt_interfaces[gpu_num]->Pop(output);
}

//hyunuk
// static void *intersim2_top(int gpu_num, unsigned output)
// {
//   assert(gpu_num < MAX_GPUS);
//   return g_icnt_interfaces[gpu_num]->Top(output);
// }

static void intersim2_transfer(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  g_icnt_interfaces[gpu_num]->Advance();
}

static bool intersim2_busy(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  return g_icnt_interfaces[gpu_num]->Busy();
}

static void intersim2_display_stats(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  g_icnt_interfaces[gpu_num]->DisplayStats();
}

static void intersim2_display_overall_stats(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  g_icnt_interfaces[gpu_num]->DisplayOverallStats();
}

static void intersim2_display_state(int gpu_num, FILE *fp)
{
  assert(gpu_num < MAX_GPUS);
  g_icnt_interfaces[gpu_num]->DisplayState(fp);
}

static unsigned intersim2_get_flit_size(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  return g_icnt_interfaces[gpu_num]->GetFlitSize();
}

//////////////////////////////////////////////////////

static void LocalInterconnect_create(int gpu_num, unsigned int n_shader,
                                     unsigned int n_mem)
{
  assert(gpu_num < MAX_GPUS);
  g_localicnt_interface[gpu_num]->CreateInterconnect(n_shader, n_mem);
}

static void LocalInterconnect_init(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  g_localicnt_interface[gpu_num]->Init();
}

static bool LocalInterconnect_has_buffer(int gpu_num, unsigned input, unsigned int size)
{
  assert(gpu_num < MAX_GPUS);
  return g_localicnt_interface[gpu_num]->HasBuffer(input, size);
}

static void LocalInterconnect_push(int gpu_num, unsigned input, unsigned output, void *data,
                                   unsigned int size)
{
  assert(gpu_num < MAX_GPUS);
  g_localicnt_interface[gpu_num]->Push(input, output, data, size);
}

static void *LocalInterconnect_pop(int gpu_num, unsigned output)
{
  assert(gpu_num < MAX_GPUS);
  return g_localicnt_interface[gpu_num]->Pop(output);
}

static void LocalInterconnect_transfer(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  g_localicnt_interface[gpu_num]->Advance();
}

static bool LocalInterconnect_busy(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  return g_localicnt_interface[gpu_num]->Busy();
}

static void LocalInterconnect_display_stats(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  g_localicnt_interface[gpu_num]->DisplayStats(output_files[gpu_num]);
}

static void LocalInterconnect_display_overall_stats(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  g_localicnt_interface[gpu_num]->DisplayOverallStats(output_files[gpu_num]);
}

static void LocalInterconnect_display_state(int gpu_num, FILE *fp)
{
  assert(gpu_num < MAX_GPUS);
  g_localicnt_interface[gpu_num]->DisplayState(fp);
}

static unsigned LocalInterconnect_get_flit_size(int gpu_num)
{
  assert(gpu_num < MAX_GPUS);
  return g_localicnt_interface[gpu_num]->GetFlitSize();
}

///////////////////////////

void icnt_reg_options(class OptionParser *opp)
{
  option_parser_register(opp, "-network_mode", OPT_INT32, &g_network_mode,
                         "Interconnection network mode", "1");
  option_parser_register(opp, "-inter_config_file", OPT_CSTR,
                         &g_network_config_filename,
                         "Interconnection network config file", "mesh");

  // parameters for local xbar
  option_parser_register(opp, "-icnt_in_buffer_limit", OPT_UINT32,
                         &g_inct_config.in_buffer_limit, "in_buffer_limit",
                         "64");
  option_parser_register(opp, "-icnt_out_buffer_limit", OPT_UINT32,
                         &g_inct_config.out_buffer_limit, "out_buffer_limit",
                         "64");
  option_parser_register(opp, "-icnt_subnets", OPT_UINT32,
                         &g_inct_config.subnets, "subnets", "2");
  option_parser_register(opp, "-icnt_arbiter_algo", OPT_UINT32,
                         &g_inct_config.arbiter_algo, "arbiter_algo", "1");
  option_parser_register(opp, "-icnt_verbose", OPT_UINT32,
                         &g_inct_config.verbose, "inct_verbose", "0");
  option_parser_register(opp, "-icnt_grant_cycles", OPT_UINT32,
                         &g_inct_config.grant_cycles, "grant_cycles", "1");
}

void icnt_wrapper_init(int gpu_num, FILE *output_file)
{
  assert(gpu_num < MAX_GPUS);
  output_files[gpu_num] = output_file;
  switch (g_network_mode)
  {
  case INTERSIM:
    // FIXME: delete the object: may add icnt_done wrapper
    g_icnt_interfaces[gpu_num] = InterconnectInterface::New(g_network_config_filename);
    icnt_create = intersim2_create;
    icnt_init = intersim2_init;
    icnt_has_buffer = intersim2_has_buffer;
    icnt_push = intersim2_push;
    icnt_pop = intersim2_pop;

    //hyunuk
    // icnt_top = intersim2_top;

    icnt_transfer = intersim2_transfer;
    icnt_busy = intersim2_busy;
    icnt_display_stats = intersim2_display_stats;
    icnt_display_overall_stats = intersim2_display_overall_stats;
    icnt_display_state = intersim2_display_state;
    icnt_get_flit_size = intersim2_get_flit_size;
    break;
  case LOCAL_XBAR:
    // assert(0);
    g_localicnt_interface[gpu_num] = LocalInterconnect::New(g_inct_config);
    icnt_create = LocalInterconnect_create;
    icnt_init = LocalInterconnect_init;
    icnt_has_buffer = LocalInterconnect_has_buffer;
    icnt_push = LocalInterconnect_push;
    icnt_pop = LocalInterconnect_pop;
    icnt_transfer = LocalInterconnect_transfer;
    icnt_busy = LocalInterconnect_busy;
    icnt_display_stats = LocalInterconnect_display_stats;
    icnt_display_overall_stats = LocalInterconnect_display_overall_stats;
    icnt_display_state = LocalInterconnect_display_state;
    icnt_get_flit_size = LocalInterconnect_get_flit_size;
    break;
  default:
    assert(0);
    break;
  }
}