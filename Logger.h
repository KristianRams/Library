#pragma once
 
#include "String.h"
#include "Array.h"
#include "Ascii_Color_Codes.h"

#include <thread> 
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <sstream>
#include <fstream>
#include <assert.h>
#include <queue>
#include <utility>

// A simple header only thread-safe logger class.
const char *to_string[] {
    "None",
    "Warning",
    "Debug",
    "Error",
};

enum Log_Level : u8 { 
    NONE,
    WARNING,
    DEBUG,
    ERROR,
};

struct Log_Message { 
    Log_Level log_level;
    string    log_contents;
};

struct Logger { 
private:
    void logger_thread_process() { 
        Log_Message log_message;
        while (true) {
            std::unique_lock<std::mutex> lock(logger_mutex);
            logger_condition.wait(lock, [this]() {
                    // We have two cases that we care about there. The 1st is the normal case when the logger has been
                    // initialized and logging happens.
                    // The 2nd happens when we've deinitialized the logger and we have to flush out all messages from the logger message queue.
                    // 1) If the thread has been signaled to pull and log messages off the queue then do so.
                    // 2) If the logger is not longer active then fall through.
                    return (thread_work_signaled.load(std::memory_order_seq_cst) || !logger_active.load(std::memory_order_seq_cst));
                    
                });
                
            // If we're not active and the logger queue is empty then return.
            if (!logger_active.load(std::memory_order_seq_cst) && logger_message_queue.empty()) { return; }
                
                
            // While the logger message queue still has messages, continue logging.
            while (!logger_message_queue.empty()) {
                log_message.log_level    = logger_message_queue.front().log_level;
                log_message.log_contents = logger_message_queue.front().log_contents;
                Logger::do_print(&log_message);
                logger_message_queue.pop();
            }
        }
    }
    
public:
    // No copying around Loggers.
    Logger(const Logger &rhs)            = delete; 
    Logger &operator=(const Logger &rhs) = delete;
    Logger(Logger &&rhs)                 = delete;
    Logger &operator=(Logger &&rhs)      = delete;

    // Stores the formatted output message
    std::stringstream output_stream;

    Log_Level current_level = Log_Level::NONE;
    Log_Level filter        = Log_Level::NONE;

    // Queue and mutex associated with the queue. Since we can have multiple 
    // threads modifying the state of the queue so we need a guard.
    std::queue<Log_Message> logger_message_queue;
    std::mutex              logger_mutex;

    // Blocks the logger thread so we only do work when we are signaled 
    // to prevent pegging the CPU.
    std::condition_variable logger_condition;

    // Thread that blocks until it has been signaled that there is a message to pull 
    // off the queue and log to all sinks provided.
    std::atomic<bool> thread_work_signaled;
    std::thread       logger_thread;

    // All destination sinks where logging with commence.
    Array <std::ostream *> sinks;

    // logger_active means that we are in a state of logging.
    std::atomic<bool> logger_active;
    
    Logger() :
         thread_work_signaled(false),
         logger_active(true),
         logger_thread(std::thread(&Logger::logger_thread_process, this)) {}

    ~Logger() { 
        thread_work_signaled.store(false, std::memory_order_seq_cst);
        logger_active.store(false, std::memory_order_seq_csdt);
        logger_condition.notify_one();

        for (auto *elem: sinks) { (*elem).flush(); }

        if (logger_thread.joinable()) { logger_thread.join(); }
    }

    void add_sink(std::ostream *sink) { 
        if (sink == nullptr) { return; }
        std::unique_lock<std::mutex> lock(logger_mutex);
        array_add(&sinks, sink);
    }

    void set_log_level(Log_Level log_level) { 
        std::unique_lock<std::mutex> lock(logger_mutex);
        current_level = log_level;
    }
    
    Log_Level get_log_level() const { 
        return current_level;
    }
    
    // @Note: Filter takes higher priority over setting the log level.
    void set_filter(Log_Level log_level) { 
        std::unique_lock<std::mutex> lock(logger_mutex);
        filter = log_level;
    }
    
    void reset_filter(Log_Level log_level) { 
        std::unique_lock<std::mutex> lock(logger_mutex);
        filter = Log_Level::NONE;
    }
    
    void do_print(Log_Message *log_message) {
        // If we want to filter then check that the logLevel is indeed the filtered log level type.
        if (filter != Log_Level::NONE && filter != log_message->log_level) { return; } 
        
        // If the filtered logLevel type is the passed in logLevel type then short circuit and print it out
        // else then proceed with the rules for currentLevel i.e. only print logLevel types of currentLevel logLevel type and higher.
        // Add line number & file name... 
        if ((filter == log_message->log_level) || (current_level <= log_message->log_level && current_level != Log_Level::NONE)) { 
            output_stream << to_string[static_cast <s32>(log_message->log_level)] << ": ";
            output_stream.write(log_message->log_contents.raw(), log_message->log_contents.size());

            // @Todo: If the logLevel == ERROR then print out the string to 
            // the console in all red then remove those ascii codes, so we don't 
            // include it if we are writing to a file as well.
            for (auto *elem: sinks) {
                (*elem) << output_stream.str();
            }

            output_stream.str("");
        }
    }
    
    template <typename ...Args>
    string get_format_string(Args ...args) {
        if (current_level != Log_Level::NONE) {
            s64 size = std::snprintf(NULL, 0, args...) + 1;
            if (size < 0) { assert(false); return ""; }
            string output(size, '\0');
            std::sprintf(output.raw(), args...);
            return output;
        }
        printf("%s\n", "Error: You did not set the logLevel.");
        return "";
    }

    template <typename ...Args>
    void helper(Log_Level log_level, Args ...args) { 
        std::lock_guard<std::mutex> lock(logger_mutex);
        string format_string = get_format_string(args...);
        if (format_string.empty()) { return; }
        Log_Message log_message;
        log_message.log_level = log_level;
        log_message.log_contents = format_string;
        logger_message_queue.push(log_message);
        thread_work_signaled.store(true, std::memory_order_seq_cst);
        logger_condition.notify_one();
    }

    template <typename ...Args>
    void warn(Args ...args) { 
        helper(Log_Level::WARNING, args...);
    }

    template <typename ...Args>
    void debug(Args ...args) { 
        helper(Log_Level::DEBUG, args...);
    }
    
    template <typename ...Args>
    void error(Args ...args) { 
        helper(Log_Level::ERROR, args...);
    }
};
