#include "logger.hpp"

std::string Logger::s_last_msg { "" };

unsigned Logger::s_last_msg_log_count { 0 };

bool Logger::s_was_used { false };

std::string Logger::s_last_update_name { "" };

std::unordered_set<std::string> Logger::s_updates { };

void Logger::info(const std::string& msg)
{
    if (msg != s_last_msg)
    {
        s_last_msg_log_count = 1;
        s_last_msg = msg;
        std::cout << (s_was_used ? "\n" : "") + msg << std::flush;
    }
    else
    {
        s_last_msg_log_count++;
        std::cout << "\r" << msg << "\t\t(" << s_last_msg_log_count << "x)\t";
    }
    s_was_used = true;
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
        std::cout << "\n" << msg << "\t\t" << std::flush;
    }
    else
    {
        std::cout << "\r" << msg << "\t\t" << std::flush;
    }
    s_last_msg = msg;
    s_last_update_name = name;
}
