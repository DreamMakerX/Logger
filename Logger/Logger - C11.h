/******************************************************************************/
/* File Name:    Logger.h                                                    */
/* Author:       Congcong Zhang                                              */
/* Date:         2023-07-01                                                  */
/* Version:      2.0                                                         */
/* Description:  This file defines a logging class, compatible with          */
/*               MSVC and MinGW toolchains.                                  */
/*----------------------------------------------------------------------------*/
/* Details:                                                                  */
/*   - The class leverages Windows-specific features and requires a          */
/*     properly configured environment.                                       */
/*   - Future compatibility with newer versions has not been tested.         */
/*----------------------------------------------------------------------------*/
/* Note:                                                                     */
/*   - To support additional IDEs or platforms, code adaptation and          */
/*     redevelopment are necessary.                                          */
/*----------------------------------------------------------------------------*/
/* Compatibility:                                                            */
/*   - Visual Studio 2013 to Visual Studio 2022                              */
/*   - MinGW (C++11 - C++20 standard)                                        */
/*----------------------------------------------------------------------------*/
/* Contact:                                                                  */
/*   - For questions or feedback, contact the author at: zccbjut@gmail.com   */
/******************************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include <atomic>

class Logger {
public:
	enum class LogLevel {// 日志等级
		LOG_DEBUG,
		LOG_INFO,
		LOG_WARNING,
		LOG_ERROR
	};

	// 构造函数
	Logger(const std::string& folderName, LogLevel level = LogLevel::LOG_INFO, bool daily = false,
           bool async = false, uint64_t logCycle = 10, int retentionDays = 30, size_t maxSize = 50 * 1024 * 1024);

	// 析构函数
	~Logger();

	// 设置日志级别
	void setLogLevel(LogLevel level);

    // 打印调试日志
    template <typename... Args>
    void debug(const std::string& format, Args... args) {
        std::string formattedString = formatString(format, args...);
        log(formattedString,  LogLevel::LOG_DEBUG);
    }

    // 打印信息日志
    template <typename... Args>
    void info(const std::string& format, Args... args) {
        std::string formattedString = formatString(format, args...);
        log(formattedString,  LogLevel::LOG_INFO);
    }

    // 打印告警日志
    template <typename... Args>
    void warn(const std::string& format, Args... args) {
        std::string formattedString = formatString(format, args...);
        log(formattedString,  LogLevel::LOG_WARNING);
    }

    // 打印错误日志
    template <typename... Args>
    void error(const std::string& format, Args... args) {
        std::string formattedString = formatString(format, args...);
        log(formattedString,  LogLevel::LOG_ERROR);
    }

    // 打印控制台日志
    template <typename... Args>
    void console(const std::string& format, Args... args) {
        std::string formattedString = formatString(format, args...);
        std::cout << formattedString << std::endl;
    }
private:
    // 格式化字符串并返回 std::string
    template <typename... Args>
    static std::string formatString(const std::string& format, Args&&... args) {
        // 估算需要的空间，提前分配
        size_t estimated_size = format.size() + (sizeof...(args) * 32);  // 大致预估每个参数占32字符空间
        std::vector<char> buffer;
        buffer.reserve(estimated_size);

        // 开始格式化过程
        formatImpl(buffer, format, std::forward<Args>(args)...);

        // 使用构造函数创建 MString 对象
        return std::string(buffer.begin(), buffer.end());
    }
    // 基本类型支持（对所有类型支持输出到流）
    template <typename T>
    static void appendToBuffer(std::vector<char>& buffer, const T& value) {
        std::ostringstream oss;
        oss << value;
        std::string str = oss.str();
        buffer.insert(buffer.end(), str.begin(), str.end());
    }

    // 递归函数，将格式字符串与参数进行匹配并输出到缓冲区
    static void formatImpl(std::vector<char>& buffer, const std::string& format) {
        buffer.insert(buffer.end(), format.begin(), format.end());
    }

    // 主递归实现，处理每个参数并替换{}
    template <typename T, typename... Args>
    static void formatImpl(std::vector<char>& buffer, const std::string& format, T&& value, Args&&... args) {
        size_t pos = format.find("{}");  // 查找第一个 {}
        if (pos != std::string::npos) {
            // 插入 '{}' 前的部分
            buffer.insert(buffer.end(), format.begin(), format.begin() + pos);

            // 插入当前参数
            appendToBuffer(buffer, std::forward<T>(value));

            // 递归处理剩余部分
            formatImpl(buffer, format.substr(pos + 2), std::forward<Args>(args)...);
        } else {
            // 没有更多的 {}，直接插入剩余的格式字符串
            formatImpl(buffer, format);
        }
    }

    // 同步日志
    void log(const std::string& message, LogLevel level = LogLevel::LOG_INFO);

    // 同步日志
    void log(const char* message, LogLevel level = LogLevel::LOG_INFO);
private:
	std::string folderName_;// 日志文件夹名称
	LogLevel logLevel_;// 日志等级
	bool async_;// 是否异步打印
	bool daily_;// 创建日志周期：true:每天创建一个；false:每小时创建一个
	std::atomic<bool> exit_;// 程序退出标识符
	int retentionDays_;// 日志留存时间（天）
	size_t maxSize_;// 单个文件最大长度
	std::ofstream logFile_;// 日志输出对象
	std::thread logThread_;// 异步日志线程
	std::thread checkThread_;// 日志检测线程：超长后新建日志并加后缀做区分；删除旧日志
	std::mutex logMutex_;// 日志输出对象锁
	std::mutex logQueueMutex_;// 异步日志队列锁
	std::deque<std::string> logQueue_;// 异步日志队列
    static const size_t maxQueueSize_ = 100000;// 异步日志队列数最大值
	size_t fileSize_;// 当前文件大小
	int currentFileIndex_; // 每天或每小时的文件编号
	std::chrono::seconds logCycle_;// 日志刷新周期，单位s

	// 获取当前时间的字符串格式
	std::string getCurrentTime() const;

	// 获取当前日期和小时
	std::string getCurrentDateHour() const;

	// 获取日志文件名，基于当前时间和文件编号
	std::string getLogFileName() const;

	// 将日志消息写入文件
	void writeToFile(const std::string& message);

	// 清理过期的日志文件
	void cleanOldLogs() const;

    // 获取当前最大序号
	int getMaxLogSequence();

	// 重置文件编号
	void resetFileIndex();

	// 异步线程工作函数
	void logThreadFunction();

	//确保在关机前写入所有剩余日志
	void flushRemainingLogs();

	// 检测线程工作函数
	void checkThreadFunction();

	// 将日志级别转换为字符串
	std::string logLevelToString(LogLevel level) const;

	// 返回 Unix 纪元时间，精确到毫秒
	static uint64_t getCurrentTimeMillis();

	// 周期执行任务
	template <typename Func>
	static void ExecuteTaskPeriodically(uint64_t & lastTime, uint64_t cycle, Func func) {
		uint64_t nowTime = getCurrentTimeMillis();
		if (nowTime > lastTime + cycle) {
			func();
			lastTime = nowTime;
		}
	}
};

#endif // LOGGER_H
