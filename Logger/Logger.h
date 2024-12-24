/******************************************************************************/
/* File Name:    Logger.h                                                    */
/* Author:       congcong.zhang                                              */
/* Date:         2023-07-01                                                  */
/* Version:      2.0                                                         */
/* Description:  This file defines a logging class for the Windows platform, */
/*               specifically designed for development in the Visual Studio  */
/*               environment. It supports Visual Studio versions 2010 to     */
/*               2022 (MSVC toolchain).                                      */
/*----------------------------------------------------------------------------*/
/* Details:                                                                  */
/*   - The class utilizes Windows-specific features and should be used in    */
/*     a properly configured environment.                                    */
/*   - Future version compatibility has not been verified.                   */
/*----------------------------------------------------------------------------*/
/* Note:                                                                     */
/*   - To extend support for other IDEs or platforms, redevelopment for the  */
/*     respective platform is required.                                      */
/*----------------------------------------------------------------------------*/
/* Compatibility:                                                            */
/*   - Visual Studio 2010 - Visual Studio 2022                               */
/*----------------------------------------------------------------------------*/
/* Contact:                                                                  */
/*   - For the MinGW(g++) version, contact the author at zccbjut@gmail.com.  */
/******************************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <ctime>
#include <deque>
#include <stdint.h>
#include <windows.h>

class LoggerMutex {
public:
	LoggerMutex() {
		InitializeCriticalSection(&section_);
		release = false;
	}
	~LoggerMutex() {
		DeleteCriticalSection(&section_);
		release = true;
	}
	PCRITICAL_SECTION cs() {
		return &section_;
	}
	bool getReleaseFlag() {
		return release;
	}

private:
	bool release;// 防止LoggerMutex释放后，依然使用LoggerLockGuard产生崩溃
	CRITICAL_SECTION section_;
};

class LoggerLockGuard {
public:
	LoggerLockGuard(LoggerMutex& mutex) : mutex_(mutex) {
		if (mutex_.getReleaseFlag()) {
			return;
		}
		EnterCriticalSection(mutex_.cs());
	}
	~LoggerLockGuard() {
		if (mutex_.getReleaseFlag()) {
			return;
		}
		LeaveCriticalSection(mutex_.cs());
	}

private:
	LoggerMutex& mutex_;
};

class Logger {
public:
	enum LogLevel {// 日志等级
		LOG_DEBUG,
		LOG_INFO,
		LOG_WARNING,
		LOG_ERROR
	};

	// 构造函数
	Logger(const std::string& folderName = "logs", LogLevel level = LogLevel::LOG_INFO, bool daily = false,
		bool async = false, int logCycle = 10, int retentionDays = 30, size_t maxSize = 50 * 1024 * 1024);

	// 析构函数
	~Logger();

	// 设置日志级别
	void setLogLevel(LogLevel level);

	// 打印调试日志
	void debug(const char* format, ...);

	// 打印调试日志
	void debug(const std::string& msg);

	// 打印消息日志
	void info(const char* format, ...);

	// 打印消息日志
	void info(const std::string& msg);
	
	// 打印警告日志
	void warn(const char* format, ...);

	// 打印警告日志
	void warn(const std::string& msg);
	
	// 打印错误日志
	void error(const char* format, ...);

	// 打印错误日志
	void error(const std::string& msg);
	
	// 打印控制台日志
	static void console(const char* format, ...);

	// 打印控制台日志
	static void console(const std::string& msg);

	// 关闭线程句柄
	static void closeThreadHandle(HANDLE& handle, const DWORD& time = 3000);

	// 格式化字符串
	static std::string format(const char* format, ...);

	// 获取当前时间的字符串格式，默认精确到毫秒
	static std::string getCurrentDateTime(bool isMillisecondPrecision = true);

	// 获取当前Unix纪元时间，默认精确到毫秒
	static uint64_t getCurrentTimestamp(bool isMillisecondPrecision = true);

	// 周期执行任务
	template <typename Func>
	static void executeTaskPeriodically(uint64_t& lastTime, uint64_t cycle, Func func) {
		uint64_t nowTime = getCurrentTimestamp();
		if (nowTime > lastTime + cycle) {
			func();
			lastTime = nowTime;
		}
	}

private:
	std::string             folderName_;       // 日志文件夹名称
	LogLevel                logLevel_;         // 日志等级
	bool                    async_;            // 日志是否异步
	bool                    daily_;            // 日志创建周期：true:每天创建一个；false:每小时创建一个
	bool                    exit_;             // 程序退出标识符
	int                     retentionDays_;    // 日志留存时间（天）
	size_t                  maxSize_;          // 单个日志最大长度
	std::ofstream           logFile_;          // 日志输出对象
	HANDLE                  logThread_;        // 异步日志线程句柄
	HANDLE                  checkThread_;      // 日志检测线程句柄：超长后新建日志并加后缀做区分；删除旧日志
	LoggerMutex             logMutex_;         // 日志输出对象锁
	LoggerMutex             logQueueMutex_;    // 异步日志队列锁
	std::deque<std::string> logQueue_;         // 异步日志队列
    static const size_t     maxQueueSize_ = 100000; // 异步日志队列数最大值
	size_t                  fileSize_;         // 当前日志大小
	int                     currentFileIndex_; // 同名文件编号
	int                     logCycle_;         // 日志刷新周期，单位s

	// 同步日志
	void log(const std::string& message, LogLevel level = LogLevel::LOG_INFO);

	// 同步日志
	void log(const char* message, LogLevel level = LogLevel::LOG_INFO);

	// 格式化字符串
	static std::string formatString(const char* format, va_list args);

	// 获取当前日期和小时
	std::string getCurrentDateHour() const;

	// 获取日志文件名，基于当前时间和文件编号
	std::string getLogFileName() const;

	// 将日志消息写入文件
	void writeToFile(const std::string& message);

	// 清理过期的日志文件
	void cleanOldLogs() const;

    // 获取当前最大序号
	int getMaxLogSequence() const;

	// 重置文件编号
	void resetFileIndex();

	// 异步线程工作函数
	static DWORD logThreadFunction(LPVOID lpVoid);

	// 确保在关机前写入所有剩余日志
	void flushRemainingLogs();

	// 检测线程工作函数
	static DWORD checkThreadFunction(LPVOID lpVoid);

	// 将日志级别转换为字符串
	static std::string logLevelToString(LogLevel level);
};

#endif // LOGGER_H
