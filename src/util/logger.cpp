#include "logger.hpp"

bool Logger::s_was_used { false };
unsigned Logger::s_last_msg_log_count { 0 };
std::string Logger::s_last_msg { "" };
std::string Logger::s_last_update_name { "" };
std::unordered_set<std::string> Logger::s_updates { };

std::string Logger::get_log()
{
    return get_log_stream().str();
}

void Logger::info(const std::string& msg)
{
    if (msg != s_last_msg)
    {
        s_last_msg_log_count = 1;
        s_last_msg = msg;
        get_log_stream() << (s_was_used ? "\n" : "") << "[INFO] " << msg << std::flush;
        std::cout << (s_was_used ? "\n" : "") << "[INFO] " << msg << std::flush;
    }
    else
    {
        s_last_msg_log_count++;
        get_log_stream() << "\n[INFO] " << msg << "\t\t(" << s_last_msg_log_count << "x)\t";
        std::cout << "\r[INFO] " << msg << "\t\t(" << s_last_msg_log_count << "x)\t";
    }
    s_was_used = true;
    s_last_update_name = "";
}

void Logger::update(const std::string& name, const std::string& msg)
{
    if (msg == s_last_msg && name == s_last_update_name)
    {
        return;
    }
    if (s_updates.count(name) == 0 || name != s_last_update_name)
    {
        s_updates.emplace(name);
        std::cout << "\n[UPDATE] " << msg << "                \t" << std::flush;
    }
    else
    {
        std::cout << "\r[UPDATE] " << msg << "                \t" << std::flush;
    }
    s_last_msg = msg;
    s_last_update_name = name;
}

// to ensure stream is initialized
std::ostringstream& Logger::get_log_stream()
{
    static std::ostringstream s_log_stream { "" };
    return s_log_stream;
}
