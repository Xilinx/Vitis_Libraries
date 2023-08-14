# Copyright (C) 2023, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

def write_system_cfg(args):
    NUM_PORTS = args['NUM_PORTS']
    r = '[connectivity]\n'
    for i in range(NUM_PORTS):
        r += f'stream_connect = mm2s_4d_hsk_1.sync_o{i}:s2mm_4d_hsk_1.sync_i{i}\n'
        r += f'stream_connect = mm2s_4d_hsk_1.axis_o{i}:s2mm_mp_1.s{i}\n'
        r += f'stream_connect = s2mm_4d_hsk_1.sync_o{i}:mm2s_4d_hsk_1.sync_i{i}\n'
        r += f'stream_connect = mm2s_mp_1.s{i}:s2mm_4d_hsk_1.axis_i{i}\n'
    return {'source': r}
