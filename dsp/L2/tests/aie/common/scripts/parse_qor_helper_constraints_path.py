# Copyright (C) 2026, Advanced Micro Devices, Inc.
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
import json
import sys

if __name__ == "__main__":
    params_file = sys.argv[1].strip()
    case_name = sys.argv[2].strip()
    data = json.load(open(params_file, "r"))[case_name]

    if "QOR_HELPER_CONSTRAINTS_PATH_OVERRIDE" in data:
        print(data["QOR_HELPER_CONSTRAINTS_PATH_OVERRIDE"])
    else:
        print("")
