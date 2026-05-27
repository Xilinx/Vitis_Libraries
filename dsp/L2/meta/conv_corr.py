#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
import aie_common as com

#### naming ####
#
# Name functions with prefix
#   validate_ for validators, returning boolean result and error message as a tuple.
#   update_ for updators, returning object with default value and refined candidate constraints.
#   info_ for creating information based on parameters
#   fn_ for internal functions
#
# Name function arguments as template parameters, when possible
# so the code matches easier with API definition.


# Example of validator.
#
# The parameter itself will be passed as first argument for validator functions.
# These functions can have extra parameters as arguments, as specified as last part of in `validator`.
# These extra parameters must appear before current one in "parameters" section.
#
# A validator function returns a dictionary, with required boolean key "is_valid",
# and "err_message" if "is_valid" is False.
#

FUNCT_CORR = 0
FUNCT_CONV = 1
COMPUTE_FULL = 0
COMPUTE_SAME = 1
COMPUTE_VALID = 2
AIE_LOAD_SIZE = 256 / 8  # 32 Bytes
AIE_LOAD_SIZE_IN_BITS = 256
TP_NUM_FRAMES_MIN = 1
TP_NUM_FRAMES_MAX = 1
TP_CASC_LEN_MIN = 1
TP_CASC_LEN_MAX = 32
TP_PHASES_MIN = 1
TP_PHASES_MAX = 16
TP_G_LEN_iobuffer_max = 256
TP_F_LEN_min_for_stream = 512 # The stream-based implementation requires a minimum length of 512 for TP_F_LEN to to ensure the stream implementation flushes out partial results
MAX_NUM_OF_STREAMS = 2 # maximum number of streams per core, used in phases validation

sampleSize = {
    "int8": 8,
    "int16": 16,
    "int32": 32,
    "cint16": 32,
    "cint32": 64,
    "float": 32,
    "cfloat": 64,
    "bfloat16": 16,
}


#######################################################
########### AIE_VARIANT Updater and Validator #########
#######################################################
def update_AIE_VARIANT(args):
    return fn_update_AIE_VARIANT()


def fn_update_AIE_VARIANT():
    legal_set_AIE_VARIANT = [com.AIE, com.AIE_ML, com.AIE_MLv2]

    param_dict = {}
    param_dict.update({"name": "AIE_VARIANT"})
    param_dict.update({"enum": legal_set_AIE_VARIANT})
    return param_dict


def validate_AIE_VARIANT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_validate_AIE_VARIANT(AIE_VARIANT)


def fn_validate_AIE_VARIANT(AIE_VARIANT):
    param_dict = fn_update_AIE_VARIANT()
    legal_set_AIE_VARIANT = param_dict["enum"]
    return com.validate_legal_set(legal_set_AIE_VARIANT, "AIE_VARIANT", AIE_VARIANT)


#######################################################
############# TP_API Updater and Validator ############
#######################################################
def update_TP_API(args):
  AIE_VARIANT=args["AIE_VARIANT"]
  return fn_update_api(AIE_VARIANT)

def fn_update_api(AIE_VARIANT):
  legal_set_api=[com.API_BUFFER, com.API_STREAM]
  if AIE_VARIANT in [com.AIE_ML,com.AIE_MLv2 ]:
     legal_set_api=[com.API_BUFFER]

  param_dict={
    "name" : "TP_API",
    "enum" : legal_set_api
  }
  return param_dict

def validate_TP_API(args):
  AIE_VARIANT = args["AIE_VARIANT"]
  TP_API = args["TP_API"]
  return fn_validate_api(AIE_VARIANT, TP_API)

def fn_validate_api(AIE_VARIANT, TP_API):
  param_dict = fn_update_api(AIE_VARIANT)
  return(com.validate_legal_set(param_dict["enum"], "TP_API", TP_API))

#######################################################
########## TT_DATA_F Updater and Validator ############
#######################################################
def update_TT_DATA_F(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    return fn_update_data_type_f(AIE_VARIANT, TP_API)


def fn_update_data_type_f(AIE_VARIANT, TP_API):
    valid_types = ["int8", "int16", "int32", "float", "bfloat16", "cfloat", "cint16", "cint32"]
    if AIE_VARIANT == com.AIE:
        if TP_API == com.API_STREAM:
            valid_types = ["cint16"]
        else:
            valid_types.remove("int8")
            valid_types.remove("bfloat16")
    else:
        valid_types.remove("cfloat")

    param_dict={
        "name" : "TT_DATA_F",
        "enum" : valid_types
    }
    return param_dict

def validate_TT_DATA_F(args):
    TT_DATA_F = args["TT_DATA_F"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    return fn_validate_data_type_f(TT_DATA_F, AIE_VARIANT, TP_API)


def fn_validate_data_type_f(TT_DATA_F, AIE_VARIANT, TP_API):
    param_dict = fn_update_data_type_f(AIE_VARIANT, TP_API)
    return com.validate_legal_set(param_dict["enum"], "TT_DATA_F", TT_DATA_F)


#######################################################
########## TT_DATA_G Updater and Validator ############
#######################################################
def update_TT_DATA_G(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_F = args["TT_DATA_F"]
    TP_API = args["TP_API"]
    return fn_update_data_type_g(AIE_VARIANT, TT_DATA_F, TP_API)


def fn_update_data_type_g(AIE_VARIANT, TT_DATA_F, TP_API):
    valid_types = fn_get_valid_g_types_for_mul(TT_DATA_F, AIE_VARIANT, TP_API)
    param_dict = {"name": "TT_DATA_G", "enum": valid_types}
    return param_dict


def validate_TT_DATA_G(args):
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    return fn_validate_data_type_g(TT_DATA_G, TT_DATA_F, AIE_VARIANT, TP_API)


def fn_validate_data_type_g(TT_DATA_G, TT_DATA_F, AIE_VARIANT, TP_API):
    param_dict = fn_update_data_type_g(AIE_VARIANT, TT_DATA_F, TP_API)
    return com.validate_legal_set(param_dict["enum"], "TT_DATA_G", TT_DATA_G)


#######################################################
######### TT_DATA_OUT Updater and Validator ###########
#######################################################
def update_TT_DATA_OUT(args):
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    return fn_update_data_type_out(TT_DATA_F, TT_DATA_G)


def fn_update_data_type_out(TT_DATA_F, TT_DATA_G):
    valid_types = fn_get_valid_out_types_for_mul(TT_DATA_F, TT_DATA_G)
    param_dict = {"name": "TT_DATA_OUT", "enum": valid_types}
    return param_dict


def validate_TT_DATA_OUT(args):
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    return fn_validate_data_type_out(TT_DATA_OUT, TT_DATA_F, TT_DATA_G)


def fn_validate_data_type_out(TT_DATA_OUT, TT_DATA_F, TT_DATA_G):
    param_dict = fn_update_data_type_out(TT_DATA_F, TT_DATA_G)
    return com.validate_legal_set(param_dict["enum"], "TT_DATA_OUT", TT_DATA_OUT)


#######################################################
######### TP_FUNCT_TYPE Updater and Validator #########
#######################################################
def update_TP_FUNCT_TYPE(args):
    return fn_update_funct_type()


def fn_update_funct_type():
    param_dict = {"name": "TP_FUNCT_TYPE", "enum": [FUNCT_CORR, FUNCT_CONV]}
    return param_dict


def validate_TP_FUNCT_TYPE(args):
    TP_FUNCT_TYPE = args["TP_FUNCT_TYPE"]
    return fn_validate_funct_type(TP_FUNCT_TYPE)


def fn_validate_funct_type(TP_FUNCT_TYPE):
    param_dict = fn_update_funct_type()
    return com.validate_legal_set(param_dict["enum"], "TP_FUNCT_TYPE", TP_FUNCT_TYPE)


#######################################################
######## TP_COMPUTE_MODE Updater and Validator ########
#######################################################
def update_TP_COMPUTE_MODE(args):
    TP_API = args["TP_API"]
    return fn_update_compute_mode(TP_API)


def fn_update_compute_mode(TP_API):
    if TP_API == com.API_STREAM:
        param_dict = {"name": "TP_COMPUTE_MODE", "enum": [COMPUTE_VALID]}

    elif TP_API == com.API_BUFFER:
        param_dict = {
            "name": "TP_COMPUTE_MODE",
            "enum": [COMPUTE_FULL, COMPUTE_SAME, COMPUTE_VALID],
        }
    return param_dict


def validate_TP_COMPUTE_MODE(args):
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_API = args["TP_API"]
    return fn_validate_compute_mode(TP_COMPUTE_MODE, TP_API)


def fn_validate_compute_mode(TP_COMPUTE_MODE, TP_API):
    param_dict = fn_update_compute_mode(TP_API)
    return com.validate_legal_set(
        param_dict["enum"], "TP_COMPUTE_MODE", TP_COMPUTE_MODE
    )


#######################################################
########### TP_F_LEN Updater and Validator ############
#######################################################
def update_TP_F_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"] if ("TP_F_LEN" in args and args["TP_F_LEN"] )else 0
    return fn_update_f_len(TP_F_LEN, TT_DATA_G, TT_DATA_F, TT_DATA_OUT, TP_COMPUTE_MODE, TP_API, AIE_VARIANT)

def fn_update_f_len(TP_F_LEN, TT_DATA_G, TT_DATA_F, TT_DATA_OUT, TP_COMPUTE_MODE, TP_API, AIE_VARIANT):
    # Alignment granularity is base load size (256 bits / 8 / sizeof)
    base_load_f = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_F)

    # Minimum is 2x base load for buffer mode, stream has different minimum
    if TP_API == com.API_BUFFER:
        min_elems_f = (com.k_max_read_write_bytes[AIE_VARIANT] << 1) // com.fn_size_by_byte(TT_DATA_F)
    else:
        min_elems_f = TP_F_LEN_min_for_stream
    
    # Calculate elems_per_load for TT_DATA_G to ensure TP_F_LEN can accommodate TP_G_LEN minimum
    if TP_API == com.API_STREAM:
        elems_per_load_g = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_G)
    else:
        elems_per_load_g = (com.k_max_read_write_bytes[AIE_VARIANT] << 1) // com.fn_size_by_byte(TT_DATA_G)

    TP_F_LEN_max = (com.k_data_memory_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_F)) # Assume always SINGLE_BUFF is ON.
    TP_F_LEN_max_pingpong_buff= int(com.k_data_memory_bytes[AIE_VARIANT] >> 1)// (com.fn_size_by_byte(TT_DATA_F))

    # Output-aware F_max: output port (single copy) must fit within one MG
    # NOTE: The graph auto-enables single_buffer for F and Out independently when they exceed half memory
    # Only apply output constraint when the output buffer won't trigger single_buffer on its own
    if TP_API == com.API_BUFFER:
        sizeof_out = com.fn_size_by_byte(TT_DATA_OUT)
        max_out_elems = com.k_data_memory_bytes[AIE_VARIANT] // sizeof_out
        pingpong_threshold_elems = (com.k_data_memory_bytes[AIE_VARIANT] >> 1) // sizeof_out
        G_min = (com.k_max_read_write_bytes[AIE_VARIANT] << 1) // com.fn_size_by_byte(TT_DATA_G)

        # Calculate the maximum output size based on compute mode
        # This represents the worst-case output buffer requirement
        if TP_COMPUTE_MODE == COMPUTE_FULL:
            TP_F_LEN_max_out = max_out_elems - G_min + 1
            max_out_elems_at_max_f = TP_F_LEN_max + G_min - 1  # out = F + G - 1 at max F with min G
        elif TP_COMPUTE_MODE == COMPUTE_SAME:
            TP_F_LEN_max_out = max_out_elems
            max_out_elems_at_max_f = TP_F_LEN_max                # out = F at max F
        else:  # VALID
            TP_F_LEN_max_out = max_out_elems + G_min - 1
            max_out_elems_at_max_f = TP_F_LEN_max - G_min + 1  # out = F - G + 1 at max F with min G
        
        # Only apply output constraint if the output won't trigger single_buffer
        # If max_out_elems_at_max_f > threshold, output will use single_buffer, no memory collision
        if max_out_elems_at_max_f <= pingpong_threshold_elems:
            TP_F_LEN_max = min(TP_F_LEN_max, TP_F_LEN_max_out)
        # else: output will use single_buffer, F can use full memory without collision

    # Floor max to base_load_f alignment so the reported maximum is always a valid value
    TP_F_LEN_max = com.FLOOR(TP_F_LEN_max, base_load_f)

    # Minimum must be at least elems_per_load_g to satisfy TP_G_LEN <= TP_F_LEN constraint
    minimum = max(min_elems_f, elems_per_load_g)
    
    param_dict={
        "name" : "TP_F_LEN",
        "minimum" : minimum,
        "maximum" : TP_F_LEN_max if TP_API == com.API_BUFFER else 2**31,
        "maximum_pingpong_buf" : TP_F_LEN_max_pingpong_buff
    }
    # Round up to nearest multiple of base_load_f (alignment granularity)
    TP_F_LEN_act = com.CEIL(TP_F_LEN, base_load_f) if TP_F_LEN > 0 else base_load_f
    if TP_F_LEN_act < param_dict["minimum"]: param_dict["actual"] = param_dict["minimum"]
    elif TP_F_LEN_act > param_dict["maximum"]: param_dict["actual"] = com.FLOOR(param_dict["maximum"], base_load_f)
    else: param_dict["actual"] = TP_F_LEN_act
    return param_dict

def validate_TP_F_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"]
    return fn_validate_f_len(AIE_VARIANT, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_API, TP_F_LEN)


def fn_validate_f_len(AIE_VARIANT, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_API, TP_F_LEN):
    # Alignment granularity is base load size (256 bits / 8 / sizeof)
    base_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_F)

    param_dict = fn_update_f_len(TP_F_LEN, TT_DATA_G, TT_DATA_F, TT_DATA_OUT, TP_COMPUTE_MODE, TP_API, AIE_VARIANT)
    
    # Check if single_buffer will be auto-enabled due to F buffer size exceeding ping-pong threshold
    # When this happens, use full memory constraint instead of ping-pong constraint
    f_buf_bytes = TP_F_LEN * com.fn_size_by_byte(TT_DATA_F)
    pingpong_threshold_bytes = com.k_data_memory_bytes[AIE_VARIANT] >> 1

    if TP_API == com.API_BUFFER and f_buf_bytes <= pingpong_threshold_bytes:
        # Buffer mode, ping-pong territory → F must fit in half memory
        max_allowed = param_dict["maximum_pingpong_buf"]
    else:
        # Buffer mode with single_buffer auto-enabled, or stream → use full maximum
        max_allowed = param_dict["maximum"]
    
    range_TP_F_LEN = [param_dict["minimum"], max_allowed]

    # Enforce base_load alignment (not doubled) - allows multiples of base_load that are >= minimum
    # For int16 buffer mode: minimum=32, but alignment=16, so 48, 64, 80, 96 are all valid
    if TP_F_LEN % base_load != 0:
        return com.isError(f"TP_F_LEN ({TP_F_LEN}) should be multiples of \"{base_load}\" (base alignment for TT_DATA_F \"{TT_DATA_F}\").")

    return com.validate_range(range_TP_F_LEN, "TP_F_LEN", TP_F_LEN)


#######################################################
########### TP_G_LEN Updater and Validator ############
#######################################################
def update_TP_G_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"]
    TP_G_LEN = args["TP_G_LEN"] if ("TP_G_LEN" in args and args["TP_G_LEN"]) else 0
    return fn_update_g_len(TP_G_LEN, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_API, AIE_VARIANT)

def fn_update_g_len(TP_G_LEN, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_API, AIE_VARIANT):

    # Alignment granularity is base load size (256 bits / 8 / sizeof(TT_DATA_G))
    # TP_G_LEN must be a multiple of base_load for both stream and buffer.
    base_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_G)
    
    # Minimum is 2x base load for buffer mode, 1x for stream mode
    if TP_API == com.API_STREAM:
        min_elems = base_load
    else:
        min_elems = (com.k_max_read_write_bytes[AIE_VARIANT] << 1) // com.fn_size_by_byte(TT_DATA_G)

    # G port MG limit (single copy must fit in one MG)
    G_max_port = com.k_data_memory_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_G)

    # Output-aware G_max: output port (single copy) must fit within one MG
    # NOTE: Similar to F buffer, check if output will trigger single_buffer before applying constraint
    if TP_API == com.API_BUFFER:
        sizeof_out = com.fn_size_by_byte(TT_DATA_OUT)
        max_out_elems = com.k_data_memory_bytes[AIE_VARIANT] // sizeof_out
        pingpong_threshold_bytes = com.k_data_memory_bytes[AIE_VARIANT] >> 1

        # Calculate worst-case output size for current TP_F_LEN
        if TP_COMPUTE_MODE == COMPUTE_FULL:
            # out = F + G - 1, with current F and max G
            max_out_with_max_g = TP_F_LEN + G_max_port - 1
            out_buffer_bytes = max_out_with_max_g * sizeof_out

            # Only apply output constraint if output won't trigger single_buffer
            if out_buffer_bytes <= pingpong_threshold_bytes:
                G_max_out = max_out_elems - TP_F_LEN + 1
            else:
                G_max_out = G_max_port  # output uses single_buffer, no constraint
        elif TP_COMPUTE_MODE == COMPUTE_SAME:
            # out = F, independent of G — no output-based restriction on G
            G_max_out = G_max_port
        else:  # VALID
            # out = F - G + 1, larger G shrinks output, no restriction from output
            G_max_out = G_max_port

        maximum = min(TP_F_LEN, G_max_port, G_max_out)
    else:
        maximum = min(TP_G_LEN_iobuffer_max, TP_F_LEN)

    # Floor max to base_load alignment so the reported maximum is always a valid value
    maximum = com.FLOOR(maximum, base_load)

    # Minimum is min_elems but capped at maximum to avoid invalid ranges
    minimum = min(min_elems, maximum)
    
    param_dict={
        "name" : "TP_G_LEN",
        "minimum" : minimum,
        "maximum" : maximum
    }
    # Round up to nearest multiple of base_load (alignment granularity)
    TP_G_LEN_act = com.CEIL(TP_G_LEN, base_load) if TP_G_LEN > 0 else base_load
    if TP_G_LEN_act < param_dict["minimum"]: param_dict["actual"] = param_dict["minimum"]
    elif TP_G_LEN_act > param_dict["maximum"]: param_dict["actual"] = com.FLOOR(param_dict["maximum"], base_load)
    else: param_dict["actual"] = TP_G_LEN_act
    return param_dict

def validate_TP_G_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"]
    TP_G_LEN = args["TP_G_LEN"] if args["TP_G_LEN"] else 0
    return fn_validate_g_len(AIE_VARIANT, TP_API, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN)

def fn_validate_g_len(AIE_VARIANT, TP_API, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN):
    # Alignment granularity is base load size (256 bits / 8 / sizeof)
    base_load = com.k_max_read_write_bytes[AIE_VARIANT] // com.fn_size_by_byte(TT_DATA_G)

    param_dict = fn_update_g_len(TP_G_LEN, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_API, AIE_VARIANT)
    range_TP_G_LEN = [param_dict["minimum"], param_dict["maximum"]]

    # Enforce base_load alignment (not doubled) - allows multiples of base_load that are >= minimum
    # For int16 buffer mode: minimum=32, but alignment=16, so 80 is valid (80 >= 32 and 80 % 16 == 0)
    if TP_G_LEN % base_load != 0:
        return com.isError(
            f"TP_G_LEN ({TP_G_LEN}) must be a multiple of {base_load} "
            f"(= kMaxBytesLoadOnAie / sizeof(TT_DATA_G=\"{TT_DATA_G}\"))."
        )

    # Check 2: TP_G_LEN must be <= TP_F_LEN and within [minimum, maximum].
    if TP_G_LEN > TP_F_LEN:
        return com.isError(f"TP_G_LEN cannot be greater than TP_F_LEN")

    # Check 3 (stream only): TP_G_LEN % kMinGLen == 0,
    #   kMinGLen = TP_PHASES * kLanes * (kPoints / kStreamsPerCore).
    #   Enforced in fn_validate_phases where TP_PHASES is available.
    return com.validate_range(range_TP_G_LEN, "TP_G_LEN", TP_G_LEN)


#######################################################
########## TP_NUM_FRAMES Updater and Validator ########
#######################################################
def update_TP_NUM_FRAMES(args):
    return fn_update_num_frames()


def fn_update_num_frames():
    param_dict = {
        "name": "TP_NUM_FRAMES",
        "minimum": TP_NUM_FRAMES_MIN,
        "maximum": TP_NUM_FRAMES_MAX,
    }
    return param_dict


def validate_TP_NUM_FRAMES(args):
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    return fn_validate_num_frames(TP_NUM_FRAMES)


def fn_validate_num_frames(TP_NUM_FRAMES):
    param_dict = fn_update_num_frames()
    range_TP_NUM_FRAMES = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_NUM_FRAMES, "TP_NUM_FRAMES", TP_NUM_FRAMES)


#######################################################
########## TP_CASC_LEN Updater and Validator ##########
#######################################################
def update_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_API = args["TP_API"]
    return fn_update_casc_len(AIE_VARIANT, TT_DATA_F, TT_DATA_G, TP_G_LEN, TP_API)


def fn_update_casc_len(AIE_VARIANT, TT_DATA_F, TT_DATA_G, TP_G_LEN, TP_API):
    legal_set = []

    if TP_API == com.API_STREAM:
        muls = getNumMuls(TT_DATA_F, TT_DATA_G, AIE_VARIANT)
        # Valid cascade lengths: gLen/muls (max throughput), gLen/(muls*2), gLen/(muls*4)
        # Matches fnCheckCascLen in conv_corr_traits.hpp
        legal_set = [TP_G_LEN // (muls << 2), TP_G_LEN // (muls << 1), TP_G_LEN // muls]
    elif TP_API == com.API_BUFFER:
        legal_set = [1]

    legal_set = [val for val in legal_set.copy() if val >= TP_CASC_LEN_MIN]
    legal_set = [val for val in legal_set.copy() if val <= TP_CASC_LEN_MAX]
    legal_set = list(dict.fromkeys(legal_set))  # remove duplicates, preserve order

    param_dict = {"name": "TP_CASC_LEN", "enum": legal_set}
    return param_dict


def validate_TP_CASC_LEN(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_API = args["TP_API"]
    return fn_validate_casc_len(AIE_VARIANT, TT_DATA_F, TT_DATA_G, TP_CASC_LEN, TP_G_LEN, TP_API)


def fn_validate_casc_len(AIE_VARIANT, TT_DATA_F, TT_DATA_G, TP_CASC_LEN, TP_G_LEN, TP_API):
    param_dict = fn_update_casc_len(AIE_VARIANT, TT_DATA_F, TT_DATA_G, TP_G_LEN, TP_API)

    legal_set_casc_len = param_dict["enum"]
    return com.validate_legal_set(legal_set_casc_len, "TP_CASC_LEN", TP_CASC_LEN)


#######################################################
########## TP_PHASES Updater and Validator ############
#######################################################
def update_TP_PHASES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_API = args["TP_API"]
    return fn_update_phases(
        AIE_VARIANT, TT_DATA_F, TT_DATA_G, TP_G_LEN, TP_API, TP_CASC_LEN
    )

def fn_update_phases(AIE_VARIANT, TT_DATA_F,TT_DATA_G,TP_G_LEN, TP_API, TP_CASC_LEN):
    muls = getNumMuls(TT_DATA_F, TT_DATA_G, AIE_VARIANT)
    legal_set = [i for i in range(TP_PHASES_MIN, TP_PHASES_MAX+1)]
    legal_set = [i for i in legal_set.copy() if com.fn_is_power_of_two(i)]

    if TP_API == com.API_STREAM:
        if TP_CASC_LEN != TP_G_LEN // muls:
            # Phases > 1 require cascLen == G_LEN / muls (maximum cascade = 1GSPS per phase)
            legal_set = [1]
        else:
            # Filter to phases where G_LEN satisfies kMinGLen constraint.
            # kMinGLen depends on phases and on G_LEN itself (through kStreamsPerCore),
            # so evaluate per-phase. MAC4_ROT always uses kLanes=4 for AIE1 stream.
            kLanes = 4
            kPoints = muls // kLanes
            def g_len_compatible(p):
                kStreamPerCoreVar = (muls * p) >> 1
                kStreamsPerCore = 1 if TP_G_LEN > kStreamPerCoreVar else MAX_NUM_OF_STREAMS
                kMinGLen = p * kLanes * (kPoints // kStreamsPerCore)
                return TP_G_LEN >= kMinGLen and TP_G_LEN % kMinGLen == 0
            legal_set = [p for p in legal_set if g_len_compatible(p)]
            if not legal_set:
                legal_set = [1]
    elif TP_API == com.API_BUFFER:
        legal_set = [1]

    param_dict = {"name": "TP_PHASES", "enum": legal_set}
    return param_dict

def validate_TP_PHASES(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TP_PHASES = args["TP_PHASES"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_API = args["TP_API"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    return fn_validate_phases(
        AIE_VARIANT, TT_DATA_F, TT_DATA_G, TP_PHASES, TP_G_LEN, TP_API, TP_CASC_LEN
    )


def fn_validate_phases(
    AIE_VARIANT, TT_DATA_F, TT_DATA_G, TP_PHASES, TP_G_LEN, TP_API, TP_CASC_LEN
):
    param_dict = fn_update_phases(
        AIE_VARIANT, TT_DATA_F, TT_DATA_G, TP_G_LEN, TP_API, TP_CASC_LEN
    )
    legal_set_phases = param_dict["enum"]

    if TP_API == com.API_STREAM:
        muls = getNumMuls(TT_DATA_F, TT_DATA_G, AIE_VARIANT)
        if TP_CASC_LEN == TP_G_LEN // muls:
            # Check 3 (stream only): TP_G_LEN must be a multiple of kMinGLen and >= kMinGLen.
            #   kMinGLen = TP_PHASES * kLanes * (kPoints / kStreamsPerCore)
            #   kStreamsPerCore = 1 (ONE_STREAM) if TP_G_LEN > (muls * TP_PHASES) >> 1, else 2 (TWO_STREAMS).
            # Note: This check is done directly here rather than via legal_set membership,
            # because fn_update_phases falls back to [1] when no phase is compatible,
            # which would allow invalid (TP_G_LEN, TP_PHASES) pairs to pass silently.
            kLanes = 4
            kPoints = muls // kLanes
            kStreamPerCoreVar = (muls * TP_PHASES) >> 1
            kStreamsPerCore = 1 if TP_G_LEN > kStreamPerCoreVar else 2
            kMinGLen = TP_PHASES * kLanes * (kPoints // kStreamsPerCore)
            if TP_G_LEN < kMinGLen or TP_G_LEN % kMinGLen != 0:
                return com.isError(
                    f"TP_PHASES={TP_PHASES} is not supported for TP_G_LEN={TP_G_LEN}: "
                    f"kMinGLen = TP_PHASES({TP_PHASES}) * kLanes({kLanes}) * (kPoints({kPoints}) / kStreamsPerCore({kStreamsPerCore})) = {kMinGLen}, "
                    f"but TP_G_LEN={TP_G_LEN} is not a multiple of kMinGLen. "
                    f"Adjust TP_PHASES to a value where TP_G_LEN is a valid multiple of kMinGLen."
                )

    return com.validate_legal_set(legal_set_phases, "TP_PHASES", TP_PHASES)


#######################################################
############### TP_SHIFT Updater and Validator ########
#######################################################
def update_TP_SHIFT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    return fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA_OUT)


def fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA):
    range_TP_SHIFT = com.fn_update_range_TP_SHIFT(AIE_VARIANT, TT_DATA)

    param_dict = {
        "name": "TP_SHIFT",
        "minimum": range_TP_SHIFT[0],
        "maximum": range_TP_SHIFT[1],
    }
    return param_dict


def validate_TP_SHIFT(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_SHIFT = args["TP_SHIFT"]
    return fn_validate_shift_val(AIE_VARIANT, TT_DATA_OUT, TP_SHIFT)


def fn_validate_shift_val(AIE_VARIANT, TT_DATA_OUT, TP_SHIFT):
    param_dict = fn_update_TP_SHIFT(AIE_VARIANT, TT_DATA_OUT)
    range_TP_SHIFT = [param_dict["minimum"], param_dict["maximum"]]
    return com.validate_range(range_TP_SHIFT, "TP_SHIFT", TP_SHIFT)


#######################################################
############# TP_RND Updater and Validator ############
#######################################################
def update_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    return fn_update_TP_RND(AIE_VARIANT)


def fn_update_TP_RND(AIE_VARIANT):
    legal_set_TP_RND = com.fn_get_legalSet_roundMode(AIE_VARIANT)
    param_dict = {"name": "TP_RND", "enum": legal_set_TP_RND}
    return param_dict


def validate_TP_RND(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_RND = args["TP_RND"]
    return com.fn_validate_roundMode(TP_RND, AIE_VARIANT)


#######################################################
############ TP_SAT Updater and Validator #############
#######################################################
def update_TP_SAT(args):
    legal_set_sat = com.fn_legal_set_sat()
    param_dict = {"name": "TP_SAT", "enum": legal_set_sat}
    return param_dict


def validate_TP_SAT(args):
    TP_SAT = args["TP_SAT"]
    return com.fn_validate_satMode(TP_SAT)


#############################################################
###### TP_USE_RTP_VECTOR_LENGTHS Updater and Validator ######
#############################################################
def update_TP_USE_RTP_VECTOR_LENGTHS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TP_F_LEN = args["TP_F_LEN"]
    TP_G_LEN = args["TP_G_LEN"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    return fn_update_TP_USE_RTP_VECTOR_LENGTHS(AIE_VARIANT, TP_API, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN)


def fn_update_TP_USE_RTP_VECTOR_LENGTHS(AIE_VARIANT, TP_API, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN):
    param_dict_flen = fn_update_f_len(TP_F_LEN, TT_DATA_G, TT_DATA_F, TT_DATA_OUT, TP_COMPUTE_MODE, TP_API, AIE_VARIANT)
    param_dict_glen = fn_update_g_len(TP_G_LEN, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_API, AIE_VARIANT)

    if (TP_API == 1) or (TP_API == 0 and TP_F_LEN == param_dict_flen["minimum"] and TP_G_LEN == param_dict_glen["minimum"]):
        legal_set_use_rtp_vec_lengths = [0]
    elif TP_API == 0 and (TP_F_LEN > param_dict_flen["minimum"] or TP_G_LEN > param_dict_glen["minimum"]):
        legal_set_use_rtp_vec_lengths = [0, 1]
    else:
        legal_set_use_rtp_vec_lengths = [0]

    param_dict = {
        "name": "TP_USE_RTP_VECTOR_LENGTHS",
        "enum": legal_set_use_rtp_vec_lengths,
    }
    return param_dict


def validate_TP_USE_RTP_VECTOR_LENGTHS(args):
    AIE_VARIANT = args["AIE_VARIANT"]
    TP_API = args["TP_API"]
    TP_USE_RTP_VECTOR_LENGTHS = args["TP_USE_RTP_VECTOR_LENGTHS"]
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"]
    TP_G_LEN = args["TP_G_LEN"]
    info_ports(args)
    return fn_validate_TP_USE_RTP_VECTOR_LENGTHS(AIE_VARIANT, TP_API, TP_USE_RTP_VECTOR_LENGTHS, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN)


def fn_validate_TP_USE_RTP_VECTOR_LENGTHS(AIE_VARIANT,TP_API, TP_USE_RTP_VECTOR_LENGTHS, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN):
    param_dict = fn_update_TP_USE_RTP_VECTOR_LENGTHS(AIE_VARIANT, TP_API, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN)
    return com.validate_legal_set(
        param_dict["enum"], "TP_USE_RTP_VECTOR_LENGTHS", TP_USE_RTP_VECTOR_LENGTHS
    )


#######################################################

# Example of updater.
#
# Updater are functions to help GUI to hint user on parameter setting with already given parameters.
# The return object will provide "value" which will be set in the wizard as the dependent parameter is being set.
# The rest of keys are similar to parameter definition, but with candidates of enum or range values refined
# based on previously set values.


# Utility Functions
#### Valid Sliding Mul Combos ####
def fn_get_valid_g_types_for_mul(TT_DATA_F, AIE_VARIANT, TP_API):
    if AIE_VARIANT == com.AIE:
        if TP_API == com.API_STREAM:
            if TT_DATA_F == "cint16":
                return ["int16", "cint16"]

        elif TP_API == com.API_BUFFER:
            if TT_DATA_F == "int16":
                return ["int16"]
            if TT_DATA_F == "int32":
                return ["int16"]
            if TT_DATA_F == "float":
                return ["float"]
            if TT_DATA_F == "cint16":
                return ["int16", "int32", "cint16"]
            if TT_DATA_F == "cint32":
                return ["int16", "cint16"]
            if TT_DATA_F == "cfloat":
                return ["float", "cfloat"]

    elif AIE_VARIANT == com.AIE_ML or AIE_VARIANT == com.AIE_MLv2:
        if TT_DATA_F == "int8":
            return ["int8"]
        if TT_DATA_F == "int16":
            return ["int16"]
        if TT_DATA_F == "int32":
            return ["int16"]
        if TT_DATA_F == "float":
            return ["float"]
        if TT_DATA_F == "bfloat16":
            return ["bfloat16"]
        if TT_DATA_F == "cint16":
            return ["int16", "int32", "cint16"]
        if TT_DATA_F == "cint32":
            return ["int16", "cint16"]
    return []

def fn_get_valid_out_types_for_mul(TT_DATA_F, TT_DATA_G):   # Don't feel too good with these functions but it's where we're at.
    if (TT_DATA_F == "int8")        and (TT_DATA_G == "int8"):      return ["int16"]
    if (TT_DATA_F == "int16")       and (TT_DATA_G == "int8"):      return ["int16"]
    if (TT_DATA_F == "int16")       and (TT_DATA_G == "int16"):     return ["int32"]
    if (TT_DATA_F == "int32")       and (TT_DATA_G == "int16"):     return ["int32"]
    if (TT_DATA_F == "cint16")      and (TT_DATA_G == "int16"):     return ["cint16", "cint32"]
    if (TT_DATA_F == "cint16")      and (TT_DATA_G == "int32"):     return ["cint32"]
    if (TT_DATA_F == "cint16")      and (TT_DATA_G == "cint16"):    return ["cint16", "cint32"]
    if (TT_DATA_F == "cint32")      and (TT_DATA_G == "int16"):     return ["cint32"]
    if (TT_DATA_F == "cint32")      and (TT_DATA_G == "cint16"):    return ["cint32"]
    if (TT_DATA_F == "float")       and (TT_DATA_G == "float"):     return ["float"]
    if (TT_DATA_F == "cfloat")      and (TT_DATA_G == "float"):     return ["cfloat"]
    if (TT_DATA_F == "cfloat")      and (TT_DATA_G == "cfloat"):    return ["cfloat"]
    if (TT_DATA_F == "bfloat16")    and (TT_DATA_G == "bfloat16"):  return ["float"]
    return []


def getNumLanes(TT_DATA_F, TT_DATA_G, AIE_VARIANT=1):
    if AIE_VARIANT == com.AIE:
        if (
            (TT_DATA_F == "int8" and TT_DATA_G == "int8")
            or (TT_DATA_F == "int16" and TT_DATA_G == "int8")
            or (TT_DATA_F == "int16" and TT_DATA_G == "int16")
        ):
            return 16
        elif (
            (TT_DATA_F == "int32" and TT_DATA_G == "int16")
            or (TT_DATA_F == "float" and TT_DATA_G == "float")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int16")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int32")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "cint16")
        ):
            return 8
        elif (
            (TT_DATA_F == "float" and TT_DATA_G == "cfloat")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "int16")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "cint16")
            or (TT_DATA_F == "cfloat" and TT_DATA_G == "float")
            or (TT_DATA_F == "cfloat" and TT_DATA_G == "cfloat")
        ):
            return 4
        else:
            return 0
    if AIE_VARIANT == com.AIE_ML:
        if TT_DATA_F == "int8" and TT_DATA_G == "int8":
            return 8
        elif (
            (TT_DATA_F == "int16" and TT_DATA_G == "int8")
            or (TT_DATA_F == "int16" and TT_DATA_G == "int16")
            or (TT_DATA_F == "int32" and TT_DATA_G == "int16")
            or (TT_DATA_F == "float" and TT_DATA_G == "float")
            or (TT_DATA_F == "bfloat16" and TT_DATA_G == "bfloat16")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int16")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int32")
        ):
            return 16
        elif (
            (TT_DATA_F == "cint16" and TT_DATA_G == "cint16")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "int16")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "cint16")
        ):
            return 8
        else:
            return 0
    if AIE_VARIANT == com.AIE_MLv2:
        if TT_DATA_F == "int8" and TT_DATA_G == "int8":
            return 8
        elif (
            (TT_DATA_F == "int16" and TT_DATA_G == "int8")
            or (TT_DATA_F == "int16" and TT_DATA_G == "int16")
            or (TT_DATA_F == "int32" and TT_DATA_G == "int16")
            or (TT_DATA_F == "float" and TT_DATA_G == "float")
            or (TT_DATA_F == "bfloat16" and TT_DATA_G == "bfloat16")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int16")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int32")
        ):
            return 16
        elif (
            (TT_DATA_F == "cint16" and TT_DATA_G == "cint16")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "int16")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "cint16")
        ):
            return 8
        else:
            return 0


def getNumMuls(TT_DATA_F, TT_DATA_G, AIE_VARIANT=1):
    if AIE_VARIANT == com.AIE:
        if TT_DATA_F == "int8" and TT_DATA_G == "int8":
            return 128
        elif TT_DATA_F == "int16" and TT_DATA_G == "int8":
            return 64
        elif TT_DATA_F == "int16" and TT_DATA_G == "int16":
            return 32
        elif (TT_DATA_F == "int32" and TT_DATA_G == "int16") or (
            TT_DATA_F == "cint16" and TT_DATA_G == "int16"
        ):
            return 16
        elif (
            (TT_DATA_F == "float" and TT_DATA_G == "float")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int32")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "cint16")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "int16")
        ):
            return 8
        elif(
            (TT_DATA_F == "cint32" and TT_DATA_G == "cint16") or
            (TT_DATA_F == "float" and TT_DATA_G == "cfloat") or
            (TT_DATA_F == "cfloat" and TT_DATA_G == "float") or
            (TT_DATA_F == "cfloat" and TT_DATA_G == "cfloat")
        ):
            return 4
        else:
            return 0
    if AIE_VARIANT == com.AIE_ML:
        if TT_DATA_F == "int8" and TT_DATA_G == "int8":
            return 256
        elif TT_DATA_F == "int16" and TT_DATA_G == "int8":
            return 128
        elif (
            (TT_DATA_F == "int16" and TT_DATA_G == "int16")
            or (TT_DATA_F == "int32" and TT_DATA_G == "int16")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int16")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int32")
        ):
            return 64
        elif (TT_DATA_F == "cint16" and TT_DATA_G == "cint16") or (
            TT_DATA_F == "cint32" and TT_DATA_G == "int16"
        ):
            return 32
        elif (TT_DATA_F == "float" and TT_DATA_G == "float") or (
            TT_DATA_F == "bfloat16" and TT_DATA_G == "bfloat16"
        ):
            return 16
        elif TT_DATA_F == "cint32" and TT_DATA_G == "cint16":
            return 8
        else:
            return 0
    if AIE_VARIANT == com.AIE_MLv2:
        if TT_DATA_F == "int8" and TT_DATA_G == "int8":
            return 512
        elif (
            (TT_DATA_F == "int16" and TT_DATA_G == "int16")
            or (TT_DATA_F == "int16" and TT_DATA_G == "cint16")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int16")
        ):
            return 128
        elif TT_DATA_F == "bfloat16" and TT_DATA_G == "bfloat16":
            return 64
        elif (
            (TT_DATA_F == "int16" and TT_DATA_G == "int32")
            or (TT_DATA_F == "int32" and TT_DATA_G == "int16")
            or (TT_DATA_F == "int32" and TT_DATA_G == "int32")
        ):
            return 32
        elif (
            (TT_DATA_F == "int16" and TT_DATA_G == "cint32")
            or (TT_DATA_F == "int32" and TT_DATA_G == "cint16")
            or (TT_DATA_F == "int32" and TT_DATA_G == "cint32")
            or (TT_DATA_F == "float" and TT_DATA_G == "float")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "int32")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "cint16")
            or (TT_DATA_F == "cint16" and TT_DATA_G == "cint32")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "int16")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "int32")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "cint16")
            or (TT_DATA_F == "cint32" and TT_DATA_G == "cint32")
        ):
            return 16
        elif (
            (TT_DATA_F == "float" and TT_DATA_G == "cfloat")
            or (TT_DATA_F == "cfloat" and TT_DATA_G == "float")
            or (TT_DATA_F == "cfloat" and TT_DATA_G == "cfloat")
        ):
            return 4
        else:
            return 0


def fn_compute_output_dimension(TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE):
    if TP_COMPUTE_MODE == COMPUTE_FULL:
        return TP_F_LEN + TP_G_LEN - 1
    elif TP_COMPUTE_MODE == COMPUTE_VALID:
        return TP_F_LEN - TP_G_LEN + 1
    elif TP_COMPUTE_MODE == COMPUTE_SAME:
        return TP_F_LEN
    else:
        return com.isError(
            f"ERROR: Invalid compute mode ({TP_COMPUTE_MODE}). Must be one of [{COMPUTE_FULL}, {COMPUTE_VALID}, {COMPUTE_SAME}]."
        )


def fn_get_input_paddedLength(
    TT_DATA_F, TT_DATA_G, TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE
):
    PaddedLength = 0
    lanes = getNumLanes(TT_DATA_F, TT_DATA_G)
    dataSamples = sampleSize[TT_DATA_F]
    
    # Float types need 512-bit (64-byte) alignment for padding, integer types use 256-bit (32-byte) alignment
    if TT_DATA_F in ["float", "cfloat", "bfloat16"]:
        dataLoad = (AIE_LOAD_SIZE_IN_BITS * 2) / dataSamples
    else:
        dataLoad = AIE_LOAD_SIZE_IN_BITS / dataSamples

    if TP_COMPUTE_MODE == COMPUTE_FULL:
        PaddedLength = (TP_G_LEN - 1) + TP_F_LEN + (TP_G_LEN - 1)
        PaddedLength = com.CEIL(PaddedLength, dataLoad)
        return PaddedLength
    elif TP_COMPUTE_MODE == COMPUTE_VALID:
        if lanes > dataLoad:
            PaddedLength = TP_F_LEN + (dataLoad << 1)
            PaddedLength = com.CEIL(PaddedLength, dataLoad)
            return PaddedLength
        else:
            PaddedLength = TP_F_LEN + dataLoad
            PaddedLength = com.CEIL(PaddedLength, dataLoad)
            return PaddedLength
    elif TP_COMPUTE_MODE == COMPUTE_SAME:
        PaddedLength = ((TP_G_LEN / 2) - 1) + TP_F_LEN + ((TP_G_LEN / 2) - 1)
        PaddedLength = com.CEIL(PaddedLength, dataLoad)
        return PaddedLength
    else:
        return com.isError(
            f"ERROR: Invalid compute mode ({TP_COMPUTE_MODE}) to compute the padded length. Must be one of [{COMPUTE_FULL}, {COMPUTE_VALID}, {COMPUTE_SAME}]."
        )


########################## Ports ###########################
def get_port_info(
    portname, dir, dataType, dim, numFrames, numPhases, apiType, vectorLength
):
    return [
        {
            "name": f"{portname}[{idx}]",
            "type": f"{apiType}",
            "direction": f"{dir}",
            "data_type": dataType,
            "fn_is_complex": com.fn_is_complex(dataType),
            "window_size": com.fn_input_window_size((dim * numFrames), dataType),
            "margin_size": 0,
            "phases": numPhases,
        }
        for idx in range(vectorLength)
    ]


def info_ports(args):
    """Standard function creating a static dictionary of information
    for upper software to correctly connect the IP.
    Some IP has a configurable number of ports according to parameter set,
    so port information has to be implemented as a function"""
    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_FUNCT_TYPE = args["TP_FUNCT_TYPE"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_API = args["TP_API"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_PHASES = args["TP_PHASES"]
    TP_USE_RTP_VECTOR_LENGTHS = args["TP_USE_RTP_VECTOR_LENGTHS"]
    AIE_VARIANT = args["AIE_VARIANT"]

    lanes = getNumLanes(TT_DATA_F, TT_DATA_G, AIE_VARIANT)
    param_dict_glen = fn_update_g_len(TP_G_LEN, TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_COMPUTE_MODE, TP_F_LEN, TP_API, AIE_VARIANT)
    # For VALID mode with RTP, use minimum G_LEN to compute the worst-case (maximum) output size,
    # since smaller G_LEN produces a larger output (F - G + 1). TP_G_LEN is kept as the
    # compile-time maximum for inG port sizing — runtime G_LEN is always <= TP_G_LEN.
    TP_G_LEN_for_output = param_dict_glen["minimum"] if (TP_USE_RTP_VECTOR_LENGTHS == 1 and TP_COMPUTE_MODE == COMPUTE_VALID) else TP_G_LEN
    loopcount = fn_compute_output_dimension(TP_F_LEN, TP_G_LEN_for_output, TP_COMPUTE_MODE)
    outDataLen = (((com.CEIL(loopcount, lanes)) / lanes)) * lanes

    if TP_API == com.API_BUFFER:
        portsInF = get_port_info(
            portname="inF",
            dir="in",
            dataType=TT_DATA_F,
            dim=TP_F_LEN,
            numFrames=TP_NUM_FRAMES,
            numPhases=TP_PHASES,
            apiType="window",
            vectorLength=TP_PHASES,
        )
        portsInG = get_port_info(
            portname="inG",
            dir="in",
            dataType=TT_DATA_G,
            dim=TP_G_LEN,
            numFrames=TP_NUM_FRAMES,
            numPhases=TP_PHASES,
            apiType="window",
            vectorLength=TP_PHASES,
        )
        portsOut = get_port_info(
            portname="out",
            dir="out",
            dataType=TT_DATA_OUT,
            dim=outDataLen,
            numFrames=TP_NUM_FRAMES,
            numPhases=TP_PHASES,
            apiType="window",
            vectorLength=TP_PHASES,
        )
        portsRtpVecLen = (
            com.get_parameter_port_info("rtpVecLen", "in", "int32", 1, 2, "async")
            if (args["TP_USE_RTP_VECTOR_LENGTHS"] == 1)
            else []
        )
    elif TP_API == com.API_STREAM:
        portsInF = get_port_info(
            portname="inF",
            dir="in",
            dataType=TT_DATA_F,
            dim=TP_F_LEN,
            numFrames=TP_NUM_FRAMES,
            numPhases=TP_PHASES,
            apiType="stream",
            vectorLength=TP_PHASES,
        )
        portsInG = get_port_info(
            portname="inG",
            dir="in",
            dataType=TT_DATA_G,
            dim=TP_G_LEN,
            numFrames=TP_NUM_FRAMES,
            numPhases=TP_PHASES_MIN,
            apiType="window",
            vectorLength=TP_PHASES_MIN,
        )
        portsOut = get_port_info(
            portname="out",
            dir="out",
            dataType=TT_DATA_OUT,
            dim=fn_compute_output_dimension(TP_F_LEN, TP_G_LEN, TP_COMPUTE_MODE),
            numFrames=TP_NUM_FRAMES,
            numPhases=TP_PHASES,
            apiType="stream",
            vectorLength=TP_PHASES,
        )
        portsRtpVecLen = (
            com.get_parameter_port_info("rtpVecLen", "in", "int32", 1, 2, "async")
            if (args["TP_USE_RTP_VECTOR_LENGTHS"] == 1)
            else []
        )
        pass

    return portsInF + portsInG + portsOut + portsRtpVecLen


#### graph generator ####
def generate_graph(graphname, args):

    if graphname == "":
        graphname = "default_graphname"

    TT_DATA_F = args["TT_DATA_F"]
    TT_DATA_G = args["TT_DATA_G"]
    TT_DATA_OUT = args["TT_DATA_OUT"]
    TP_FUNCT_TYPE = args["TP_FUNCT_TYPE"]
    TP_COMPUTE_MODE = args["TP_COMPUTE_MODE"]
    TP_F_LEN = args["TP_F_LEN"]
    TP_G_LEN = args["TP_G_LEN"]
    TP_SHIFT = args["TP_SHIFT"]
    TP_API = args["TP_API"]
    TP_RND = args["TP_RND"]
    TP_SAT = args["TP_SAT"]
    TP_NUM_FRAMES = args["TP_NUM_FRAMES"]
    TP_CASC_LEN = args["TP_CASC_LEN"]
    TP_PHASES = args["TP_PHASES"]
    TP_USE_RTP_VECTOR_LENGTHS = args["TP_USE_RTP_VECTOR_LENGTHS"]
    AIE_VARIANT = args["AIE_VARIANT"]

    rtp_declare_str = (
        f" std::array<adf::port<input>, 1> rtpVecLen;"
        if TP_USE_RTP_VECTOR_LENGTHS == 1
        else "//No RTP port"
    )
    rtp_connect_str = (
        f"adf::connect<> net_rtpVecLen(rtpVecLen[0], conv_corr_graph.rtpVecLen[0]);"
        if TP_USE_RTP_VECTOR_LENGTHS == 1
        else "//No RTP port"
    )

    # Use formatted multi-line string to avoid a lot of \n and \t
    code = f"""
class {graphname} : public adf::graph {{
public:
  // ports
  //template <typename dir>
  static constexpr unsigned int TP_PHASES = {TP_PHASES};
  static constexpr unsigned int TP_USE_RTP_VECTOR_LENGTHS = {TP_USE_RTP_VECTOR_LENGTHS};
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, TP_PHASES>;

  std::array<adf::port<input>, {TP_PHASES}> inF;
  std::array<adf::port<input>, 1> inG;
  std::array<adf::port<output>, {TP_PHASES}> out;
  {rtp_declare_str}

  xf::dsp::aie::conv_corr::conv_corr_graph<
    {TT_DATA_F}, // TT_DATA_F
    {TT_DATA_G}, // TT_DATA_G
    {TT_DATA_OUT}, // TT_DATA_OUT
    {TP_FUNCT_TYPE}, // TP_FUNCT_TYPE
    {TP_COMPUTE_MODE}, // TP_COMPUTE_MODE
    {TP_F_LEN}, // TP_F_LEN
    {TP_G_LEN}, //TP_G_LEN
    {TP_SHIFT}, //TP_SHIFT
    {TP_API}, //TP_API
    {TP_RND}, //TP_RND
    {TP_SAT}, // TP_SAT
    {TP_NUM_FRAMES}, // TP_NUM_FRAMES
    {TP_CASC_LEN}, //TP_CASC_LEN
    {TP_PHASES}, // TP_PHASES
    {TP_USE_RTP_VECTOR_LENGTHS} // TP_USE_RTP_VECTOR_LENGTHS
    > conv_corr_graph;

  {graphname}() : conv_corr_graph() {{
    adf::kernel *conv_corr_kernels = conv_corr_graph.getKernels();

    for (int phIndx = 0; phIndx < TP_PHASES; phIndx++) {{
        adf::connect<> net_inF(inF[phIndx], conv_corr_graph.inF[phIndx]);
        adf::connect<> net_out(conv_corr_graph.out[phIndx], out[phIndx]);
    }}
    adf::connect<> net_inG(inG[0], conv_corr_graph.inG);
    {rtp_connect_str}
  }}
}};
"""
    out = {}
    out["graph"] = code
    out["port_info"] = info_ports(args)
    out["headerfile"] = "conv_corr_graph.hpp"
    out["searchpaths"] = [
        "L2/include/aie",
        "L2/tests/aie/common/inc",
        "L1/include/aie",
        "L1/src/aie",
        "L1/tests/aie/inc",
        "L1/tests/aie/src",
    ]

    return out
