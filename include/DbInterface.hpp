#pragma once
#include <memory>
#include <xtypes_generator/XTypeRegistry.hpp>
#include <xtypes_generator/XType.hpp>
#include <nlohmann/json.hpp>
using namespace xtypes;
namespace nl = nlohmann;

namespace xdbi
{
    // 20220609 HW: We should again try to introduce const correctness here
    /** This is the base class for all classes that connect to the database
    */
    class DbInterface
    {
    public:
        /// Constructor with registry
        DbInterface(const XTypeRegistryPtr registry);
        /// Constructor with registry and config
        DbInterface(const XTypeRegistryPtr registry, const nl::json config, const bool read_only);

        /*!
           \brief "Factory function. Instantiates a specific child of DbInterface from the passed config"
           \param "A registry instance to which top level XTypes will be registered"
           \param "The config from which the DbInterface is instantiated"
           \param "Whether this DbInterface shall be read_only"
           \return "The instance of DbInterface"
        */
        static std::shared_ptr<DbInterface> from_config(const XTypeRegistryPtr registry, const nl::json config, const bool read_only);
        /*!
         * \brief Returns the default config of this backend from the currently selected bundle
         * assuming the config file is located in "bundle/xdbi/config/db_config.json"
         * \return "Json Config"
         */
        static nl::json getDefaultConfig();

        /*!
         * \brief Returns a list of available backends which can be specified in the config for e.g. from_config()
         * \return "List of backends"
         */
        static std::vector<std::string> get_available_backends();

        /*!
           \brief "Sets the database graph/directory this DbInterface works on"
           \param "graph/directory name"
        */
        virtual void setWorkingGraph(const std::string &graph)=0;
        /*!
           \brief "Sets the database graph/directory this DbInterface works on"
           \return "graph/directory name"
        */
        virtual std::string getWorkingGraph()=0;

        /*!
            \brief "Checks whether the either the server is reachable or the directory for Serverless exists"
        */
        virtual bool isReady()=0;

        /*!
         * \brief Returns the absolute path where to find this Database
         * For client this will be the url, for Serverless the absolute local path, for MultiDB the path of the main db
         */
        virtual std::string getAbsoluteDbPath()=0;

        /*!
         * \brief Returns the absolute path where to find this Database including the graph
         * For client this will be the url, for Serverless the absolute local path, for MultiDB the path of the main db
         */
        virtual std::string getAbsoluteDbGraphPath();

        /*!
           \brief "Tries to load the XType specified via the passed uri. If classname is provided this limits the search area, and is therefore faster."
           \param "The uri of the XType to load"
           \param "(optional) The classname of the XType to load, if known"
           \return "The XType instance if found otherwise nullptr"
        */
        virtual XTypePtr load(const std::string &uri, const std::string &classname = "") = 0;
        /*!
           \brief "Deletes all entries in the current working graph/directory"
        */
        virtual bool clear() = 0;
        /*!
           \brief "Deletes the entry if it exists in the current working graph/directory"
           \param uri "The uri of the Xtype to be deleted"
           \return "True if it has been deleted, false otherwise"
        */
        virtual bool remove(const std::string &uri) = 0;
        /*!
           \brief "Adds the passed XType instance to the database in the current working graph"
           \param xtypes "A vector of Xtypes to be added to the database"
        */
        virtual bool add(std::vector<XTypePtr> xtypes, const int max_depth=-1) = 0;
        // 20220712 MS: Who is using the JSON variant?
        /*!
           \brief "Adds the passed XType instance to the database in the current working graph"
           \param xtypes "A vector of serialized Xtypes to be added to the database"
        */
        virtual bool add(nl::json xtypes) = 0;
        /*!
           \brief "Updates the passed json representation of a XType instance to the database in the current working graph"
           \param xtypes "A vector of Xtypes to be added to the database"
           \param depth_limit "A limit of the depth at which things get updated in the database"
        */
        virtual bool update(std::vector<XTypePtr> xtypes, const int max_depth=-1) = 0;
        // 20220712 MS: Who is using the JSON variant?
        /*!
           \brief "Updates the passed json representation of a XType instance to the database in the current working graph"
           \param xtypes "A vector of serialized Xtypes to be added to the database"
        */
        virtual bool update(nl::json xtypes) = 0;
        /*!
          \brief "Loads all xtypes that match the passed classname and properties and returns them as vector. If classname is provided this limits the search area, and is therefore faster."
          \param "The classname of the XType to load"
          \param "Certain properties the XType(s) to be retrieved have to have"
          \return "a vector of matching XType instances"
        */
        virtual std::vector<XTypePtr> find(const std::string &classname="", const nl::json &properties=nl::json{}) = 0;

        /*
         * \brief "Returns a list of all uris present in the database"
         * \param "The classname of the XType(s) to be considered"
          \param "Certain properties the XType(s) to be looked up have to have"
         * \return "A set of URI strings"
         */
        virtual std::set<std::string> uris(const std::string &classname="", const nl::json &properties=nl::json{}) = 0;
        
        /**
         * @brief Get the Config from which this Backend constructed from
         *
         * @return const nl::json&
         */
        virtual const nl::json &getConfig() const noexcept { return config; }

        /**
         * @brief Set the logger level: "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL", "OFF"
         * Sets the general log level for all interfaces
         */
        static void setLoggerLevel(std::string level, bool verbose=false);

    protected:
        std::weak_ptr<XTypeRegistry> registry;
        nl::json config;
        bool read_only = false;
        void checkReadiness();
        void checkWriteable();
    };
}
