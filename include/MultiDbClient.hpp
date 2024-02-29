#pragma once

#include "DbInterface.hpp"

using namespace xtypes;

namespace xdbi
{
    using DbInterfacePtr = std::shared_ptr<DbInterface>;

    /** The MultiDbClient manages multiple DbInterface to communicate with all of them.
     *  It holds one main interface to which can be written to (only) and many import interfaces which are read-only.
     *  >Note: If you want to read from the main_interface as well you have to put it in the list of import interfaces, too.
     *  Every dependent XType will also be saved to the main interface. That way the main interface alway holds the
     *  complete dataset.
     **/
    class MultiDbClient : public DbInterface
    {
    public:
        /// Constructor
        MultiDbClient(const XTypeRegistryPtr registry, const nl::json &config);

        void setWorkingGraph(const std::string &graph) override;
        std::string getWorkingGraph() override;
        void setImportServerWorkingGraph(const std::string &name, const std::string &graph);
        std::string getImportServerWorkingGraph(const std::string &name);
        std::string getAbsoluteDbPath() override;
        std::string getAbsoluteDbGraphPath() override;

        bool isReady() override;

        // First match semantics: Will return the first match in order of the import_interfaces list
        XTypePtr load(const std::string &uri, const std::string &classname = "") override;
        bool clear() override;
        bool remove(const std::string &uri) override;
        bool add(std::vector<XTypePtr> xtypes, const int max_depth=-1) override;
        bool add(nl::json xtypes) override;
        bool update(std::vector<XTypePtr> xtypes, const int max_depth=-1) override;
        bool update(nl::json xtypes) override;
        std::vector<XTypePtr> find(const std::string &classname="", const nl::json &properties=nl::json{}) override;
        std::set<std::string> uris(const std::string &classname="", const nl::json &properties=nl::json{}) override;

        // All matches semantics: Will return all matches in any of the import_interfaces list
        std::vector< std::pair< XTypePtr, DbInterfacePtr > > findAll(const std::string &classname="", const nl::json &properties=nl::json{});

        // Getters for interfaces
        const DbInterfacePtr getMainInterface();
        std::vector< DbInterfacePtr > getImportInterfaces();

    private:
        DbInterfacePtr main_interface;
        std::vector< DbInterfacePtr > import_interfaces;
        nl::json multi_config;

    };

}
