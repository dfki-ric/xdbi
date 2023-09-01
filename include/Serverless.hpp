#pragma once

#include "DbInterface.hpp"
#include "JsonDatabaseBackend.hpp"

using namespace xtypes;

namespace xdbi
{
    /// Database Interface to access securely a local database instance without going via Server-Client
    class Serverless : public DbInterface
    {
    public:
        /// Constructor
        Serverless(const XTypeRegistryPtr registry, const fs::path &db_path, const std::string graph);

        void setWorkingGraph(const std::string &graph) override;
        std::string getWorkingGraph() override;

        void setWorkingDbPath(const fs::path &db_path);
        fs::path getWorkingDbPath();
        std::string getAbsoluteDbPath() override;

        bool isReady() override;

        XTypePtr load(const std::string &uri, const std::string &classname = "", const int search_depth=-1) override;
        bool clear() override;
        bool remove(const std::string &uri) override;
        bool add(std::vector<XTypePtr> xtypes, const int depth_limit=-1) override;
        bool add(nl::json xtypes) override;
        bool update(std::vector<XTypePtr> xtypes, const int depth_limit=-1) override;
        bool update(nl::json xtypes) override;
        std::vector<XTypePtr> find(const std::string &classname="", const nl::json &properties=nl::json{}, const int search_depth=-1) override;
        std::set<std::string> uris(const std::string &classname="", const nl::json &properties=nl::json{}) override;

    private:
        std::unique_ptr<JsonDatabaseBackend> backend;
    };

}
