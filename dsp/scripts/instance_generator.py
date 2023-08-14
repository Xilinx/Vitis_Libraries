#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

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

import os
import sys
import json
import argparse
import importlib.util

def run_function(path, method, *args):
    sys.path.append(os.path.dirname(path))
    spec = importlib.util.spec_from_file_location(os.path.basename(path).rstrip(".py"), path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    fn = getattr(mod, method)
    return fn(*args)

def get_api_spec(spec_file, api_name):
    with open(spec_file, "r") as fd:
        d = json.load(fd)
    if d["schema"] == "vitis_libraries_api_list_schema-1.0":
        x = None
        for t in d["api_list"]:
            if t["api_name"] == api_name:
                x = t
                break
        if not x:
            print(f"ERROR: {api_name} not in {spec_file}")
            return None
        if "spec" in x:
            return x["spec"]
        elif "spec_file" in x:
            with open(os.path.join(os.path.dirname(spec_file), x["spec_file"]), "r") as fdx:
                dx = json.load(fdx)
                d = dx
        else:
            print(f"ERROR: ill formated spec file {spec_file}")
            return None
    if d["schema"] == "vitis_library_api_spec_schema-1.0":
        if d["api_name"] == api_name:
            return d
        else:
            print(f"ERROR: {spec_file} is not for {api_name}")
            return None

def get_params(param_file, param_name):
    with open(param_file, "r") as fd:
        d = json.load(fd)
        return d.get(param_name, {})

def write_file(out_file_name, context):
    with open(out_file_name, 'w') as f:
        if isinstance(context, dict):
            # XXX: existing art in DSP lib
            if "graph" in context:
                s = context["graph"]
            # standard?
            elif "source" in context:
                s = context["source"]
        elif isinstance(context, str):
            s = context
        if s:
            f.write(s)
        f.write('\n')

# ------------------------------------------------------------------------------

class inst_generator:
    def __init__(self, spec_file, api_name, param_file, param_name):
        self.spec_file = spec_file
        self.spec_data = get_api_spec(spec_file, api_name)
        self.params = get_params(param_file, param_name)

    def valid_all_params(self):
        #lifecycle_status
        if "lifecycle_status" in self.spec_data:
            lifecycle_status = self.spec_data["lifecycle_status"]
            if lifecycle_status in ["obsolete", "superseded"]:
                print("WARNING: API {} is {}!".format(self.spec_data["api_name"], lifecycle_status)) 
            else:
                print("INFO: API {} version: {}".format(self.spec_data["api_name"], lifecycle_status)) 

        for p in self.spec_data["parameters"]:
            if p["name"] not in self.params:
                print("Parameter {} is not defined in parameter file.".format(p["name"]))
                return False
            v = self.params[p["name"]]
            # type/enum/min/max checkings
            #if p["type"] not in ["int", "uint", "float", "double", "bool", "typename"]
            #    print("Valid alues are [typename, bool, int, double")
            #    return False
            if p["type"] == "typename":
                if "enum" in p and v not in p["enum"]:
                    print("{} is not valid enum for {}.".format(v, p["name"]))
                    return False
            elif p["type"] == "int" or p["type"] == "uint":
                if "minimum" in p and int(v) < int(p["minimum"]):
                    print("{!s} is less than minimum {} of {}.".format(v, p["minimum"], p["name"]))
                    return False
                if "maximum" in p and int(v) > int(p["maximum"]):
                    print("{!s} is greater than maximum {} of {}.".format(v, p["maximum"], p["name"]))
                    return False
                if "enum" in p and v not in [int(item) for item in p["enum"]]:
                    print("{} is not valid enum for {}.".format(v, p["name"]))
                    return False
            elif p["type"] == "double" or p["type"] == "float":
                if "minimum" in p and float(v) < float(p["minimum"]):
                    print("{!s} is less than minimum {} of {}.".format(v, p["minimum"], p["name"]))
                    return False
                if "maximum" in p and float(v) > float(p["maximum"]):
                    print("{!s} is greater than maximum {} of {}.".format(v, p["maximum"], p["name"]))
                    return False
                if "enum" in p and v not in [float(item) for item in p["enum"]]:
                    print("{} is not valid enum for {}.".format(v, p["name"]))
                    return False
            elif p["type"] == "bool":
                if v not in ["true", "false"]:
                    print("{} is not valid bool type for {}.".format(v, p["name"]))
            #validator 
            #standard output:
                #{"is_valid": true}
                #{"is_valid": false, "err_message": "TT_COEF must be the same as TT_DATA"}
            if "validator" in p:
                od = run_function(os.path.join(os.path.dirname(self.spec_file), p["validator"]["file"]), p["validator"]["function"], self.params)
                if not od["is_valid"]:
                    print(od["err_message"])
                    return False
        return True

    def gen_api_instance(self, out_file_name, instance_name):
        #instance_name = os.path.basename(os.path.splitext(out_file_name)[0])
        od = run_function(os.path.join(os.path.dirname(self.spec_file), self.spec_data["generator"]["file"]),
                          self.spec_data["generator"]["function"], instance_name, self.params)
        write_file(out_file_name, od)
        print(f"[INFO]: generated {out_file_name}")
        return True

def gen_tb_instance(out_file_name, script_name, func_name, param_file, param_name):
    params = get_params(param_file, param_name)
    od = run_function(script_name, func_name, params)
    write_file(out_file_name, od)
    return True

# ------------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", type=str, dest="out_file", help="output file name, e.g. foo.cpp.")
    parser.add_argument("--name", type=str, default="", help="name of the generated instance. This is optional.")
    parser.add_argument("--param-set", type=str, help="param set name,e.g. foo_1")
    parser.add_argument("--param-file", type=str, help="param file, e.g. config_params.json")
    parser.add_argument("--spec", type=str, default="", help="file which defines the API metadata, e.g. L2/meta/api.json")
    parser.add_argument("--api", type=str, default="", help="quanlified name of the API, e.g. xf::data_mover::mm2s")
    parser.add_argument("--file", type=str, default="", help="customzied generator python file name, e.g. my_test.py")
    parser.add_argument("--func", type=str, default="", help="python function name in file, e.g. gen_my_test_config")
    parser.add_argument("--valid-params", action='store_true', help="param validation, true for validation")

    args = parser.parse_args()
    #print("DEBUG: out_file:", args.out_file)
    #print("DEBUG: spec:", args.spec)
    #print("DEBUG: api:", args.param_file)
    #print("DEBUG: param_set:", args.param_set)
    #print("DEBUG: param_file:", args.param_file)
    #print("DEBUG: valid_params:", args.valid_params)
    res = True
    if args.valid_params:
        if args.api and args.param_file and args.param_set:
            gen = inst_generator(args.spec, args.api, args.param_file, args.param_set)
            res = gen.valid_all_params()
        else:
            print("ERROR: cannot validate without an API name and spec file/param set.")
            res = False
    elif args.out_file:
        if args.api:
            if args.param_file and args.param_set:
                gen = inst_generator(args.spec, args.api, args.param_file, args.param_set)
                res = gen.gen_api_instance(args.out_file, args.name)
            else:
                print("ERROR: cannot generate API without param file/param set.")
                res = False
        elif args.file and args.func:
            if args.param_file and args.param_set:
                res = gen_tb_instance(args.out_file, args.file, args.func, args.param_file, args.param_set)
            else:
                print("ERROR: cannot generate TB without param file/param set.")
                res = False
        else:
            print("ERROR: cannot generate API without api_name or TB generation file/func")
            res = False
    return res

if __name__ == '__main__':
    sys.exit(0 if main() else 1)
