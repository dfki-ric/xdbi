#include "FilesystemBasedBackend.hpp"
#include <fstream>
#include <xtypes_generator/utils.hpp>
#include <algorithm>

namespace xdbi
{

    FilesystemBasedBackend::FilesystemBasedBackend(const fs::path &db_path)
        : m_db_path(db_path)
    {
    }

    void FilesystemBasedBackend::makeDir(const fs::path &path)
    {
        if (fs::exists(path))
        {
            LOGI("Directory " << path << " already exists");
            return;
        }
        if (!fs::create_directory(path))
            LOGE("failed to create directory at " << path);
    }

    fs::path FilesystemBasedBackend::createGraphPath(const std::string& graph)
    {
        fs::path p = m_db_path / graph;
        makeDir(p);
        return p;
    }

    std::map<std::string, fs::path> FilesystemBasedBackend::getGraphs()
    {
        std::map<std::string, fs::path> graphs;
        for (const auto &entry : fs::directory_iterator(m_db_path))
        {
            const fs::path path = m_db_path / entry.path();
            if (!fs::is_directory(path))
                continue;
            graphs[entry.path().filename().string()] = path;
        }
        return graphs;
    }

    std::string FilesystemBasedBackend::convertClassname(const std::string& classname)
    {
        // Convert any : to -
        std::string result(classname);
        std::replace(result.begin(), result.end(), ':', '-');
        return result;
    }

    fs::path FilesystemBasedBackend::createClassPath(const std::string& graph, const std::string& classname)
    {
        // 2022-12-22 MS: Before creating the classpath, we have to convert any : to -
        fs::path p = createGraphPath(graph) / convertClassname(classname);
        makeDir(p);
        return p;
    }

    // NOTE: It can happen, that someone externally uses this function! Then the class_path would contain - instead of : !
    // Make sure this does not happen
    std::map<std::string, fs::path> FilesystemBasedBackend::getClasses(const std::string &graph)
    {
        if (graph.empty())
            throw std::invalid_argument("FilesystemBasedBackend::getClasses(): graph is empty");

        std::map<std::string, fs::path> classes;
        const fs::path graph_path = m_db_path / fs::path(graph);
        if (!fs::is_directory(graph_path))
            return classes;
        for (const auto &entry : fs::directory_iterator(graph_path))
        {
            const fs::path &class_path = entry.path();
            if (!fs::is_directory(class_path))
                continue;
            classes[entry.path().filename().string()] = class_path;
        }
        return classes;
    }

    std::string FilesystemBasedBackend::getFileName(const std::string& uri)
    {
        const size_t uuid = xtypes::uri_to_uuid(uri);
        const std::string filename = std::to_string(uuid);
        return filename;
    }

    fs::path FilesystemBasedBackend::createFilePath(const std::string& graph, const std::string& classname, const std::string& uri)
    {
        fs::path p = createClassPath(graph, classname) / getFileName(uri);
        return p;
    }

    std::map<std::string, fs::path> FilesystemBasedBackend::getFiles(const std::string &graph, const std::string &classname)
    {
        std::map<std::string, fs::path> files;
        std::map<std::string, fs::path> classes = this->getClasses(graph);

        for (const auto &[c, p] : classes)
        {
            // 2022-12-22 MS: Before comparing classnames, we have to make sure, that it has been converted first
            if (!classname.empty() && convertClassname(classname) != c)
                continue;
            for (const auto &f : fs::directory_iterator(p))
            {
                const fs::path &fpath = f.path();
                if (!fs::is_regular_file(fpath))
                    continue;
                files[f.path().filename().string()] = fpath;
            }
        }
        return files;
    }

    bool FilesystemBasedBackend::removeFiles(const std::string &graph, const std::set<std::string> &uris)
    {
        std::map<std::string, fs::path> files = this->getFiles(graph);
        for (const std::string& uri : uris)
        {
            const std::string filename = getFileName(uri);
            auto it = files.find(filename);
            while (it != files.end())
            {
                LOGI("Removing " << it->second << " ...");
                if (!fs::remove_all(it->second))
                {
                    LOGE("Failed to remove file " << it->second);
                    return false;
                }
                files.erase(it);
                it = files.find(filename);
            }
        }

        return true;
    }

    bool FilesystemBasedBackend::removeAllFiles(const std::string &graph)
    {
        const std::map<std::string, fs::path> classes = this->getClasses(graph);
        bool success = true;
        for (const auto &[c, path] : classes)
        {
            LOGI("Removing " << path << " ...");
            if (!fs::remove_all(path))
            {
                LOGE("Failed to remove path " << path);
                success = false;
            }
        }
        return success;
    }

    void FilesystemBasedBackend::setWorkingDbPath(const fs::path &db_path)
    {
        m_db_path = db_path;
    }

    fs::path FilesystemBasedBackend::getWorkingDbPath()
    {
        return m_db_path;
    }
}
