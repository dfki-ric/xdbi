#include "FilesystemBasedLock.hpp"

namespace xdbi
{

    FilesystemBasedLock::FilesystemBasedLock(const fs::path &db_path)
        : m_db_path(db_path)
    {
    }

    void FilesystemBasedLock::makeDir(const fs::path &path)
    {
        if (fs::exists(path))
        {
            LOGI("Directory " << path << " already exists");
            return;
        }
        if (!fs::create_directories(path))
            LOGE("failed to create directories at " << path);
    }

    bool FilesystemBasedLock::lockDB(const std::string &graph)
    {
        if (graph.empty())
            throw std::invalid_argument("FilesystemBasedLock::lockDB(): graph is empty");
        /*
        open(filename, O_RDWR|O_CREAT, 0666)
        0666 is an octal number, i.e. every one of the 6's corresponds to three permission bits

        6 = rw

        7 = rwx
        */
        const fs::path mutex_file_path = m_db_path / graph / fs::path("mutex_file");
        if (!fs::is_directory(m_db_path / graph))
        {
            this->makeDir(m_db_path / graph);
        }
        const int fd = open(mutex_file_path.string().c_str(), O_CREAT | O_RDWR, 0666);
        if (fd < 0)
        {
            LOGE("Failed to open file " << mutex_file_path.string());
            return false;
        }
        if (flock(fd, LOCK_EX) < 0)
        {
            LOGE("Failed to lock file " << mutex_file_path.string());
            close(fd);
            return false;
        }
        // NOTE: After a successfull call to the blocking flock() we are safe to proceed into a critical section here
        m_mutex[graph] = fd; /* update fd */
        return true;
    }

    void FilesystemBasedLock::unlockDB(const std::string &graph)
    {
        if (graph.empty())
            throw std::invalid_argument("FilesystemBasedLock::unlockDB(): graph is empty");
        try
        {
            const int fd = m_mutex.at(graph); // get fd to unlock
            flock(fd, LOCK_UN);               // Unlock the file
            close(fd);                        // close fd
        }
        catch (const std::out_of_range &)
        {
            LOGE("Could not find mutex for graph " << graph);
        }
    }

    bool FilesystemBasedLock::isFileLocked(const int fd)
    {
        struct flock fl{};
        fl.l_type = F_RDLCK;
        fl.l_start = 0;
        fl.l_whence = SEEK_SET;
        fcntl(fd, F_GETLK, &fl);
        return fl.l_type == F_UNLCK;
    }
}
