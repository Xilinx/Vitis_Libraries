#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

import argparse
import os
import sys

common_dir = os.path.dirname(os.path.abspath(__file__))

def convert_to_include(file_name, dry_run=False):
    with open(file_name, "r") as f:
        lines = f.readlines()
    newlines = []
    is_inc = False
    inc = ""
    for l in lines:
        if not is_inc:
            if l.startswith('# MK_INC_BEGIN'):
                inc = l.split("MK_INC_BEGIN")[1].strip()
                newlines.append("include $(MK_COMMON_DIR)/" + inc + "\n")
                is_inc = True
            else:
                newlines.append(l)
        else:
            if l.startswith("# MK_INC_END " + inc):
                is_inc = False
    if dry_run:
        for l in newlines:
             print(l, end="")
    else:
        with open(file_name, "w") as f:
            for l in newlines:
                f.write(l)

def convert_to_flat(file_name, dry_run=False, to_vitis=False):
    with open(file_name, "r") as f:
        newlines = f.readlines()
    lines = []
    while len(newlines) != len(lines):
        lines = newlines
        newlines = []
        for l in lines:
            if l.startswith("include $(MK_COMMON_DIR)/"):
                inc = l.strip().split("$(MK_COMMON_DIR)/")[1]
                if inc == "test_rules.mk":
                    if to_vitis:
                        inc = "vitis_test_rules.mk"
                    else:
                        inc = "sdx_test_rules.mk"
                if to_vitis:
                    if inc == "set_part.mk":
                        inc = "vitis_set_part.mk"
                    elif inc == "set_platform.mk":
                        inc = "vitis_set_platform.mk"
                    elif inc[:3] == "sdx":
                        inc = "vitis" + inc[3:]
                else:
                    if inc == "vitis_set_part.mk":
                        inc = "set_part.mk"
                    elif inc == "vitis_set_platform.mk":
                        inc = "set_platform.mk"
                    elif inc[:5] == "vitis":
                        inc = "sdx" + inc[5:]
                ilines = []
                with open(os.path.join(common_dir, inc), "r") as finc:
                    head_end = False
                    for il in finc.readlines():
                        if head_end:
                            ilines.append(il)
                        else:
                            if il.strip() == "# MK_BEGIN":
                                head_end = True
                newlines.append("# MK_INC_BEGIN " + inc + "\n")
                newlines.extend(ilines)
                newlines.append("# MK_INC_END " + inc + "\n")
            else:
                if to_vitis:
                    newlines.append(l.replace("XOCC", "VPP").replace("/bin/xocc", "/bin/v++"))
                else:
                    newlines.append(l.replace("VPP", "XOCC").replace("/bin/v++", "/bin/xocc"))
    if dry_run:
        for l in newlines:
             print(l, end="")
    else:
        with open(file_name, "w") as f:
            for l in newlines:
                f.write(l)

def update_shared(file_name, dry_run=False, template_file=None):
    if template_file is None:
        template_file = os.path.join(common_dir, "sdx_example.mk")
    if not os.path.isfile(template_file):
        print("ERROR: {} is not a file".format(template_file), file=sys.stderr)
        return
    with open(file_name, "r") as f:
        lines = f.readlines()
    userlines = []
    is_user_part = False
    for l in lines:
        if not is_user_part:
            if l.startswith("# BEGIN_XF_MK_USER_SECTION"):
                is_user_part = True
        else:
            if l.startswith("# END_XF_MK_USER_SECTION"):
                is_user_part = False
            else:
                userlines.append(l)
    if not userlines:
        print("ERROR: no user section in {}, will skip".format(file_name),
              file=sys.stderr)
        return
    newlines = []
    with open(template_file, "r") as f:
        for l in f:
            if not is_user_part:
                newlines.append(l)
                if l.startswith("# BEGIN_XF_MK_USER_SECTION"):
                    is_user_part = True
                    newlines.extend(userlines)
            else:
                if l.startswith("# END_XF_MK_USER_SECTION"):
                    newlines.append(l)
                    is_user_part = False
    if dry_run:
        for l in newlines:
             print(l, end="")
    else:
        with open(file_name, "w") as f:
            for l in newlines:
                f.write(l)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--template", help="File used to as template.", default=None)
    parser.add_argument("-n", "--dry_run", help="Dry run, just print to stdout",
                        action="store_true")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-i", "--include", help="Convert to include style",
                       action="store_true")
    group.add_argument("-f", "--flat", help="Convert to flat style",
                       action="store_true")
    group.add_argument("-u", "--update", help="Update by convert to include and then flat",
                       action="store_true")
    group.add_argument("-U", "--upgrade", help="Update and upgrade to Vitis",
                       action="store_true")
    parser.add_argument("file", help="File to be updated")
    args = parser.parse_args()

    if args.file is None or not os.path.isfile(args.file):
        parser.print_help()
        return 1

    if args.template:
        update_shared(args.file, args.dry_run, args.template)

    if args.include:
        convert_to_include(args.file, args.dry_run)
    elif args.flat:
        convert_to_flat(args.file, args.dry_run)
    elif args.update:
        convert_to_include(args.file, args.dry_run)
        convert_to_flat(args.file, args.dry_run)
    elif args.upgrade:
        convert_to_include(args.file, args.dry_run)
        convert_to_flat(args.file, args.dry_run, True)
    else:
        print("ERROR: unexpected error")

if __name__ == '__main__':
    main()
