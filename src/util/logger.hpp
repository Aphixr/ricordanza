#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>

class Logger
{

public:

    // get everything that was logged
    static std::string get_log();

    // prints to console and groups identical messages
    static void info(const std::string& msg);

    // print an update
    // overwrites current line regardless of whether the message is identical to the previous or not
    static void update(const std::string& name, const std::string& msg);

private:

    static std::string s_last_msg;

    static unsigned s_last_msg_log_count;

    static bool s_was_used;

    static std::string s_last_update_name;

    static std::unordered_set<std::string> s_updates;

    static std::ostringstream& get_log_stream();

    Logger() = default;

};
