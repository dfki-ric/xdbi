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
        if(!db_cfg.contains("name")) // we need a unique identifier (name) to distinguish between import_servers when setting/getting graph
            db_cfg["name"] = "import_server_" + std::to_string(import_interfaces.size()+1);
  

        bool isReadOnly = db_cfg.contains("readonly") ? db_cfg["readonly"].get<bool>() : true;
        auto interface = DbInterface::from_config(registry, db_cfg, isReadOnly);
        import_interfaces.push_back(interface);

        if (!isReadOnly && writable_interface == nullptr)
        {
            writable_interface = interface;
        }
    }
    if (multi_config.contains("main_server") && !multi_config["main_server"].empty())
    {
        bool isMainReadOnly = multi_config["main_server"].contains("readonly") ? multi_config["main_server"]["readonly"].get<bool>() : false;
        main_interface = DbInterface::from_config(registry, multi_config["main_server"], isMainReadOnly);

        if (!isMainReadOnly && writable_interface == nullptr)
        {
            writable_interface = main_interface;
        }
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
    // If the main interface is not listed under import_servers it is a write only interface.
    // REVIEW: with the following lines we could have the main server as fall back
    if (!found)// If not yet found we look (again) into the main database ...
        found = main_interface->load(uri, classname);
    return found;
}

bool xdbi::MultiDbClient::clear()
{
    return writable_interface && writable_interface->clear();
}

bool xdbi::MultiDbClient::remove(const std::string &uri)
{
    return writable_interface && writable_interface->remove(uri);
}

bool xdbi::MultiDbClient::add(std::vector<XTypePtr> xtypes, const int max_depth)
{
    return writable_interface && writable_interface->add(xtypes, max_depth);
}

bool xdbi::MultiDbClient::add(nl::json xtypes)
{
    return writable_interface && writable_interface->add(xtypes);
}

bool xdbi::MultiDbClient::update(std::vector<XTypePtr> xtypes, const int max_depth)
{
    return writable_interface && writable_interface->update(xtypes, max_depth);
}

bool xdbi::MultiDbClient::update(nl::json xtypes)
{
    return writable_interface && writable_interface->update(xtypes);
}

std::vector<XTypePtr> xdbi::MultiDbClient::find(const std::string &classname, const nl::json &properties)
{
    std::set<std::string> known;
    std::vector<XTypePtr> out;

    // Search in import interfaces first
    for (auto &interface : import_interfaces)
    {
        std::vector<XTypePtr> _out = interface->find(classname, properties);
        for (auto& xtype : _out)
        {
            const std::string &uri = xtype->uri();

            // Insert the URI into the set. If it's a new entry (not already present), add the xtype to the out .
            if (known.insert(uri).second) 
            {
                out.push_back(xtype);
            }
        }
    }

    // Fallback to main interface if it's not already included in import servers
    if (std::find(import_interfaces.begin(), import_interfaces.end(), main_interface) == import_interfaces.end())
    {
        std::vector<XTypePtr> interface_out = main_interface->find(classname, properties);
        for (auto &xtype : interface_out)
        {
            const std::string &uri = xtype->uri();
            if (known.insert(uri).second) //// Again, insert the URI. If it's new, add the xtype to the out.
            {
                out.push_back(xtype);
            }
        }
    }

    return out;
}

std::vector<std::pair<XTypePtr, DbInterfacePtr>> xdbi::MultiDbClient::findAll(const std::string &classname, const nl::json &properties)
{
    std::vector<std::pair<XTypePtr, DbInterfacePtr>> out;

    // Search in import interfaces first
    for (auto &interface : import_interfaces)
    {
        std::vector<XTypePtr> imported_out = interface->find(classname, properties);
        for (auto &xtype : imported_out)
        {
            out.push_back({xtype, interface});
        }
    }

    // Fallback to main interface if it's not already included in import servers
    if (std::find(import_interfaces.begin(), import_interfaces.end(), main_interface) == import_interfaces.end())
    {
        std::vector<XTypePtr> main_out = main_interface->find(classname, properties);
        for (auto &xtype : main_out)
        {
            out.push_back({xtype, main_interface});
        }
    }

    return out;
}

std::set<std::string> xdbi::MultiDbClient::uris(const std::string &classname, const nl::json &properties)
{
    // When we look for all uris, we use a set to simply merge them (no duplicates)
    std::set<std::string> results = main_interface->uris(classname, properties);
    for (auto &interface : import_interfaces)
    {
        std::set<std::string> _out = interface->uris(classname, properties);
        results.insert(_out.begin(), _out.end());
    }
    return results;
}

const DbInterfacePtr xdbi::MultiDbClient::fromWhichDb(const std::string &uri)
{
    if (main_interface->load(uri))
        return main_interface;
    for (auto &interface : import_interfaces)
    {
        if (interface->load(uri))
            return interface;
    }
    return nullptr;
}
