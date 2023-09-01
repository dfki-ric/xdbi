#pragma once

#include "FilesystemBasedBackend.hpp"

namespace xdbi
{
    /**
     * @brief JsonDatabaseBackend class
     */
    class JsonDatabaseBackend : public FilesystemBasedBackend
    {
    public:
        JsonDatabaseBackend(const fs::path &db_path, const std::string graph="");
        ~JsonDatabaseBackend() = default;

        bool isReady();

        bool add(const nl::json &models) override;
        bool update(const nl::json &models) override;
        nl::json find(const std::string &classname, const nl::json &properties) override;
        bool remove(const std::string &uri) override;
        bool clear() override;
        nl::json load(const std::string &uri, const std::string &classname = "");
        nl::json findEdgesFrom(const std::vector<std::string> &uris);
        nl::json findEdgesTo(const std::vector<std::string> &uris);
        void removeEdgesTo(const std::vector<std::string> &uris);

        void setWorkingGraph(const std::string &graph);
        std::string getWorkingGraph();
        std::string dumps(const nl::json& dict);
    private:
        nl::json getXtypesByURI(const std::string &graph, const std::string &uri, const std::string &classname="");
        nl::json getXtypes(const std::string &graph, const std::string &classname="");
        nl::json loadAndCheck(const std::string &fname, const fs::path &fpath, const std::string &classname);
        nl::json _load(const std::string &uri, const std::string &classname = "");
        nl::json _find(const std::string &classname, const nl::json &properties);
        nl::json _findEdgesFrom(const std::vector<std::string> &uris);
        nl::json _findEdgesTo(const std::vector<std::string> &uris);
        void _removeEdgesTo(const std::vector<std::string> &uris);
        bool _remove(const std::string &uri);
        bool _update(const nl::json &models);
        bool _store(const nl::json &xtype);
        bool _add(const nl::json &models);
        bool _clear();
    private:
        std::string m_graph = ""; /**< Current working graph */
    };
}
