#!/usr/bin/env python3
from flask import Flask, request

import sys
import os
import json
import argparse
import xdbi_py
import xtypes_py

import logging
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

config = {
    'backend':'jsondb',
    'server_url':'localhost:8183',
    'db_path':os.environ["XROCK_DBPATH"],
    'graph':'graph_test',
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='XDBI-Server allows REST access to the X-Rock database.')
    # parser.add_argument('--backend', help="Backend to be used [%(default)s]", choices=xdbi.factory.get_backends().keys(), default=config['backend'])
    parser.add_argument('-s', '--server_url', help="URL to the graph database server", default=config['server_url'])
    parser.add_argument('-g', '--graph', help="The name of the graph to import to", default=config['graph'])
    parser.add_argument('-p', '--db_path', help="The folder of the jsondb database", default=config['db_path'])
    args = None
    try:
        args = parser.parse_args()
        config["server_url"] = args.server_url
        config["graph"] = args.graph
        config["db_path"] = args.db_path
    except SystemExit:
        sys.exit(1)

dbb = xdbi_py.JsonDatabaseBackend(config["db_path"])
# add this function to serverless implementation
dbb.setWorkingGraph(config["graph"])

#log = logging.getLogger('werkzeug')
#log.setLevel(logging.ERROR)
app = Flask(__name__)

def load(req):
    global dbb
    classname = ""
    if not "uuid" in req:
        return {"status": "error", "message": "no uuid in load request"}
    if "classname" in req:
        classname = req["classname"]
        
    result = dbb.load(req["uuid"], classname)
    return {"status": "finished", "result": result}

def clear(req):
    global dbb
    # add try execpt handling
    dbb.clear()
    return {"status": "finished"}

def remove(req):
    global dbb

    if not "uuid" in req:
        return {"status": "error", "message": "no uuid in remove request"}
    dbb.remove(req["uuid"])
    return {"status": "finished"}

def add(req):
    global dbb

    if not "models" in req:
        return {"status": "error", "message": "no models given in add request"}
    for model in req["models"]:
        if not "uuid" in model:
            return {"status": "error", "message": "one or more models given has no uuid in add request"}
    dbb.add(req["models"])
    return {"status": "finished"}

def update(req):
    global dbb

    if not "models" in req:
        return {"status": "error", "message": "no models given in update request"}
    for model in req["models"]:
        if not "uuid" in model:
            return {"status": "error", "message": "one or more models given has no uuid in update request"}
    dbb.update(req["models"])
    return {"status": "finished"}

def find(req):
    global dbb

    if not "classname" in req:
        return {"status": "error", "message": "no classname given in find request"}
    if not "properties" in req:
        return {"status": "error", "message": "no properties given in find request"}
    result = dbb.find(req["classname"], req["properties"])
    return {"status": "finished", "result": result}

def ping(req):
    return {"status": "finished"}

@app.route('/', methods=['GET', 'POST'])
def db_request():
    global dbb

    response = {'response':{'id':0, 'results':[]}}
    content = {}
    if request.headers['Content-Type'] == 'application/json':
        content = json.loads(request.get_data(True, True))

    if "graph" in content:
        dbb.setWorkingGraph(content["graph"])
        pass
    else:
        dbb.setWorkingGraph(config["graph"])
        pass

    requestType = content["type"]    
    msg = globals()[requestType](content)
    return dbb.dumps(msg)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port="8183")
