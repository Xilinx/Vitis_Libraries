/*
*Copyright 2022 Xilinx, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
*/

#include "xrm.h"
#include <stdlib.h>

xrmContext xrmCreateContext(uint32_t xrmApiVersion) {
    return malloc(16);
}

int32_t xrmLoadOneDevice(xrmContext context, int32_t deviceId, char* xclbinFileName) {
    return 0;
}

bool xrmCuRelease(xrmContext context, xrmCuResource* cuRes) {
    return true;
}

int32_t xrmCuAlloc(xrmContext context, xrmCuProperty* cuProp, xrmCuResource* cuRes) {
    return 1;
}

int32_t xrmCheckCuAvailableNum(xrmContext context, xrmCuProperty* cuProp) {
    return 0;
}

int32_t xrmDestroyContext(xrmContext context) {
    return 0;
}

int32_t xrmCuListAlloc(xrmContext context, xrmCuListProperty* cuListProp, xrmCuListResource* cuListRes) {
    return 0;
}

int32_t xrmUnloadOneDevice(xrmContext context, int32_t deviceId) {
    return 0;
}
