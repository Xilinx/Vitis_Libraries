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

import json
import importlib
import sys
import numpy as np   
import os

dsplib_ip_list=[
    "conv_corr",
    "dds_mixer",
    "dds_mixer_lut",
    "dft",
    "fft_ifft_dit_1ch",
    "fft_window",
    "fir_decimate_asym",
    "fir_decimate_hb",
    "fir_decimate_sym",
    "fir_interpolate_asym",
    "fir_interpolate_hb",
    "fir_resampler",
    "fir_sr_asym",
    "fir_sr_sym",
    "fir_tdm",
    "func_approx",
    "hadamard",
    "kronecker",
    "matrix_mult",
    "matrix_vector_mul",
    "mixed_radix",
    "outer_tensor",
    "sample_delay",
    "widget_api_cast",
    "widget_real2complex"
]


#Arguments
for i in range(len(sys.argv)):
    if sys.argv[i] == "--ip":
        IP_in_use = sys.argv[i+1]
    if sys.argv[i] == "--mdir":
        meta_dir = sys.argv[i+1]
    if sys.argv[i] == "--outdir":
        out_dir = sys.argv[i+1]

if "--h" in sys.argv: 
  help_msg=("\nConfig Helper Options:"+
  "\n--h [prints the helper message]"+
  "\n--ip ip_name [providing the config helper the IP to configure]"+
	"\n--mdir metadata_directory [by default config helper will guide you to xf_dsp\L2\meta]"+
	"\n--outdir output_directory [by default config helper will guide you to xf_dsp\L2\meta]"+
	"\nLIST_PARAMS [lists the parameters of the chosen IP to configure]"+
	"\nPRINT_GRAPH [prints the resulting graph at the end of the configuration]"+
	"\nNO_INSTANCE [no graph instance is to be generated at the end of the configuration]"+
	"\nLIST_IPS [prints the IP list in DSPLIB]"
  )
  print(help_msg)
  sys.exit()

if "LIST_PARAMS" in sys.argv: LIST_PARAMS = True
else: LIST_PARAMS = False

if "PRINT_GRAPH" in sys.argv: PRINT_GRAPH = True
else: PRINT_GRAPH = False

if "NO_INSTANCE" in sys.argv: NO_INSTANCE = True
else: NO_INSTANCE = False

if "LIST_IPS" in sys.argv: LIST_IPS = True
else: LIST_IPS = False


#add meta data and output directory defaults if they are not provided in arguments
script_directory = os.path.dirname(os.path.abspath(__file__))  

def print_IPs(LIST_IPS):
  if LIST_IPS:
    print("\nAvailable DSPLIB IPs:")
    sorted_list_ip=sorted(dsplib_ip_list)
    for dsplibip in sorted_list_ip:
      print(dsplibip)

#Functions and Classes
def map_power(num):
  superscript_mapping = str.maketrans("0123456789", "⁰¹²³⁴⁵⁶⁷⁸⁹")  
  formatted_num = str(num).translate(superscript_mapping)
  return formatted_num

def validName(string):  
    english_letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    allowed_chars = english_letters+"1234567890"+"_"
    if (string==""):
      print("Instance name cannot be empty.")
      return False
    if string[0] not in english_letters:
      print("Instance name cannot start with special characters/numbers.")
      return False
    for char in string:  
        if not(char in allowed_chars):
            print("Instance name can only contain letters/numbers/underscore symbol.")  
            return False  
    return True  

def fn_helper_IPname_in():
  IP_in_use = input('\nPlease enter the IP name you would like to configure: ')
  return(IP_in_use)

def fn_helper_metadir_in(script_directory):
  meta_dir=input(f"\nPlease provide the metadata directory. To accept the default {script_directory} return: ")
  if meta_dir=="":
    meta_dir=script_directory
  return(meta_dir)

def fn_helper_outdir_in(script_directory):
  out_dir=input(f"\nPlease provide the output directory. To accept the default {script_directory} return: ")
  if out_dir=="":
    out_dir=script_directory
  return(out_dir)

class ip_parameter:
  def __init__(self):
    self.name = ""
    self.valid = "False"
    self.default = '0'
    self.type = 'string'
    self.value = '0'
    self.length = 0
    self.actual = []
    self.helper_msg = ""
    self.element_type = ""

    self.updater =""
    self.validator =""
    self.idx_inc = 0

  def get_input(self,param_vals):
    self.idx_inc = 0
    while True:
      if self.type=="vector":
        print(f"\n{self.helper_msg}\n(Enter z/Z and press return for previous parameter)")
      else:
        print(f"\n{self.helper_msg}\n(Press return to accept default of {self.name} = {self.default})\n(Enter z/Z and press return for previous parameter)")
      
      inMsg=f"Please input for {self.name}: "
      user_input = input(inMsg)
      if (user_input=="z" or user_input=="Z"):
        self.idx_inc = 1
        break
      elif not(self.type == "string" or self.type == "typename" or self.type == "vector"):
        if ((user_input.isnumeric()) or (user_input== "")):
          self.value  = user_input or self.default
          self.value  = int(self.value)
          break          
        else:
          print("ERROR: Input should be numerical!")  
      elif self.type=="vector":
        if param_vals[self.element_type] in ["int16", "int32", "cint16", "cint32"]:
          self.value=np.zeros(self.length).astype(int)
        else:
          self.value=np.zeros(self.length)
        break
      else:
        self.value  = user_input or self.default
        break
      
  def param_update(self, param_vals):
    func_update = getattr(module, self.updater)
    update_return = func_update(param_vals)
    if "enum" in update_return:
      default_val=update_return["enum"][0]
      enum_list=update_return["enum"]
      helper_msg=f"{self.name} should be within the legal set of {enum_list}"
    elif "len" in update_return:
      self.length=update_return["len"]
      helper_msg=f"{self.name}: An all-zeros vector is constructed with a valid length of {self.length} for {self.name}! \nPlease edit the {self.name} array in the graph as required."
    else:
      default_val=update_return["minimum"]
      min_val=update_return["minimum"]
      if "maximum" in update_return:
        max_val=update_return["maximum"]
        if max_val == 2**31:
          max_val_formatted=map_power(31)
          helper_msg=f"{self.name} should be within the legal range of [{min_val}, 2{max_val_formatted}]"  
        else:  
          if min_val == max_val:
            helper_msg=f"{self.name} should be {min_val}."  
          else:
            helper_msg=f"{self.name} should be within the legal range of [{min_val}, {max_val}]"  

      else: 
        helper_msg=f"{self.name} should be at least {min_val}"

    if self.type != "vector":
      self.default = default_val
    self.helper_msg = helper_msg

    if "actual" in update_return:
      self.actual = update_return["actual"]

  def param_validate(self, param_vals):
    func_validate = getattr(module, self.validator)
    validate_return = func_validate(param_vals)
    self.valid = str(validate_return["is_valid"])
    if self.valid == "False":
      print(validate_return["err_message"])

##CONFIG HELPER ALGORITHM
#import the IP 
print("\nHello this is CONFIG HELPER!")

if ("IP_in_use" not in locals()):
  print_IPs(LIST_IPS)
  IP_in_use=fn_helper_IPname_in()

if ("meta_dir" not in locals()):
  meta_dir=fn_helper_metadir_in(script_directory)

if ("out_dir" not in locals()):
  out_dir=fn_helper_outdir_in(script_directory)


json_loc= f"{meta_dir}/{IP_in_use}.json"
while not(os.path.exists(json_loc)): 
  help_request=input(f"\nERROR: {IP_in_use} cannot be found in the metadata directory: {meta_dir}."+ 
        "\nWould you like to retry? (y/n) ")
  if help_request == "y" or  help_request == "":
    print_IPs(LIST_IPS)
    IP_in_use=fn_helper_IPname_in()
    meta_dir=fn_helper_metadir_in(script_directory)
    out_dir=fn_helper_outdir_in(script_directory)
    json_loc=json_loc= f"{meta_dir}/{IP_in_use}.json"
  if help_request == "n":
    print("\nExiting config_helper...")
    sys.exit()

module_name = IP_in_use
module = importlib.import_module(module_name)
generate_graph = getattr(module, "generate_graph")  

with open(json_loc) as f:
  json_load = json.load(f)
  params_json=json_load["parameters"]

  param_list = []
  validator_dict = {}
  updater_dict = {}
  type_dict = {}
  param_vals = {}
  vector_dict={}
  for pj in params_json: 
    if ("updater" in pj):
      param_list.append(pj["name"])
      updater_dict.update({pj["name"] : pj["updater"]["function"]})
      validator_dict.update({pj["name"] : pj["validator"]["function"]})
      type_dict.update({pj["name"] : pj["type"]})
    else:
      pj_name=pj["name"]
      raise Exception(f"Error! There is no updater for {pj_name}.")

    if "element_type" in pj:
      vector_dict.update({pj["name"] : pj["element_type"]})
    param_vals.update({pj["name"] : ""}) #construct the parameter library where all the necessary values will be written
print(f"\n{IP_in_use} IP found in the DSPLIB! Now, let me help you validate {IP_in_use} parameters.")

if(LIST_PARAMS):
  print("")
  print(f"\nHere are the parameters that will be configured for {IP_in_use} IP:")
  for n in param_list:
    print(n)

#Run the updaters to capture defaults
ip_parameter_objects_dict = {}
idx = 0
while idx<(len(param_list)):
  #Extract Params
  new_helper_param = "param_"+ param_list[idx]
  new_helper_param = ip_parameter() #generate parameter object
  ip_parameter_objects_dict.update({param_list[idx]: new_helper_param})  # dict of the parameter objects
  ip_parameter_objects_dict[param_list[idx]].name = param_list[idx] #assign names of the parameters
  ip_parameter_objects_dict[param_list[idx]].type = type_dict[param_list[idx]] #assign types
  ip_parameter_objects_dict[param_list[idx]].updater = updater_dict[param_list[idx]] #assign updaters
  ip_parameter_objects_dict[param_list[idx]].validator = validator_dict[param_list[idx]] #assign validators
  if ip_parameter_objects_dict[param_list[idx]].name in vector_dict:
    ip_parameter_objects_dict[param_list[idx]].element_type=vector_dict[ip_parameter_objects_dict[param_list[idx]].name]
  ip_parameter_objects_dict[param_list[idx]].param_update(param_vals) # update the parameters with the previous set of values
  while (ip_parameter_objects_dict[param_list[idx]].valid == "False"):
    ip_parameter_objects_dict[param_list[idx]].get_input(param_vals) # get input from the user
    if ip_parameter_objects_dict[param_list[idx]].idx_inc == 1:
      break
    param_vals[param_list[idx]] = ip_parameter_objects_dict[param_list[idx]].value
    ip_parameter_objects_dict[param_list[idx]].param_validate(param_vals) #validate the input from the user
    if ip_parameter_objects_dict[param_list[idx]].valid == "False":
      #Run the updaters to capture defaults
      ip_parameter_objects_dict[param_list[idx]].param_update(param_vals) 
      if (ip_parameter_objects_dict[param_list[idx]].actual != []):
        print(f"\nWARNING: The requested value for {ip_parameter_objects_dict[param_list[idx]].name} cannot be achieved."+
              f"\nThe closest legal value {ip_parameter_objects_dict[param_list[idx]].actual} is suggested by config helper.")

  if ip_parameter_objects_dict[param_list[idx]].idx_inc == 1:
    if idx !=0 : idx -= 1
  else: idx += 1

print(f"\nAll parameters of {IP_in_use} IP is validated!")
if not(NO_INSTANCE):
  #get the instance name
  print(f"\nNow, let me output a {IP_in_use} instance.")
  valid_instance_name=False 
  while(not valid_instance_name):
    instanceName = input('\nPlease enter a name for your instance: ')
    valid_instance_name=validName(instanceName)
 
  print(instanceName + " is accepted\n")
  instance_out=module.generate_graph(instanceName,param_vals)
  file = open(f"{out_dir}/graph_{IP_in_use}_{instanceName}.txt", "w")    
  file.write(instance_out["graph"])   
  file.close()
  print(f"\nPlease find the configured graph in {out_dir}/graph_{IP_in_use}_{instanceName}.txt")

  if (PRINT_GRAPH):
    print("\nHere is your configured} graph:")
    print(instance_out["graph"])
  