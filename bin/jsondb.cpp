#include <cxxopts/cxxopts.hpp>
#include <iostream>
#include <csignal>
#include "Server.hpp"
#include "Logger.hpp"
using namespace xdbi;

static std::unique_ptr<xdbi::Server> server;

static void shutdown_signal_handler(int sig)
{
    LOGI("Signal " << sig << " received.\nShutting down server...");
    if (server)
        server.reset(nullptr);
    Logger::shutdown();
    std::exit(sig == SIGINT ? EXIT_SUCCESS : EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    // Handle termination signals to exit server gracefully
    std::signal(SIGABRT, shutdown_signal_handler); // Abnormal termination. (std::abort() call)
    std::signal(SIGTERM, shutdown_signal_handler); // Termination request.
    std::signal(SIGINT, shutdown_signal_handler);  // Interactive attention signal. (CTRL+C)
    std::signal(SIGKILL, shutdown_signal_handler);  // Kill signal. (killall -9 e.g.)

    // Parse the args.
    cxxopts::Options options("jsondb", "Json Database Server");
    options.add_options()
        ("d,db_path", "Path of the database directory", cxxopts::value<std::string>(), " ")
        ("p,port", "Server port", cxxopts::value<int>()->default_value(std::to_string(DEFAULT_DB_PORT)), " ")
        ("h,help", "Print usage")
        ("l,log_level", "Set log level", cxxopts::value<std::string>()->default_value("TRACE")," ")
        ("f,log_file", "Logs output file", cxxopts::value<std::string>()," ")
        ("v,verbose", "Enable/disable logging");

    const cxxopts::ParseResult result = options.parse(argc, argv);
    std::string log_file;
    if (result.count("log_file"))
    {
        log_file = result["log_file"].as<std::string>();
    }
    Logger::initialize(result.count("verbose"), log_file);
    Logger::setLoggerLevel(result["log_level"].as<std::string>());

    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return EXIT_SUCCESS;
    }
    fs::path db_path = result["db_path"].as<std::string>();
    if (!db_path.is_absolute())
    {
        std::string env = std::getenv("AUTOPROJ_CURRENT_ROOT");
        db_path = env / db_path;
    }
    else
    {
        db_path = db_path;
    }
    int port = DEFAULT_DB_PORT;
    if (result.count("port"))
        port = result["port"].as<int>();

    // db_path must be a directory
    if (!fs::is_directory(db_path))
    {
        LOGE("Directory " << db_path.string() << " does not exist");
        return EXIT_FAILURE;
    }

    // Create & start server
    server = std::make_unique<xdbi::Server>(db_path, DEFAULT_DB_IP, port);
    server->start();
    return EXIT_SUCCESS;
}
