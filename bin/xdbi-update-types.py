#!/usr/bin/python3

import argparse
import os
import sys
import importlib
import xdbi_py


backends = xdbi_py.get_available_backends()


def main():
    parser = argparse.ArgumentParser(description='Expert tool to go through all xtypes and update added props in the type definition! ')
    # xdbi specific args
    parser.add_argument('-b', '--db_backend', help="Database backend to be used", choices=backends, default=backends[0])
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True) 
    parser.add_argument('-g', '--db_graph', help="The name of the database graph to be used", required=True) 
    # tool specific args
    parser.add_argument('--types', help="The name of the type set that needs to be updated", default="xtypes")
    parser.add_argument('--classname', help="If you want to limit this operation to one classname", default=None)
    args = None
    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(1)

    try:
        types_module = importlib.import_module(args.types+"_py")
    except ImportError as e:
        raise e

    working_dir = os.path.join(os.getenv("AUTOPROJ_CURRENT_ROOT"),args.db_address, args.db_graph)
    print(f"Operating on {working_dir} ...")

    print(f"Setting up registry ...")
    registry = types_module.ProjectRegistry()

    print(f"Accessing database at {args.db_address}")
    dbi = xdbi_py.db_interface_from_config(registry, config={"type":args.db_backend, "address": args.db_address, "graph": args.db_graph}, read_only=False)

    for path, dirs, files in os.walk(working_dir):
        classname = os.path.basename(path)
        if not classname.startswith(args.types+"--") or (args.classname is not None and not classname.endswith(args.classname.replace("::", "--"))):
            continue
        print("Updating", classname)
        classname = classname.replace("--", "::")
        type_obj = getattr(types_module, classname[len(args.types+"::"):])
        entities = dbi.find(classname=classname)
        updated_entities = []
        for old_entity in entities:
            for relation in type_obj().get_relations().keys():
                if not old_entity.has_facts(relation):
                    old_entity.set_unknown_fact_empty(relation)
        dbi.update(entities)
        print("Updated", classname)


if __name__ == '__main__':
    main()

