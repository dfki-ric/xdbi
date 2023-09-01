#!/usr/bin/python3
import sys
import argparse
import xdbi_py
import os
import importlib


backends = xdbi_py.get_available_backends()


def main():
    parser = argparse.ArgumentParser(description='Wipes a graph in the graph database clean of vertices and edges')
    # xdbi specific args
    parser.add_argument('-b', '--db_backend', help="Database backend to be used", choices=backends, default=backends[0]) 
    parser.add_argument('-a', '--db_address', help="The url/local path to the db", required=True) 
    parser.add_argument('-g', '--db_graph', help="The name of the database graph to be used", required=True) 
    # tool specific args
    parser.add_argument('--types', help="The name of the type set that needs to be updated", default="xtypes")
    
    args = None
    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(1)

    try:
        types_module = importlib.import_module(args.types+"_py")
    except ImportError as e:
        raise e

    print(f"Setting up registry ...")
    registry = types_module.ProjectRegistry()
    print(f"Accessing database at {args.db_address}")
    dbi = xdbi_py.db_interface_from_config(registry, config={"type":args.db_backend, "address": args.db_address, "graph": args.db_graph}, read_only=False)

    print("Deleting all vertices")
    dbi.clear()


if __name__ == '__main__':
    main()

