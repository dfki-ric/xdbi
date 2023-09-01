#pragma once
#include "Backend.hpp"
#include "Logger.hpp"

#include <set>
#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <boost/filesystem.hpp>
    namespace fs = boost::filesystem;
#endif

namespace xdbi
{
    class FilesystemBasedBackend : public Backend
    {
    protected:
        fs::path m_db_path;
        std::string convertClassname(const std::string& classname);

    public:
        FilesystemBasedBackend(const fs::path &db_path);
        ~FilesystemBasedBackend() = default;

        void makeDir(const fs::path &path);
        fs::path createGraphPath(const std::string& graph);
        std::map<std::string, fs::path> getGraphs();
        fs::path createClassPath(const std::string& graph, const std::string& classname);
        // NOTE: This function returns the class paths which might not be the same as the externally defined classnames. (see convertClassname())
        std::map<std::string, fs::path> getClasses(const std::string &graph);
        std::string getFileName(const std::string& uri);
        fs::path createFilePath(const std::string& graph, const std::string& classname, const std::string& uri);
        std::map<std::string, fs::path> getFiles(const std::string &graph, const std::string &classname = "");
        bool removeFiles(const std::string &graph, const std::set<std::string>& uris);
        bool removeAllFiles(const std::string &graph);

        void setWorkingDbPath(const fs::path &db_path);
        fs::path getWorkingDbPath();
    };
}
