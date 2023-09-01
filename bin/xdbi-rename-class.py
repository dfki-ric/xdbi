#!/usr/bin/python3
import sys
import os
import argparse
import json


def main():
    parser = argparse.ArgumentParser(description='Expert tool to rename Xtype classnames in an existing database! Use with care!')
    # xdbi specific args
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True) 
    parser.add_argument('-g', '--db_graph', help="The name of the database graph to be used", required=True) 
    # tool specific args
    parser.add_argument('original_classname', help="The original classname")
    parser.add_argument('new_classname', help="The new classname")
    args = None
    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(1)

    working_dir = os.path.join(os.getenv("AUTOPROJ_CURRENT_ROOT"), args.db_address, args.db_graph)
    print(f"Operating on {working_dir} ...")

    new_class_dir_name = args.new_classname.replace(":", "-")
    new_class_dir = os.path.join(working_dir, new_class_dir_name)
    if os.path.isdir(new_class_dir):
        print(f"{new_class_dir} already exists")
        sys.exit(2)

    original_class_dir_name = args.original_classname.replace(":", "-")
    original_class_dir = os.path.join(working_dir, original_class_dir_name)
    if not os.path.isdir(original_class_dir):
        print(f"{original_class_dir} does not exist")
        sys.exit(3)

    # First, go through every file in the directory and update the classname key
    print(f"Modifiing classname keys in files of {original_class_dir} ...")
    all_files = [f for f in os.listdir(original_class_dir) if os.path.isfile(os.path.join(original_class_dir, f))]
    for fname in all_files:
        fp = os.path.join(original_class_dir, fname)
        j = None
        with open(fp, "r") as f:
            j = json.load(f)
        if not j:
            print(f"WARNING: Could not open {fp}")
            continue
        if "classname" not in j:
            print(f"WARNING: No classname in {fp}")
            continue
        if j["classname"] != args.original_classname:
            print(f"WARNING: {fp} seems to be in the wrong folder. {j['classname']} != {args.original_classname}")
            continue
        j["classname"] = args.new_classname
        with open(fp, "w") as f2:
            json.dump(j, f2, indent=4)

    # Finally, move the old dir to the new dir
    print(f"Renaming {original_class_dir} to {new_class_dir} ...")
    os.rename(original_class_dir, new_class_dir)


if __name__ == '__main__':
    main()
