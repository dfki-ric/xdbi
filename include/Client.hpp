#pragma once

#include "DbInterface.hpp"
#include "Server.hpp"

using namespace xtypes;

namespace xdbi
{
    /// Used to establish HTTP/REST communication to a database server. (see Server Class)
    class Client : public DbInterface
    {
    public:
        Client(const XTypeRegistryPtr registry, const std::string& dbAddress, const std::string graph);

        /** \brief Checks whether the client can communicate with the Server
         * \return true if the client is connected */
        bool isReady();
        /** Pings the server and returns time it took for the server to respond in milliseconds
         * @return: -1 if the server is offline or something went wrong, otherwise milliseconds took for the server to respond
         */
        std::time_t ping();
        /// \brief "Sets the url where to reach the server. Default: http://localhost:8183"
        void setDbAddress(const std::string &_dbAddress);
        /// \brief "Sets the username if the server requires one. Default: '' "
        void setDbUser(const std::string &_dbUser);
        /// \brief "Sets the password if the server requires one. Default: '' "
        void setDbPassword(const std::string &_dbPassword);

        void setWorkingGraph(const std::string &graph) override;
        std::string getWorkingGraph() override;

        std::string getAbsoluteDbPath() override;

        XTypePtr load(const std::string &uri, const std::string &classname = "") override;
        bool clear() override;
        bool remove(const std::string &uri) override;
        bool add(std::vector<XTypePtr> xtypes, const int max_depth=-1) override;
        bool add(nl::json xtypes) override;
        bool update(std::vector<XTypePtr> xtypes, const int max_depth=-1) override;
        bool update(nl::json xtypes) override;
        std::vector<XTypePtr> find(const std::string &classname="", const nl::json &properties=nl::json{}) override;
        std::set<std::string> uris(const std::string &classname="", const nl::json &properties=nl::json{}) override;

    protected:
        std::string dbAddress = "http://localhost:8183";
        std::string dbUser = "";
        std::string dbPassword = "";
        std::string workingGraph = "";
    };

}
