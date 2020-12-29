#!/bin/bash
./accel-sim.out -trace ../../../util/tracer_nvbit/traces/ -config ../../gpgpu-sim/configs/tested-cfgs/SM75_RTX2060/gpgpusim.config -config ../../../gpu-simulator/configs/tested-cfgs/SM75_RTX2060/trace.config -num_gpus 2 -cxl_config ../../cxl-config/cxl.config
