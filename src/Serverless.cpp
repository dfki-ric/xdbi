#include <algorithm>
#include "Serverless.hpp"

using namespace xtypes;
using namespace xdbi;

xdbi::Serverless::Serverless(const XTypeRegistryPtr registry, const fs::path &db_path, const std::string graph)
    : DbInterface(registry), backend(new JsonDatabaseBackend(db_path, graph))
{
}

void xdbi::Serverless::setWorkingGraph(const std::string &graph)
{
    backend->setWorkingGraph(graph);
}

std::string xdbi::Serverless::getWorkingGraph()
{
    return backend->getWorkingGraph();
}

void xdbi::Serverless::setWorkingDbPath(const fs::path &db_path)
{
    backend->setWorkingDbPath(db_path);
}

fs::path xdbi::Serverless::getWorkingDbPath()
{
    return backend->getWorkingDbPath();
}

std::string xdbi::Serverless::getAbsoluteDbPath()
{
    return fs::canonical(fs::absolute(backend->getWorkingDbPath())).make_preferred().string();
}

bool xdbi::Serverless::isReady()
{
   return fs::exists(backend->getWorkingDbPath()) && backend->getWorkingGraph() != "";

}

XTypePtr xdbi::Serverless::load(const std::string &uri, const std::string &classname)
{
    this->checkReadiness();
    nl::json spec = this->backend->load(uri);

    return XType::import_from(spec, registry.lock());
}

bool xdbi::Serverless::clear()
{
    this->checkReadiness();
    return this->backend->clear();
}

bool xdbi::Serverless::remove(const std::string &uri)
{
    this->checkReadiness();
    this->checkWriteable();
    return this->backend->remove(uri);
}

bool xdbi::Serverless::add(std::vector<XTypePtr> xtypes, const int max_depth)
{
    this->checkReadiness();
    this->checkWriteable();
    nl::json models;
    for (auto xtype : xtypes)
    {
        std::map< std::string, nl::json> specs = xtype->export_to(max_depth);
        for (const auto &[_,spec] : specs)
        {
            models.push_back(spec);
        }
    }
    return this->add(models);
}

bool xdbi::Serverless::add(nl::json xtypes)
{
    this->checkReadiness();
    this->checkWriteable();
    return this->backend->add(xtypes);
}

bool xdbi::Serverless::update(std::vector<XTypePtr> xtypes, const int max_depth)
{
    this->checkReadiness();
    this->checkWriteable();
    nl::json models;
    for (const auto &xtype : xtypes)
    {
        std::map< std::string, nl::json> specs = xtype->export_to(max_depth);
        for (const auto &[_,spec] : specs)
        {
            models.push_back(spec);
        }
    }
    return this->update(models);
}

bool xdbi::Serverless::update(nl::json xtypes)
{
    this->checkReadiness();
    this->checkWriteable();
    return this->backend->update(xtypes);
}

std::vector<XTypePtr> xdbi::Serverless::find(const std::string &classname, const nl::json &properties)
{
    this->checkReadiness();
    nl::json models = this->backend->find(
        classname,
        properties);
    std::vector<XTypePtr> out;
    out.reserve(models.size());
    std::transform(models.begin(), models.end(), std::back_inserter(out), [&](nl::json &model)
                   { return this->load(model["uri"], model["classname"]); });
    return out;
}

std::set<std::string> xdbi::Serverless::uris(const std::string &classname, const nl::json &properties)
{
    this->checkReadiness();
    nl::json models = this->backend->find(
        classname,
        properties);
    std::set<std::string> results;
    std::transform(models.begin(), models.end(), std::inserter(results, results.begin()), [&](const nl::json &model)
                   { return model["uri"]; });
    return results;
}
