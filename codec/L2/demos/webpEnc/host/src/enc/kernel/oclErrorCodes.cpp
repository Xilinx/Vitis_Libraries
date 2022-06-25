
/*
 * Copyright 2021 Xilinx, Inc.
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

#include <map>
#include <string>

#include <CL/cl.h>

#define TO_STRING(x) #x

static const std::pair<cl_int, std::string> map_pairs[] = {
    std::make_pair(CL_SUCCESS, TO_STRING(CL_SUCCESS)),
    std::make_pair(CL_DEVICE_NOT_FOUND, TO_STRING(CL_DEVICE_NOT_FOUND)),
    std::make_pair(CL_DEVICE_NOT_AVAILABLE, TO_STRING(CL_DEVICE_NOT_AVAILABLE)),
    std::make_pair(CL_COMPILER_NOT_AVAILABLE, TO_STRING(CL_COMPILER_NOT_AVAILABLE)),
    std::make_pair(CL_MEM_OBJECT_ALLOCATION_FAILURE, TO_STRING(CL_MEM_OBJECT_ALLOCATION_FAILURE)),
    std::make_pair(CL_OUT_OF_RESOURCES, TO_STRING(CL_OUT_OF_RESOURCES)),
    std::make_pair(CL_OUT_OF_HOST_MEMORY, TO_STRING(CL_OUT_OF_HOST_MEMORY)),
    std::make_pair(CL_PROFILING_INFO_NOT_AVAILABLE, TO_STRING(CL_PROFILING_INFO_NOT_AVAILABLE)),
    std::make_pair(CL_MEM_COPY_OVERLAP, TO_STRING(CL_MEM_COPY_OVERLAP)),
    std::make_pair(CL_IMAGE_FORMAT_MISMATCH, TO_STRING(CL_IMAGE_FORMAT_MISMATCH)),
    std::make_pair(CL_IMAGE_FORMAT_NOT_SUPPORTED, TO_STRING(CL_IMAGE_FORMAT_NOT_SUPPORTED)),
    std::make_pair(CL_BUILD_PROGRAM_FAILURE, TO_STRING(CL_BUILD_PROGRAM_FAILURE)),
    std::make_pair(CL_MAP_FAILURE, TO_STRING(CL_MAP_FAILURE)),
    std::make_pair(CL_MISALIGNED_SUB_BUFFER_OFFSET, TO_STRING(CL_MISALIGNED_SUB_BUFFER_OFFSET)),
    std::make_pair(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, TO_STRING(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_W)),
    std::make_pair(CL_INVALID_VALUE, TO_STRING(CL_INVALID_VALUE)),
    std::make_pair(CL_INVALID_DEVICE_TYPE, TO_STRING(CL_INVALID_DEVICE_TYPE)),
    std::make_pair(CL_INVALID_PLATFORM, TO_STRING(CL_INVALID_PLATFORM)),
    std::make_pair(CL_INVALID_DEVICE, TO_STRING(CL_INVALID_DEVICE)),
    std::make_pair(CL_INVALID_CONTEXT, TO_STRING(CL_INVALID_CONTEXT)),
    std::make_pair(CL_INVALID_QUEUE_PROPERTIES, TO_STRING(CL_INVALID_QUEUE_PROPERTIES)),
    std::make_pair(CL_INVALID_COMMAND_QUEUE, TO_STRING(CL_INVALID_COMMAND_QUEUE)),
    std::make_pair(CL_INVALID_HOST_PTR, TO_STRING(CL_INVALID_HOST_PTR)),
    std::make_pair(CL_INVALID_MEM_OBJECT, TO_STRING(CL_INVALID_MEM_OBJECT)),
    std::make_pair(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, TO_STRING(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)),
    std::make_pair(CL_INVALID_IMAGE_SIZE, TO_STRING(CL_INVALID_IMAGE_SIZE)),
    std::make_pair(CL_INVALID_SAMPLER, TO_STRING(CL_INVALID_SAMPLER)),
    std::make_pair(CL_INVALID_BINARY, TO_STRING(CL_INVALID_BINARY)),
    std::make_pair(CL_INVALID_BUILD_OPTIONS, TO_STRING(CL_INVALID_BUILD_OPTIONS)),
    std::make_pair(CL_INVALID_PROGRAM, TO_STRING(CL_INVALID_PROGRAM)),
    std::make_pair(CL_INVALID_PROGRAM_EXECUTABLE, TO_STRING(CL_INVALID_PROGRAM_EXECUTABLE)),
    std::make_pair(CL_INVALID_KERNEL_NAME, TO_STRING(CL_INVALID_KERNEL_NAME)),
    std::make_pair(CL_INVALID_KERNEL_DEFINITION, TO_STRING(CL_INVALID_KERNEL_DEFINITION)),
    std::make_pair(CL_INVALID_KERNEL, TO_STRING(CL_INVALID_KERNEL)),
    std::make_pair(CL_INVALID_ARG_INDEX, TO_STRING(CL_INVALID_ARG_INDEX)),
    std::make_pair(CL_INVALID_ARG_VALUE, TO_STRING(CL_INVALID_ARG_VALUE)),
    std::make_pair(CL_INVALID_ARG_SIZE, TO_STRING(CL_INVALID_ARG_SIZE)),
    std::make_pair(CL_INVALID_KERNEL_ARGS, TO_STRING(CL_INVALID_KERNEL_ARGS)),
    std::make_pair(CL_INVALID_WORK_DIMENSION, TO_STRING(CL_INVALID_WORK_DIMENSION)),
    std::make_pair(CL_INVALID_WORK_GROUP_SIZE, TO_STRING(CL_INVALID_WORK_GROUP_SIZE)),
    std::make_pair(CL_INVALID_WORK_ITEM_SIZE, TO_STRING(CL_INVALID_WORK_ITEM_SIZE)),
    std::make_pair(CL_INVALID_GLOBAL_OFFSET, TO_STRING(CL_INVALID_GLOBAL_OFFSET)),
    std::make_pair(CL_INVALID_EVENT_WAIT_LIST, TO_STRING(CL_INVALID_EVENT_WAIT_LIST)),
    std::make_pair(CL_INVALID_EVENT, TO_STRING(CL_INVALID_EVENT)),
    std::make_pair(CL_INVALID_OPERATION, TO_STRING(CL_INVALID_OPERATION)),
    std::make_pair(CL_INVALID_GL_OBJECT, TO_STRING(CL_INVALID_GL_OBJECT)),
    std::make_pair(CL_INVALID_BUFFER_SIZE, TO_STRING(CL_INVALID_BUFFER_SIZE)),
    std::make_pair(CL_INVALID_MIP_LEVEL, TO_STRING(CL_INVALID_MIP_LEVEL)),
    std::make_pair(CL_INVALID_GLOBAL_WORK_SIZE, TO_STRING(CL_INVALID_GLOBAL_WORK_SIZE)),
    std::make_pair(CL_INVALID_PROPERTY, TO_STRING(CL_INVALID_PROPERTY))};

static const std::map<cl_int, std::string> oclErrorCodes(map_pairs,
                                                         map_pairs + sizeof(map_pairs) / sizeof(map_pairs[0]));

extern "C" const char* oclErrorCode(cl_int code) {
    std::map<cl_int, std::string>::const_iterator iter = oclErrorCodes.find(code);
    if (iter == oclErrorCodes.end())
        return "UNKNOWN ERROR";
    else
        return iter->second.c_str();
}
