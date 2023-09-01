#pragma once
#include <sstream>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

class Logger
{
private:
    inline static std::shared_ptr<spdlog::logger> m_logger;
    inline static std::shared_ptr<spdlog::details::thread_pool> m_thread_pool; // https://github.com/gabime/spdlog/issues/952
    inline static bool m_verbose;
    inline static std::string m_log_file;

public:
    /**
     * @brief Initialize the logger
     */
    static void initialize(bool verbose, const std::string &log_file)
    {
        m_verbose = verbose;
        m_log_file = log_file;

        if (verbose)
        {
            // Initialize thread pool to enable asynchronous logging
            m_thread_pool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
            // Create console/file sink
            std::shared_ptr<spdlog::sinks::sink> sink;
            if (!log_file.empty())
            {
                // Create file sink
                sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
                // Set file sink pattern
                sink->set_pattern("%^[%Y-%m-%d %T.%e] [%t] [%l]: %v%$");
            }
            else
            {
                sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                sink->set_pattern("%^[%T.%e] [%l]: %v%$");
            }

            // Create and register the console logger
            std::vector<spdlog::sink_ptr> sinks{sink};
            m_logger = std::make_shared<spdlog::async_logger>("jsondb", sinks.begin(), sinks.end(), m_thread_pool, spdlog::async_overflow_policy::block);
            spdlog::register_logger(m_logger);
            m_logger->set_level(spdlog::level::warn);
            m_logger->flush_on(spdlog::level::warn);
        }
    }

    /**
     * @brief Shutdown the logger
     */
    static void shutdown()
    {
        if (m_verbose)
        {
            if (m_logger)
                m_logger->flush();
            spdlog::shutdown();
        }
    }

    static const std::shared_ptr<spdlog::logger> &getLogger()
    {
        if (!m_logger)
            initialize(m_verbose, m_log_file);
        return m_logger;
    }
    static void setVerbose(bool verbose)
    {
        if (m_logger)
        {
            m_logger->flush();
            spdlog::drop_all(); // Drop the existing logger and recreate
        }
        initialize(verbose, m_log_file);
    }
    static bool isVerbose()
    {
        return m_verbose;
    }

    static void setLoggerLevel(std::string level)
    {
        if (m_verbose)
        {

            spdlog::level::level_enum spdlevel = spdlog::level::off;
            std::transform(level.begin(), level.end(), level.begin(), toupper);
            if (level == "TRACE")
                spdlevel = spdlog::level::trace;
            else if (level == "DEBUG")
                spdlevel = spdlog::level::debug;
            else if (level == "INFO")
                spdlevel = spdlog::level::info;
            else if (level == "WARN")
                spdlevel = spdlog::level::warn;
            else if (level == "ERROR")
                spdlevel = spdlog::level::err;
            else if (level == "CRITICAL")
                spdlevel = spdlog::level::critical;
            else if (level == "OFF")
                spdlevel = spdlog::level::off;
            else
                throw std::runtime_error("Couldn't set logger level");

            Logger::getLogger()->set_level(spdlevel);
        }
    }
};

/**
 * @brief Some macro shortcuts
 */
#define LOGI(v)                              \
    if (Logger::isVerbose())                 \
    {                                        \
        std::ostringstream ss;               \
        ss << v;                             \
        Logger::getLogger()->info(ss.str()); \
    }

#define LOGW(v)                              \
    if (Logger::isVerbose())                 \
    {                                        \
        std::ostringstream ss;               \
        ss << v;                             \
        Logger::getLogger()->warn(ss.str()); \
    }

#define LOGE(v)                                              \
    if (Logger::isVerbose())                                 \
    {                                                        \
        std::ostringstream ss;                               \
        ss << v << "\nFile: " << __FILE__ << ':' << __LINE__ \
           << "\nFunction: " << __PRETTY_FUNCTION__;         \
        Logger::getLogger()->error(ss.str());                \
    }

#define LOGT(v)                               \
    if (Logger::isVerbose())                  \
    {                                         \
        std::ostringstream ss;                \
        ss << v;                              \
        Logger::getLogger()->trace(ss.str()); \
    }

#define LOG_SET_LEVEL(level, verbose)  \
    {                                  \
        Logger::setLoggerLevel(level); \
        Logger::setVerbose(verbose);   \
    }
