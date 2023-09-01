def instantiate_db_interface_from_args(backend, path, url, graph):
    if backend == 'Client':
        if url is None:
            raise ArgumentError("Please specify the --url when using --backend Client")
        dbi = xdbi_py.Client()
        dbi.setDbAddress(args.url)
        if not dbi.isConnected():
            raise ArgumentError("No Server running at the given url!")
    elif backend == 'Serverless':
        if path is None:
            raise ArgumentError("Please specify the --path when using --backend Serverless")
        print(f"Path for Serverless {os.path.abspath(path)} ")
        dbi = xdbi_py.Serverless(os.path.abspath(path))
    if graph is not None:
        print(f"Connect to server  using graph {args.graph_name} via {args.backend}")
        dbi.setWorkingGraph(args.graph_name)
    return dbi
