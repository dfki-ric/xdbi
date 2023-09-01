#pragma once
#include <nlohmann/json.hpp>

namespace nl = nlohmann;

namespace xdbi
{
    /**
     * @brief The base class of all Backends
     */
    class Backend
    {
    public:
        Backend() = default;
        ~Backend() = default;

    public: /* CRUD methods (mutual methods between most backends)*/
        /**
         * @brief Add Xtype instances to the database
         * @param xtypes: xtype models to add
         */
        virtual bool add(const nl::json &xtypes){ return false; };

        /**
         * @brief Update an instance or more of Xtypes in the database
         * @param xtypes : xtype models to update
         */
        virtual bool update(const nl::json &xtypes){ return false; };

        /**
         * @brief Finds instances of Xtype matching the arguments in the database
         * @param classname: The name of the classes to be matched
         * @param properties: A map of properties to match
         * @return  Empty json object if not found
         *
         */
        virtual nl::json find(const std::string &classname, const nl::json &properties) { return nl::json(); };

        /**
         * @brief Deletes an Xtype from database by id
         * @param id: Xtype unique identifier (e.g. URI)
         */
        virtual bool remove(const std::string &id) { return false; };

        /**
         * @brief Deletes all Xtypes from database
         */
        virtual bool clear(){ return false; };
    };
}
