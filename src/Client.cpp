#include "Client.hpp"
#include <cpr/cpr.h>
#include <iostream>
#include <xtypes_generator/utils.hpp>

using namespace xtypes;
using namespace xdbi;
namespace nl = nlohmann;

xdbi::Client::Client(const XTypeRegistryPtr registry, const std::string &dbAddress, const std::string graph)
    : DbInterface(registry)
{
    if (graph != "")
        this->setWorkingGraph(graph);
    this->setDbAddress(dbAddress);
}

std::time_t xdbi::Client::ping()
  {
    struct timeval time_now{};
    gettimeofday(&time_now, nullptr);
    const std::time_t msecs_time = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);

      nl::json dbRequest;
      dbRequest["type"] = "ping";
    dbRequest["time"] = msecs_time;
    dbRequest["graph"] = getWorkingGraph();
      cpr::Response response = cpr::Post(cpr::Url(dbAddress + "/"),
                                         cpr::Body{{dbRequest.dump()}},
                                         cpr::Header{{"content-type", "application/json"}});
    if(response.status_code == 0) return -1;
    const nl::json r = nl::json::parse(response.text);
    if(r["status"].get<std::string>() == "finished")
        return r["result"].get<std::int64_t>();
    else
        return -1;

  }
  
bool xdbi::Client::isReady()
{
   return this->ping() != -1 && this->getWorkingGraph() != "";
}

void xdbi::Client::setDbAddress(const std::string& _dbAddress) {
    this->dbAddress = (_dbAddress.back() == '/' ? _dbAddress.substr(0, _dbAddress.size()-1) : _dbAddress);
    if (!this->isReady())
        std::cerr << "Couldn't connect to the server at " << this->dbAddress << ". Is it running and the graph name set?" << std::endl;
}

void xdbi::Client::setDbUser(const std::string &_dbUser)
{
    this->dbUser = _dbUser;
}

void xdbi::Client::setDbPassword(const std::string &_dbPassword)
{
    this->dbPassword = _dbPassword;
}

void xdbi::Client::setWorkingGraph(const std::string &graph)
{
    workingGraph = graph;
}

std::string xdbi::Client::getWorkingGraph()
{
    return workingGraph;
}

std::string xdbi::Client::getAbsoluteDbPath()
{
    std::string out = this->dbAddress;
    std::string protocol = "http://";
    if (out.rfind("https", 0) == 0) {
        protocol = "https://";
        out = out.substr(protocol.size(), out.size());
    } else if (out.rfind("http", 0) == 0) {
        protocol = "http://";
        out = out.substr(protocol.size(), out.size());
    }
    if (this->dbPassword != "" && this->dbUser != "")
        out = this->dbUser + ":" + this->dbPassword + "@" + out;

    return protocol + out;
}

XTypePtr xdbi::Client::load(const std::string &uri, const std::string &classname, const int search_depth)
{
    this->checkReadiness();
    ImportByURIFunc db_callback = [=](const std::string& _uri) -> nl::json
    {
        nl::json dbRequest;
        dbRequest["graph"] = getWorkingGraph();
        dbRequest["type"] = "load";
        dbRequest["uri"] = _uri;
        const auto r = cpr::Post(cpr::Url(dbAddress + "/"),
                           cpr::Body{{dbRequest.dump()}},
                           cpr::Header{{"content-type", "application/json"}});
        if (r.status_code == 0)
        {
            throw std::runtime_error("Client::load() db_callback: No response from server. Is it running?");
        }
        return xtypes::parseJson(r.text)["result"];
    };

    return XType::import_from(
        uri,
        db_callback,
        *this->registry.lock(),
        search_depth);
}

bool xdbi::Client::clear()
{
    this->checkReadiness();
    this->checkWriteable();
    nl::json dbRequest;
    dbRequest["graph"] = getWorkingGraph();
    dbRequest["type"] = "clear";

    cpr::Response r = cpr::Post(cpr::Url(dbAddress + "/"),
                                cpr::Body{{dbRequest.dump()}},
                                cpr::Header{{"content-type", "application/json"}});
    if (r.status_code == 0)
    {
        throw std::runtime_error("Client::clear(): No response from server. Is it running?");
    }
    const nl::json response = xtypes::parseJson(r.text);
    return response["status"].get<std::string>() == "finished";
}

bool xdbi::Client::remove(const std::string &uri)
{
    this->checkReadiness();
    this->checkWriteable();
    nl::json dbRequest;
    dbRequest["graph"] = getWorkingGraph();
    dbRequest["type"] = "remove";
    dbRequest["uri"] = uri;
    cpr::Response r = cpr::Post(cpr::Url(dbAddress + "/"),
                                cpr::Body{{dbRequest.dump()}},
                                cpr::Header{{"content-type", "application/json"}});
    if (r.status_code == 0)
    {
        throw std::runtime_error("Client::remove(): No response from server. Is it running?");
    }
    const nl::json response = xtypes::parseJson(r.text);
    return response["status"].get<std::string>() == "finished";
}

bool xdbi::Client::add(nl::json xtypes)
{
    this->checkReadiness();
    this->checkWriteable();
    nl::json dbRequest;
    dbRequest["graph"] = getWorkingGraph();
    dbRequest["type"] = "add";
    dbRequest["models"] = xtypes;
    cpr::Response r = cpr::Post(cpr::Url(dbAddress + "/"),
                                cpr::Body{{dbRequest.dump()}},
                                cpr::Header{{"content-type", "application/json"}});
    if (r.status_code == 0)
    {
        throw std::runtime_error("Client::add(): No response from server. Is it running?");
    }
    const nl::json response = xtypes::parseJson(r.text);
    return response["status"].get<std::string>() == "finished";
}

bool xdbi::Client::add(std::vector<XTypePtr> xtypes, const int depth_limit)
{
    this->checkReadiness();
    this->checkWriteable();
    nl::json models;
    for (auto xtype : xtypes)
    {
        URI2Spec spec = xtype->export_to(depth_limit,true);
        for (const auto &[_, model] : spec)
        {
            models.push_back(model);
        }
    }
    return this->add(models);
}

bool xdbi::Client::update(nl::json xtypes)
{
    this->checkReadiness();
    this->checkWriteable();
    assert(xtypes.is_array());
    nl::json dbRequest;
    dbRequest["graph"] = getWorkingGraph();
    dbRequest["type"] = "update";
    dbRequest["models"] = xtypes;
    cpr::Response r = cpr::Post(cpr::Url(dbAddress + "/"),
                                cpr::Body{{dbRequest.dump()}},
                                cpr::Header{{"content-type", "application/json"}});
    if (r.status_code == 0)
    {
        throw std::runtime_error("Client::update(): No response from server. Is it running?");
    }
    const nl::json response = xtypes::parseJson(r.text);
    return response["status"].get<std::string>() == "finished";
}

bool xdbi::Client::update(std::vector<XTypePtr> xtypes, const int depth_limit)
{
    this->checkReadiness();
    this->checkWriteable();
    nl::json models;
    for (const auto &xtype : xtypes)
    {
        URI2Spec spec = xtype->export_to(depth_limit, true);
        for (const auto &[_, model] : spec)
        {
            models.push_back(model);
        }
    }
    return this->update(models);
}

std::vector<XTypePtr> xdbi::Client::find(const std::string &classname, const nl::json &properties, const int search_depth)
{
    this->checkReadiness();
    nl::json dbRequest;
    dbRequest["graph"] = getWorkingGraph();
    dbRequest["type"] = "find";
    dbRequest["classname"] = classname;
    dbRequest["properties"] = properties;
    cpr::Response r = cpr::Post(cpr::Url(dbAddress + "/"),
                                cpr::Body{{dbRequest.dump()}},
                                cpr::Header{{"content-type", "application/json"}});
    if (r.status_code == 0)
    {
        throw std::runtime_error("Client::find(): No response from server. Is it running?");
    }
    const nl::json models = xtypes::parseJson(r.text)["result"];
    std::vector<XTypePtr> out;
    out.reserve(models.size());
    std::transform(models.begin(), models.end(), std::back_inserter(out), [&](const nl::json &model)
                   { return this->load(model["uri"], model["classname"], search_depth); });
    return out;
}

std::set<std::string> xdbi::Client::uris(const std::string &classname, const nl::json &properties)
{
    this->checkReadiness();
    nl::json dbRequest;
    dbRequest["graph"] = getWorkingGraph();
    dbRequest["type"] = "find";
    dbRequest["classname"] = classname;
    dbRequest["properties"] = properties;
    cpr::Response r = cpr::Post(cpr::Url(dbAddress + "/"),
                                cpr::Body{{dbRequest.dump()}},
                                cpr::Header{{"content-type", "application/json"}});
    if (r.status_code == 0)
    {
        throw std::runtime_error("Client::find(): No response from server. Is it running?");
    }
    const nl::json models = xtypes::parseJson(r.text)["result"];
    std::set<std::string> results;
    std::transform(models.begin(), models.end(), std::inserter(results, results.begin()), [&](const nl::json &model)
                   { return model["uri"]; });
    return results;
}
