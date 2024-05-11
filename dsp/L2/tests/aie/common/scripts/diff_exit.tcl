#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#
set fileDir             [lindex $argv 0]

# locate diff.txt
set diffFile "${fileDir}logs/diff.txt"
set funcPassPhrase "identical"

# exit 1 if functional fail, else exit 0
if {[catch {exec grep -i $funcPassPhrase -c $diffFile}]} {
    exit 1
} else {
    exit 0
}