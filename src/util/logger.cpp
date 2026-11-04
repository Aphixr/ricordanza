#include "logger.hpp"

std::string Logger::s_last_msg { "" };

unsigned Logger::s_last_msg_log_count { 0 };

bool Logger::s_was_used { false };

void Logger::info(const std::string& msg)
{
    if (msg != s_last_msg)
    {
        s_last_msg_log_count = 0;
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
