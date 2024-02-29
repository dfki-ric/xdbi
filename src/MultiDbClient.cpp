#include "MultiDbClient.hpp"

#include <iostream>

#include "Serverless.hpp"
#include "Client.hpp"

using namespace xtypes;
using namespace xdbi;

xdbi::MultiDbClient::MultiDbClient(const XTypeRegistryPtr registry, const nl::json &config)
    : DbInterface(registry), multi_config(config)
{
    if (!multi_config.contains("import_servers") || !multi_config["import_servers"].is_array())
        throw std::runtime_error("Invalid config; no valid import_servers list.");

    for (auto &db_cfg : this->multi_config["import_servers"])
    {
        // FIXME: 2024-02-28 MS: This name is never used nor used to distinguish between import servers ... Can it be removed?
        if(!db_cfg.contains("name")) // we need a unique identifier (name) to distinguish between import_servers when setting/getting graph
            db_cfg["name"] = "import_server_" + std::to_string(import_interfaces.size()+1);
        import_interfaces.push_back(DbInterface::from_config(registry, db_cfg, true));
    }
    if (multi_config.contains("main_server") && !multi_config["main_server"].empty())
    {
        main_interface = DbInterface::from_config(registry, multi_config["main_server"], false);
    }
    // We have to create our own load function here, because the others before have overwritten ours
    auto auto_loader = [&](const std::string& _uri) -> XTypePtr {
        return load(_uri);
    };
    this->registry.lock()->set_load_func(auto_loader);
}

void xdbi::MultiDbClient::setWorkingGraph(const std::string &graph)
{
    main_interface->setWorkingGraph(graph);
}

std::string xdbi::MultiDbClient::getWorkingGraph()
{
    return main_interface->getWorkingGraph();
}

void xdbi::MultiDbClient::setImportServerWorkingGraph(const std::string &name,const std::string &graph)
{
    for (auto &interface : import_interfaces)
    {
        if (interface->getConfig()["name"] == name)
        {
            interface->setWorkingGraph(graph);
            return;
        }
    }
    throw std::runtime_error("MultiDbClient::setImportServerWorkingGraph: Could not find an import server with name \'"+name+'\'');
}

std::string xdbi::MultiDbClient::getAbsoluteDbPath()
{
    return main_interface->getAbsoluteDbPath();
}

std::string xdbi::MultiDbClient::getAbsoluteDbGraphPath()
{
    return main_interface->getAbsoluteDbGraphPath();
}

std::string xdbi::MultiDbClient::getImportServerWorkingGraph(const std::string &name)
{

     for (auto &interface : import_interfaces)
     {
        if (interface->getConfig()["name"] == name)
        {
          return interface->getWorkingGraph();
        }
     }
    throw std::runtime_error("MultiDbClient::getImportServerWorkingGraph: Could not find an import server with name \'"+name+'\'');

}

bool xdbi::MultiDbClient::isReady()
{
    if (!main_interface->isReady()) {
        std::cout<<"MultiDbClient::isReady: main interface is not ready"<<std::endl;
        return false;
    }
    for (auto &interface : import_interfaces)
    {
        if (!interface->isReady())
        {
            std::cout<<"MultiDbClient::isReady: import interface \'"+nl::to_string(interface->getConfig()["name"])+"\' is not ready"<<std::endl;
            return false;
        }
    }
    return true;
}


XTypePtr xdbi::MultiDbClient::load(const std::string &uri, const std::string &classname)
{
    XTypePtr found;
    // We search the import databases in the look-up order specified by the order of interfaces in import_servers
    for (auto &interface : import_interfaces)
    {
        found = interface->load(uri, classname);
        if (found)
            break;
    }
    return found;
}

bool xdbi::MultiDbClient::clear()
{
    return main_interface->clear();
}

bool xdbi::MultiDbClient::remove(const std::string &uri)
{
    return main_interface->remove(uri);
}

bool xdbi::MultiDbClient::add(std::vector<XTypePtr> xtypes, const int max_depth)
{
    return main_interface->add(xtypes, max_depth);
}

bool xdbi::MultiDbClient::add(nl::json xtypes)
{
    return main_interface->add(xtypes);
}

bool xdbi::MultiDbClient::update(std::vector<XTypePtr> xtypes, const int max_depth)
{
    return main_interface->update(xtypes, max_depth);
}

bool xdbi::MultiDbClient::update(nl::json xtypes)
{
    return main_interface->update(xtypes);
}

std::vector<XTypePtr> xdbi::MultiDbClient::find(const std::string &classname, const nl::json &properties)
{
    std::set<std::string> known;
    // The semantics of find are as follows:
    std::vector<XTypePtr> out;
    // We search the import databases in the look-up order specified by the order of interfaces in import_servers
    std::vector<XTypePtr> interface_out;
    for (auto &interface : import_interfaces)
    {
        std::vector<XTypePtr> _out = interface->find(classname, properties);
        for (auto& xtype : _out)
        {
            const std::string& _uri(xtype->uri());
            if (known.count(_uri))
                continue;
            // ... BUT we only insert UNKNOWN xtypes in the result
            out.push_back(xtype);
            known.insert(_uri);
        }
    }
    // In the end, we have all matching xtypes in the result but no duplicate Xtypes
    return out;
}

std::vector< std::pair< XTypePtr, DbInterfacePtr > > xdbi::MultiDbClient::findAll(const std::string &classname, const nl::json &properties)
{
    // This special function gathers all matching XType(s) across the database(s)
    // Therefore, we also have to return the source of the found XType(s)
    std::vector< std::pair< XTypePtr, DbInterfacePtr > > out;
    for (auto &interface : import_interfaces)
    {
        std::vector<XTypePtr> imported_out = interface->find(classname, properties);
        for (auto& xtype : imported_out)
        {
            out.push_back( { xtype, interface } );
        }
    }
    return out;
}

std::set<std::string> xdbi::MultiDbClient::uris(const std::string &classname, const nl::json &properties)
{
    // When we look for all uris, we use a set to simply merge them (no duplicates)
    std::set<std::string> results;
    for (auto &interface : import_interfaces)
    {
        std::set<std::string> _out = interface->uris(classname, properties);
        results.insert(_out.begin(), _out.end());
    }
    return results;
}

const DbInterfacePtr xdbi::MultiDbClient::getMainInterface()
{
    return main_interface;
}

std::vector< DbInterfacePtr > xdbi::MultiDbClient::getImportInterfaces()
{
    return import_interfaces;
}
