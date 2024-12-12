
#pragma once

#include "AVConsole.h"

#include <vector>
#include <string>
#include <unordered_map>

#include <assert.h>

class CmdParser
{
public:
    typedef std::vector<std::string>                      arg_list_t;
    typedef std::unordered_map<std::string, std::string>  dict_type;

    enum {
        E_CMD_ERROR = -1,
        S_CMD_OK,
        E_CMD_WARNING,
        E_CMD_LAST
    };

    static std::string empty_vlaue;

    CmdParser() {
    }
    CmdParser(int argc, char * argv[]) {
        parse(argc, argv);
    }
    ~CmdParser() {
    }

    std::string exe() const {
        return exe_;
    }

    std::size_t count() const {
        return args_.size();
    }

    std::string params(int index) const {
        if (index < count())
            return args_[index];
        else
            return "";
    }

    bool is_contains(const std::string & key) const {
        auto iter = dict_.find(key);
        return (iter != dict_.end());
    }

    std::string & find(const std::string & key) {
        auto iter = dict_.find(key);
        if (iter != dict_.end())
            return iter->second;
        else
            return empty_vlaue;
    }

    const std::string & find(const std::string & key) const {
        const auto iter = dict_.find(key);
        if (iter != dict_.cend())
            return iter->second;
        else
            return empty_vlaue;
    }

    void clear() {
        exe_ = "";
        args_.clear();
        no_keys_.clear();
        dict_.clear();
    }

    void dump() {
        console.print("Exec: %s, Path: %s\n\n", exe_.c_str(), path_.c_str());

        console.print("Argument list:\n\n");
        for (const auto & arg : args_) {
            console.print("%s ", arg.c_str());
        }

        console.print("\n\n");

        console.print("Argument dictionary:\n\n");
        for (const auto & pair : dict_) {
            console.print("Key: \"%s\", Value: \"%s\"\n", pair.first.c_str(), pair.second.c_str());
        }

        console.print("\n\n");

        console.print("No keys argument(s):\n\n");
        for (auto const & arg : no_keys_) {
            console.print("- \"%s\"\n", arg.c_str());
        }

        console.print("\n");
    }

    int parse(int argc, char * argv[]) {
        clear();

        int result = E_CMD_ERROR;
        if (argc >= 1) {
            std::string exe_full, path, exec;
            exe_full = argv[0];
            result = parse_path_and_exe(exe_full, path, exec);
            if (result == S_CMD_OK) {
                path_ = path;
                exe_  = exec;
            }
        }
        for (int i = 1; i < argc; i++) {
            args_.push_back(argv[i]);
        }

        int idx = 0;
        while (idx < count()) {
            std::string key, value;
            const std::string & arg = args_[idx];
            std::size_t len = arg.size();
            if ((len > 0) && (arg[0] == '-')) {
                if ((len > 1) && arg[1] == '-') {
                    // Parse long key
                    result = parse_long_key(idx, arg, key, value);
                }
                else {
                    // Parse short key
                    result = parse_short_key(idx, arg, key, value);
                }
                if (result != E_CMD_ERROR) {
                    dict_.insert(std::make_pair(key, value));
                }
            }
            else {
                no_keys_.push_back(arg);
            }
            idx++;
        }
        return result;
    }

protected:
    bool start_with_c(const std::string & str, char prefix) {
        return (str.size() >= 1) ? (str[0] == prefix) : false;
    }

    bool start_with(const std::string & str, const std::string & prefix) {
        if (str.size() >= prefix.size()) {
            for (int i = 0; i < prefix.size(); i++) {
                if (str[i] != prefix[i]) {
                    return false;
                }
            }
            return (prefix.size() > 0);
        }
        else {
            return false;
        }
    }

    int parse_value(int & idx, const std::string & key, std::string & value) {
        if ((idx + 1) < count()) {
            value = args_[idx + 1];
            if (!start_with_c(value, '-') && !start_with(value, "--")) {
                idx++;
                return S_CMD_OK;
            }
            else {
                value = "";
                return S_CMD_OK;
            }
        }
        else {
            // Only key, missing the value.
            console.warning("CmdParser: Only key: [%s], missing the value.", key.c_str());
            value = "";
            return S_CMD_OK;
        }
    }

    int parse_long_key(int & idx, const std::string & arg, std::string & key, std::string & value) {
        // Long key
        int result;
        if (arg.size() > 2) {
            assert((arg[0] == '-') && (arg[1] == '-'));
            key = arg.substr(2, std::string::npos);
            if (key.size() > 1) {
                result = parse_value(idx, key, value);
                return result;
            }
            else {
                // Long key, but the size of key name is too short.
                console.warning("CmdParser: Long key: [%s], but the size of key name is too short.", key.c_str());
                result = parse_value(idx, key, value);
                return E_CMD_WARNING;
            }
        }
        else {
            // Error: only "--", missing the key name.
            console.warning("CmdParser: Only long key prefix, missing the key name.");
            return E_CMD_ERROR;
        }
    }

    int parse_short_key(int & idx, const std::string & arg, std::string & key, std::string & value) {
        // short key
        int result = E_CMD_ERROR;
        if (arg.size() > 1) {
            assert((arg[0] == '-') && (arg[1] != '-'));
            key = arg.substr(1, std::string::npos);
            if (key.size() == 1) {
                result = parse_value(idx, key, value);
                return result;
            }
            else if (key.size() > 1) {
                // Short key, but the size of key name is too long.
                console.warning("CmdParser: Short key: [%s], but the size of key name is too long.", key.c_str());
                result = parse_value(idx, key, value);
                return E_CMD_WARNING;
            }
            else {
                // It's unreachable.
                return E_CMD_ERROR;
            }
        }
        else {
            // Error: only "-", missing the key name.
            console.warning("CmdParser: Only short key prefix, missing the key name.");
            return E_CMD_ERROR;
        }
    }

    int parse_path_and_exe(const std::string & full_path,
                           std::string & path, std::string & exec) {
        // Find last "\" or "/" char
        std::size_t last_pos = std::size_t(-1);
        for (std::size_t i = full_path.size() - 1; i >= 0; i--) {
            char ch = full_path[i];
            if (ch == '\\' || ch == '/') {
                last_pos = i;
                break;
            }
        }
        if (last_pos != std::size_t(-1)) {
            if (last_pos > 1)
                path = full_path.substr(0, last_pos);
            else
                path = "";
            exec = full_path.substr((last_pos + 1), std::string::npos);
            return S_CMD_OK;
        }
        else {
            path = full_path;
            exec = "";
            return E_CMD_ERROR;
        }
    }

protected:
    std::string exe_;
    std::string path_;
    arg_list_t  args_;
    arg_list_t  no_keys_;
    dict_type   dict_;
};

__declspec(selectany) std::string CmdParser::empty_vlaue;
