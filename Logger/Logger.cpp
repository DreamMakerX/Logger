#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <functional>
#include <cstdarg>
#include <regex>
#include <io.h>
#include <sys/stat.h>

Logger::Logger(const std::string& folderName, LogLevel level, bool daily, bool async, int logCycle, int retentionDays, size_t maxSize)
	: folderName_(folderName), logLevel_(level), daily_(daily), async_(async), logCycle_(logCycle),
	retentionDays_(retentionDays), maxSize_(maxSize), fileSize_(0), exit_(false), currentFileIndex_(getMaxLogSequence() + 1),
	logThread_(nullptr), checkThread_(nullptr) {
	LPDWORD threadID = 0;
	if (async_) {
		logThread_ = (HANDLE)CreateThread(nullptr, 0, logThreadFunction, (LPVOID)this, 0, threadID);
	}

	checkThread_ = (HANDLE)CreateThread(nullptr, 0, checkThreadFunction, (LPVOID)this, 0, threadID);
}

Logger::~Logger() {
	exit_ = true;
	closeThreadHandle(logThread_);
	closeThreadHandle(checkThread_);
}

void Logger::setLogLevel(LogLevel level) {
	logLevel_ = level;
}

void Logger::debug(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string formattedString = formatString(format, args); // 使用va_list传递
	va_end(args);

	log(formattedString, LogLevel::LOG_DEBUG);
}

void Logger::debug(const std::string& msg){
	debug(msg.c_str());
}

void Logger::info(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string formattedString = formatString(format, args); // 使用va_list传递
	va_end(args);

	log(formattedString, LogLevel::LOG_INFO);
}

void Logger::info(const std::string& msg){
	info(msg.c_str());
}

void Logger::warn(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string formattedString = formatString(format, args); // 使用va_list传递
	va_end(args);

	log(formattedString, LogLevel::LOG_WARNING);
}

void Logger::warn(const std::string& msg){
	warn(msg.c_str());
}

void Logger::error(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string formattedString = formatString(format, args); // 使用va_list传递
	va_end(args);

	log(formattedString, LogLevel::LOG_ERROR);
}

void Logger::error(const std::string& msg){
	error(msg.c_str());
}

void Logger::console(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string formattedString = formatString(format, args); // 使用va_list传递
	va_end(args);

	// 打印格式化的日志信息
	std::cerr << formattedString << std::endl;
}

void Logger::console(const std::string& msg){
	console(msg.c_str());
}

void Logger::log(const std::string& message, LogLevel level) {
	log(message.c_str(), level);
}

void Logger::log(const char* message, LogLevel level) {
	if (level < logLevel_ || message == nullptr) return;

	std::stringstream logStream;
	logStream << "[" << getCurrentTime() << " " << logLevelToString(level) << "] " << message;

	if (async_) {
		{
			LoggerLockGuard lock(logQueueMutex_);
			logQueue_.push_back(logStream.str());
			if (logQueue_.size() >= maxQueueSize_) {
				Sleep(10);// 等待日志打印，防止累积
			}
		}
	}
	else {
		writeToFile(logStream.str());
	}
}

std::string Logger::formatString(const char* format, va_list args) {
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	int rsp = vsnprintf(buffer, sizeof(buffer) - 1, format, args);

	if (rsp < 0 || rsp > static_cast<int>(sizeof(buffer) - 1)) {  // 重新分配内存
		const static size_t MAX_BUFFER_SIZE = 1024 * 1024;
		std::unique_ptr<char[]> newBuffer(new char[MAX_BUFFER_SIZE]);
		memset(newBuffer.get(), 0, MAX_BUFFER_SIZE);
		vsnprintf(newBuffer.get(), MAX_BUFFER_SIZE - 1, format, args);

		return std::string(newBuffer.get());
	}

	return std::string(buffer);
}

std::string Logger::getCurrentTime() {
	SYSTEMTIME st;
	GetLocalTime(&st); // 获取当前本地时间

	// 格式化时间为字符串
	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(4) << st.wYear << "-"    // 年
		<< std::setw(2) << st.wMonth << "-"                        // 月
		<< std::setw(2) << st.wDay << " "                          // 日
		<< std::setw(2) << st.wHour << ":"                         // 时
		<< std::setw(2) << st.wMinute << ":"                       // 分
		<< std::setw(2) << st.wSecond << "."                       // 秒
		<< std::setw(3) << st.wMilliseconds;                       // 毫秒

	return oss.str();
}

std::string Logger::getCurrentDateHour() const{
	SYSTEMTIME st;
	GetLocalTime(&st); // 获取当前本地时间

	// 使用字符串流格式化时间
	std::ostringstream oss;
	if (daily_) {
		// 格式化为 YYYYMMDD
		oss << std::setfill('0') << std::setw(4) << st.wYear
			<< std::setw(2) << st.wMonth
			<< std::setw(2) << st.wDay;
	}
	else {
		// 格式化为 YYYYMMDDHH
		oss << std::setfill('0') << std::setw(4) << st.wYear
			<< std::setw(2) << st.wMonth
			<< std::setw(2) << st.wDay
			<< std::setw(2) << st.wHour;
	}

	return oss.str();
}

std::string Logger::getLogFileName() const {
	std::stringstream fileName;
	fileName << folderName_ << "/" << getCurrentDateHour() << "_" << currentFileIndex_ << ".log";
	return fileName.str();
}

void Logger::writeToFile(const std::string& message) {
	LoggerLockGuard lock(logMutex_);
	if (!logFile_.is_open()) {
		logFile_.open(getLogFileName(), std::ios::out | std::ios::app);
	}

	if (logFile_.is_open()) {
		logFile_ << message << std::endl;
		fileSize_ += message.size() + 2;

		if (fileSize_ >= maxSize_) {
			logFile_.close();
			currentFileIndex_++;
			fileSize_ = 0;
			logFile_.open(getLogFileName(), std::ios::out | std::ios::app);
		}
	}
}

void Logger::cleanOldLogs() const {
	// 通配符路径，匹配所有文件
	std::string searchPath = folderName_ + "\\*";

	struct _finddata_t fileInfo;
	intptr_t handle = _findfirst(searchPath.c_str(), &fileInfo); // 查找第一个文件

	if (handle == -1) {
		std::cerr << "Unable to open directory: " << folderName_ << std::endl;
		return;
	}

	do {
		std::string fileName = fileInfo.name;

		// 检查文件名是否符合条件：包含 "_" 且以 ".log" 结尾
		if (fileName.find("_") != std::string::npos && fileName.find(".log") != std::string::npos) {
			std::string fullPath = folderName_ + "\\" + fileName;

			struct _stat fileStat;
			if (_stat(fullPath.c_str(), &fileStat) == 0) {
				// 获取当前时间
				std::time_t currentTime = std::time(nullptr);
				// 获取文件最后修改时间
				std::time_t lastModifiedTime = fileStat.st_mtime;

				// 计算文件的年龄（天数）
				int daysOld = static_cast<int>((currentTime - lastModifiedTime) / (60 * 60 * 24));

				// 如果文件超出保留天数，删除文件
				if (daysOld > retentionDays_) {
					std::cout << "Deleting old log file: " << fullPath << std::endl;
					if (remove(fullPath.c_str()) != 0) {
						std::cerr << "Failed to delete file: " << fullPath << std::endl;
					}
				}
			}
		}
	} while (_findnext(handle, &fileInfo) == 0); // 查找下一个文件

	// 关闭句柄
	_findclose(handle);
}

int Logger::getMaxLogSequence() const {
	int maxSequence = -1;  // -1 表示没有找到符合条件的日志文件
	std::string currentDateHour = getCurrentDateHour();
	// 添加通配符以匹配所有文件
	std::string searchPath = folderName_ + "\\*";

	struct _finddata_t fileInfo;
	intptr_t handle = _findfirst(searchPath.c_str(), &fileInfo);  // 查找第一个文件

	if (handle == -1) {
		std::cerr << "Failed to open directory: " << folderName_ << std::endl;
		return -1;
	}

	// 使用正则表达式匹配文件名
	std::regex pattern(currentDateHour + "_(\\d+)\\.log"); // 日期+_+序号.log
	std::smatch match;

	do {
		const std::string fileName = fileInfo.name;

		// 如果文件名匹配当前日期和序号模式
		if (std::regex_match(fileName, match, pattern)) {
			// 提取序号并转换为整数
			int sequence = std::stoi(match[1].str());
			maxSequence = max(maxSequence, sequence);
		}
	} while (_findnext(handle, &fileInfo) == 0);  // 查找下一个文件

	// 关闭句柄
	_findclose(handle);
	return maxSequence;
}

void Logger::resetFileIndex() {
	static std::string lastDateHour = getCurrentDateHour();
	if (lastDateHour != getCurrentDateHour()) {
		LoggerLockGuard lock(logMutex_);
		currentFileIndex_ = 0;
		if (!logFile_.is_open()) {
			logFile_.open(getLogFileName(), std::ios::out | std::ios::app);
		}
		else {
			logFile_.close();
			fileSize_ = 0;
			logFile_.open(getLogFileName(), std::ios::out | std::ios::app);
		}
		lastDateHour = getCurrentDateHour();
	}
}

DWORD WINAPI Logger::logThreadFunction(LPVOID lpVoid) {
	Logger* logger = (Logger*)lpVoid;
	static auto lastWriteTime = getCurrentTimeMillis();
	while (!logger->exit_) {
		Sleep(30);
		std::deque<std::string> logsToWrite;
		{
			auto currentTime = getCurrentTimeMillis();
			LoggerLockGuard lock(logger->logQueueMutex_);
			if (logger->logQueue_.empty()) {
				continue;
			}
			if (logger->logQueue_.size() >= maxQueueSize_ || currentTime > lastWriteTime + logger->logCycle_ * 1000) {
				logger->logQueue_.swap(logsToWrite);
				lastWriteTime = currentTime;
			}
		}

		while (!logsToWrite.empty()) {
			logger->writeToFile(logsToWrite.front());
			logsToWrite.pop_front();
		}
	}

	logger->flushRemainingLogs();
	return 0;
}

void Logger::flushRemainingLogs() {
	std::deque<std::string> logsToWrite;
	{
		LoggerLockGuard lock(logMutex_);
		logQueue_.swap(logsToWrite);
	}

	while (!logsToWrite.empty()) {
		writeToFile(logsToWrite.front());
		logsToWrite.pop_front();
	}
}

DWORD WINAPI Logger::checkThreadFunction(LPVOID lpVoid) {
	Logger* logger = (Logger*)lpVoid;
	logger->cleanOldLogs();
	auto lastCleanTime = getCurrentTimeMillis();
	while (!logger->exit_) {
		Sleep(500);
		logger->resetFileIndex();
		executeTaskPeriodically(lastCleanTime, 24 * 60 * 60 * 1000, std::bind(&Logger::cleanOldLogs, logger));
	}
	return 0;
}

void Logger::closeThreadHandle(HANDLE& handle, const DWORD& time)
{
	if (handle != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(handle, time);
		CloseHandle(handle);

		handle = INVALID_HANDLE_VALUE;
	}
}

std::string Logger::logLevelToString(LogLevel level) {
	switch (level) {
	case LogLevel::LOG_DEBUG:
		return "DEBUG";
	case LogLevel::LOG_INFO:
		return "INFO";
	case LogLevel::LOG_WARNING:
		return "WARNING";
	case LogLevel::LOG_ERROR:
		return "ERROR";
	}
	return "UNKNOWN";
}

uint64_t Logger::getCurrentTimeMillis() {
	// 获取当前系统时间
	SYSTEMTIME st;
	GetSystemTime(&st);

	// 将 SYSTEMTIME 转换为 FILETIME
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	// 将 FILETIME 转换为 64 位整数（100 纳秒为单位的时间戳）
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;

	// Windows FILETIME 起始时间是 1601 年 1 月 1 日，减去 Unix 时间起始时间 1970 年 1 月 1 日
	const long long WINDOWS_TO_UNIX_EPOCH = 116444736000000000LL;

	// 计算从 1970 年 1 月 1 日开始的时间戳（单位：毫秒）
	uint64_t timestampInMilliseconds = (ull.QuadPart - WINDOWS_TO_UNIX_EPOCH) / 10000;

	return timestampInMilliseconds;
}
