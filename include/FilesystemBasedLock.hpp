#pragma once
#include <fcntl.h> // for lock/unlock mutex file
#include <unistd.h>
#include <sys/file.h>
#include <map>
#include <string>
#include <exception>
#include <stdexcept>
#include "Logger.hpp"
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
    /**
     * @brief Scope based lock & unlock graph mutex file taking advantage of C++ RAII,
     * so the mutex will be unlocked even if the operation in between failed.
     */
#define GUARD_DATABASE(graph)                                      \
    struct RAII_GUARD                                              \
    {                                                              \
        RAII_GUARD(const fs::path &path, const std::string &graph) \
            : m_lock(path), m_graph(graph)                         \
        {                                                          \
            m_lock.lockDB(m_graph);                                \
            LOGI("Locked " << m_graph);                            \
        }                                                          \
        ~RAII_GUARD()                                              \
        {                                                          \
            m_lock.unlockDB(m_graph);                              \
            LOGI("Unlocked " << m_graph);                          \
        }                                                          \
        std::string m_graph;                                       \
        FilesystemBasedLock m_lock;                                \
    };                                                             \
    [[maybe_unused]] RAII_GUARD _ { this->m_db_path, graph }

    /**
     * @brief Filesystem based locking mechanism class used for file system based DB backends
     */
    class FilesystemBasedLock
    {
    protected:
        fs::path m_db_path = "modkom/component_db"; // can be modified at runtime
        std::map<std::string, int> m_mutex;

    public:
        FilesystemBasedLock(const fs::path &db_path);
        ~FilesystemBasedLock() = default;

        void makeDir(const fs::path &path);
        bool lockDB(const std::string &graph);
        void unlockDB(const std::string &graph);
        bool isFileLocked(const int fd);
    };
}
