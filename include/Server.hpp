#pragma once
#include <nlohmann/json.hpp>
#include "crow/crow_all.h"
#include "Logger.hpp"
#include "JsonDatabaseBackend.hpp"
#include <unordered_map>

namespace nl = nlohmann;

namespace xdbi {
    const static int DEFAULT_DB_PORT = 8183;
    const static std::string DEFAULT_DB_IP = "0.0.0.0";
    const static std::string DEFAULT_DB_ADDRESS = "http://"+DEFAULT_DB_IP+":"+std::to_string(DEFAULT_DB_PORT);

    /// Server class for HTTP/REST communication (see Client)
    class Server {
      public:
        const std::string dbAddress;
        const int dbPort;
        fs::path dbPath;

        /** \brief Constructor for an HTTP/REST Server
         * \param The path to the local database instance
         * \param The URL by which the server can be reached
         * \param The port for the communication
         */
        Server(const fs::path& dbPath, std::string dbAddress=DEFAULT_DB_IP, int dbPort=DEFAULT_DB_PORT);
        ~Server();
        /// Starts the server
        void start();
        /// Stops the server
        void stop();

      private:
        /// Callback for incoming requests
        crow::response db_request(const crow::request &req);
        /// Callback for incoming load requests (calls are delegated by db_request()
        crow::response load(const crow::request &req, const nl::json& dbrequest);
        /// Callback for incoming clear requests (calls are delegated by db_request()
        crow::response clear(const crow::request &req, const nl::json& dbrequest);
        /// Callback for incoming remove requests (calls are delegated by db_request()
        crow::response remove(const crow::request &req, const nl::json& dbrequest);
        /// Callback for incoming add requests (calls are delegated by db_request()
        crow::response add(const crow::request &req, const nl::json& dbrequest);
        /// Callback for incoming update requests (calls are delegated by db_request()
        crow::response update(const crow::request &req, const nl::json& dbrequest);
        /// Callback for incoming find requests (calls are delegated by db_request()
        crow::response find(const crow::request &req, const nl::json& dbrequest);
        /// Callback for incoming ping requests (calls are delegated by db_request()
        crow::response ping(const crow::request &req,const nl::json &dbRequest);

        std::unique_ptr<JsonDatabaseBackend> backend;
        std::unique_ptr<crow::SimpleApp> server;
        using handler_t = crow::response (Server::*)(const crow::request &, const nl::json &);
        std::unordered_map<std::string, handler_t> handlers;

    };

}
