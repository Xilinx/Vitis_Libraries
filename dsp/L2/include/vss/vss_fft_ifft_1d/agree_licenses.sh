#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#!/bin/bash
# get the path of this file
ROOT_DIR=$(dirname $(realpath $0))
#check if script has argument
OUT_DIR=$(dirname $(realpath $0))
if [ $# -ge 1 ]; then
    OUT_DIR=$1
fi
cat $ROOT_DIR/disclaimer.txt


# Print a message to the screen
echo "Do you agree to the license in disclaimer.txt?"

# Get input from the user
read -p "Yes/No: " choice

# Perform different tasks based on the input
case $choice in
    yes)
        echo "I have read and accepted the terms and conditions in the disclaimer.txt file" >> $OUT_DIR/agreement.txt
        ;;
    Yes)
        echo "I have read and accepted the terms and conditions in the disclaimer.txt file" >> $OUT_DIR/agreement.txt
        ;;
    No)
        echo "Please agree to the license to use the VSS IP."
        exit 1
        ;;
    no)
        echo "Please agree to the license to use the VSS IP."
        exit 1
        ;;
    *)
        echo "You cannot run the VSS IP without agreeing to the license"
        exit 1
        ;;
esac
