#include "Server.hpp"
#include <iostream>
#include <sys/time.h>
namespace nl = nlohmann;
using namespace xdbi;
static constexpr const char banner[] =
    "      _                     _ _               \n"
    "     | |                   | | |              \n"
    "     | |___  ___  _ __   __| | |__            \n"
    " _   | / __|/ _ \\| '_ \\ / _` | '_ \\        \n"
    "| |__| \\__ \\ (_) | | | | (_| | |_) |        \n"
    " \\____/|___/\\___/|_| |_|\\__,_|_.__/  v2.0.0\n";

xdbi::Server::Server(const fs::path &dbPath, std::string dbAddress, int dbPort)
    : dbAddress(dbAddress), dbPort(dbPort), dbPath(dbPath),
      backend(new JsonDatabaseBackend(dbPath)), server(new crow::SimpleApp())
{
    handlers["load"] = &xdbi::Server::load;
    handlers["clear"] = &xdbi::Server::clear;
    handlers["remove"] = &xdbi::Server::remove;
    handlers["add"] = &xdbi::Server::add;
    handlers["update"] = &xdbi::Server::update;
    handlers["find"] = &xdbi::Server::find;
    handlers["ping"] = &xdbi::Server::ping;
}

xdbi::Server::~Server()
{
    stop();
}

void xdbi::Server::start()
{
    std::cout << banner << std::endl;
    LOGI("Server starting at " << dbAddress << ':' << dbPort << " ...");
    CROW_ROUTE((*server), "/")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)([&](const crow::request &req) -> crow::response
                                                                { return this->db_request(req); });

    LOGI("Server running at " << dbAddress << ':' << dbPort << " ...");
    server->bindaddr(this->dbAddress)
        .port(this->dbPort)
        .multithreaded()
        .run();
}

void xdbi::Server::stop()
{
    if (server)
        server->stop();
    LOGI("Server at " << dbAddress << ':' << dbPort << " stopped! ^__^");
}

crow::response xdbi::Server::ping(const crow::request &req, const nl::json &dbRequest)
{
    try
    {
        struct timeval time_now{};
        gettimeofday(&time_now, nullptr);
        const std::time_t current_time = (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
        const std::time_t client_request_time = dbRequest["time"].get<time_t>();
        const std::time_t time_diff = current_time - client_request_time;
        const nl::json response = {
            {"status", "finished"},
            {"result", time_diff}};
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        const nl::json response = {
            {"status", "error"},
            {"message", e.what()},
        };
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}
crow::response xdbi::Server::load(const crow::request &req, const nl::json &dbRequest)
{
    try
    {
        if (!dbRequest.contains("uri"))
            throw std::runtime_error("Could not find uri field in request");
        if (!dbRequest.contains("graph"))
            throw std::runtime_error("No graph specified");
        backend->setWorkingGraph(dbRequest["graph"]);
        const nl::json r = backend->load(dbRequest["uri"].get<std::string>(), dbRequest.contains("classname") ? dbRequest["classname"] : "");
        const nl::json response = {
            {"status", "finished"},
            {"result", r}};
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        const nl::json response = {
            {"status", "error"},
            {"message", e.what()},
        };
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}
crow::response xdbi::Server::clear(const crow::request &req, const nl::json &dbRequest)
{
    try
    {
        if (!dbRequest.contains("graph"))
            throw std::runtime_error("No graph specified");
        backend->setWorkingGraph(dbRequest["graph"]);
        backend->clear();
        const nl::json response = {
            {"status", "finished"}};
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        const nl::json response = {
            {"status", "error"},
            {"message", e.what()},
        };
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}
crow::response xdbi::Server::remove(const crow::request &req, const nl::json &dbRequest)
{
    try
    {
        if (!dbRequest.contains("uri"))
            throw std::runtime_error("Could not find uri field in request");
        if (!dbRequest.contains("graph"))
            throw std::runtime_error("No graph specified");
        backend->setWorkingGraph(dbRequest["graph"]);
        backend->remove(dbRequest["uri"].get<std::string>());
        const nl::json response = {
            {"status", "finished"}};
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        const nl::json response = {
            {"status", "error"},
            {"message", e.what()},
        };
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}
crow::response xdbi::Server::add(const crow::request &req, const nl::json &dbRequest)
{
    try
    {
        if (!dbRequest.contains("graph"))
            throw std::runtime_error("No graph specified");
        backend->setWorkingGraph(dbRequest["graph"]);
        if (!dbRequest.contains("models"))
            throw std::runtime_error("Could not find models field in request");
        const nl::json &models = dbRequest["models"];

        // Check if all models we are storing have a uri (side check to xrock-gui store)
        const auto it = std::find_if(models.begin(), models.end(), [&](const nl::json &model)
                                     { return !model.contains("uri"); });
        if (it != models.end())
            throw std::runtime_error("Missing uri in model " + (*it)["name"].get<std::string>());

        backend->add(dbRequest["models"]);
        const nl::json response = {
            {"status", "finished"},
        };
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        const nl::json response = {
            {"status", "error"},
            {"message", e.what()},
        };
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}
crow::response xdbi::Server::update(const crow::request &req, const nl::json &dbRequest)
{
    try
    {
        if (!dbRequest.contains("graph"))
            throw std::runtime_error("No graph specified");
        backend->setWorkingGraph(dbRequest["graph"]);
        if (!dbRequest.contains("models"))
            throw std::runtime_error("Could not find models field in request");
        const nl::json &models = dbRequest["models"];

        // Check if all models we are storing have a uri (side check to xrock-gui store)
        const auto it = std::find_if(models.begin(), models.end(), [&](const nl::json &model)
                                     { return !model.contains("uri"); });
        if (it != models.end())
            throw std::runtime_error("Missing uri in model " + (*it)["name"].get<std::string>());

        backend->update(dbRequest["models"]);
        const nl::json response = {
            {"status", "finished"}};
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        const nl::json response = {
            {"status", "error"},
            {"message", e.what()},
        };
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}
crow::response xdbi::Server::find(const crow::request &req, const nl::json &dbRequest)
{
    try
    {
        if (!dbRequest.contains("classname"))
            throw std::runtime_error("Could not find classname field in request");
        if (!dbRequest.contains("properties"))
            throw std::runtime_error("Could not find properties field in request");
        if (!dbRequest.contains("graph"))
            throw std::runtime_error("No graph specified");
        backend->setWorkingGraph(dbRequest["graph"]);

        const nl::json r = backend->find(dbRequest["classname"].get<std::string>(), dbRequest["properties"]);
        const nl::json response = {
            {"status", "finished"},
            {"result", r}};
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
    catch (const std::exception &e)
    {
        const nl::json response = {
            {"status", "error"},
            {"message", e.what()},
        };
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}

crow::response xdbi::Server::db_request(const crow::request &req)
{
    // Parse body to json, but first check if we received a content type as json
    try
    {
        nl::json dbRequest;
        if (req.get_header_value("Content-Type") == "application/json")
        {
            dbRequest = nl::json::parse(req.body);
        }
        else
        {
            throw std::runtime_error("Unsupported content-type header, server only accepts application/json requests at the moment");
        }

        if (!dbRequest.contains("graph"))
            throw std::runtime_error("No graph specified");
        backend->setWorkingGraph(dbRequest["graph"]);

        /// Handle db request types ///
        const std::string type = dbRequest["type"].get<std::string>();
        if (handlers.count(type) > 0)
        {
            return (this->*(handlers[type]))(req, dbRequest);
        }
        else
        {
            throw std::runtime_error("Unsupported request type '" + type + "'");
        }
    }
    catch (const std::exception &e)
    {
        const nl::json response = {
            {"status", "error"},
            {"message", e.what()},
        };
        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    }
}
