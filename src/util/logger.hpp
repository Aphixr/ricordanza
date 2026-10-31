#pragma once

#include <iostream>
#include <string>

class Logger
{

public:

    static void info(const std::string& msg);

private:

    static std::string s_last_msg;

    static unsigned s_last_msg_log_count;

    static bool s_was_used;

    Logger() = default;

};
