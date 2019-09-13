/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __XCLHOST_H__
#define __XCLHOST_H__

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>
#include <CL/cl_ext_xilinx.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#define MSTR_(m) #m
#define MSTR(m) MSTR_(m)

unsigned long read_binary_file(const char* fname, void** buffer);

cl_int init_hardware(cl_context* context,
                     cl_device_id* device_id,
                     cl_command_queue* cmd_queue,
                     cl_command_queue_properties queue_props,
                     const char* shell_name);

cl_int load_binary(cl_program* program, cl_context context, cl_device_id device_id, const char* xclbin);

#endif // !defined(__XCLHOST_H__)
