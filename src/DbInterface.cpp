#include "DbInterface.hpp"
#include "Logger.hpp"
#include "Serverless.hpp"
#include "Client.hpp"
#include "MultiDbClient.hpp"

#include <xtypes_generator/utils.hpp>

using namespace xtypes;
using namespace xdbi;

void xdbi::DbInterface::setLoggerLevel(std::string level, bool verbose)
{
    Logger::setLoggerLevel(level);
    Logger::setVerbose(verbose);
}

void xdbi::DbInterface::checkReadiness()
{
    if (!this->isReady()) {
        throw std::runtime_error("ERROR: Tried to use a DbInterface that is not ready");
    }
}

void xdbi::DbInterface::checkWriteable()
{
    if (this->read_only) {
        throw std::runtime_error("ERROR: Tried to write to a read_only DbInterface");
    }
}

std::vector<std::string> xdbi::DbInterface::get_available_backends()
{
    std::vector<std::string> results;
    results.push_back("Serverless");
    results.push_back("Client");
    results.push_back("MultiDbClient");
    return results;
}

std::shared_ptr<DbInterface> xdbi::DbInterface::from_config(const XTypeRegistryPtr registry, const nl::json config, const bool read_only)
{
    std::shared_ptr<DbInterface> out;
    std::string address;
    // backwards compatibility
    if (!config.contains("address") && config.contains("path") && config["type"] == "Serverless")
    {
        address = config["path"].get<std::string>();
    }
    else if (!config.contains("address") && config.contains("url") && config["type"] == "Client")
    {
        address = config["url"].get<std::string>();
    }
    else if (!config.contains("address") && config.contains("config") && config["type"] == "MultiDbClient")
    {
        address = "";
    }
    else if (config.contains("address"))
    {
        address = config["address"].get<std::string>();
    }
    else
        throw std::runtime_error("Invalid config 'address' of db not given!");

    // resolve path except for MultiDbClient without address
    if (config["type"] != "MultiDbClient" || (config["type"] == "MultiDbClient" && config.contains("address")))
    {
        char *xdbi_base_path = getenv("XDBI_BASE_PATH");
        char *autoproj_current_root = getenv("AUTOPROJ_CURRENT_ROOT");
        if (fs::path(address).is_relative() && !fs::exists(address) && xdbi_base_path && fs::exists(fs::path(std::string(xdbi_base_path)) / address))
        {
            address = fs::path(std::string(xdbi_base_path)) / address;
        }
        else if (fs::path(address).is_relative() && !fs::exists(address) && autoproj_current_root && fs::exists(fs::path(std::string(autoproj_current_root)) / address))
        {
            address = fs::path(std::string(autoproj_current_root)) / address;
        }
        else if (config["type"] != "Client" && !fs::exists(address))
        {
            throw std::runtime_error("Couldn't find local db at '" + address + "'. Does it exist?");
        }
    }

    std::string graph = "";
    if (config.contains("graph"))
        graph = config["graph"].get<std::string>();

    if (config["type"] == "Serverless")
    {
        out = std::make_shared<Serverless>(registry, address, graph);
        out->read_only = read_only;
    }
    else if (config["type"] == "Client")
    {
        out = std::make_shared<Client>(registry, address, graph);
        out->read_only = read_only;
    }
    else if (config["type"] == "MultiDbClient")
    {
        auto json_config = config["config"];
        if (config.contains("address"))
        {
            fs::path path = config["address"].get<std::string>();
            json_config = nl::json::parse(std::ifstream{path});
        }
        out = std::make_shared<MultiDbClient>(registry, json_config);
        out->read_only = true; // true because we only want to have one db where we write to, this here is only used for reading
    }
    else
    {
        std::cerr << config.dump() << std::endl;
        throw std::runtime_error("Invalid DbInterface configuration");
    }

    out->config = config;
    return out;
}

nl::json xdbi::DbInterface::getDefaultConfig()
{
    char *envs = getenv("XDBI_CONFIG_PATH");
    if (envs)
    {
        fs::path path = std::string(envs);

        if (fs::exists(path))
        {
            std::cout << "Found db default config: " << path << std::endl;
            return nl::json::parse(std::ifstream{path});
        }
        else if (path.is_relative())
        {
            fs::path base_path = std::string(getenv("XDBI_BASE_PATH"));
            if (fs::exists(base_path / path))
            {
                std::cout << "Found db default config: " << path << std::endl;
                return nl::json::parse(std::ifstream{base_path / path});
            }
        }
        else
        {
            std::cerr << "No db default config found by XDBI_CONFIG_PATH and XDBI_BASE_PATH" << std::endl;
        }
    }

    // will deprecate as soon as XDBI_CONFIG_PATH and XDBI_BASE_PATH are set by rock-select-bundle
    envs = getenv("ROCK_BUNDLE");
    if (envs)
    {
        // check for config file in bundle folder
        std::string bundleName = envs;
        envs = getenv("AUTOPROJ_CURRENT_ROOT");
        if (envs)
        {
            fs::path bundlePath = std::string(envs) + "/bundles";
            // we continue if path and name is available
            fs::path path = bundlePath / bundleName / "config" / "xdbi" / "db_config.json";
            if (fs::exists(path))
            {
                std::cout << "Found db default config: " << path << std::endl;
                return nl::json::parse(std::ifstream{path});
            }
            else
            {
                std::cerr << "No db default config found in bundle " << bundleName << ", missing file: " << path << std::endl;
            }
        }
        else
        {
            std::cerr << "No bundle path set in env: AUTOPROJ_CURRENT_ROOT" << std::endl;
        }
    }
    return nl::json();
}

std::string xdbi::DbInterface::getAbsoluteDbGraphPath()
{
    return getAbsoluteDbPath() + "/" + getWorkingGraph();
}