#include "Logger.h"
#include <iomanip>
#include <chrono>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <cstdarg>
#include <regex>

#ifdef _MSC_VER
#include <windows.h>   // Windows API (VS2015 环境)
#include <io.h>        // _unlink
#else
#include <sys/stat.h>
#include <dirent.h>  // POSIX 文件操作
#endif

Logger::Logger(const std::string& folderName, LogLevel level, bool daily, bool async, uint64_t logCycle, int retentionDays, size_t maxSize)
	: folderName_(folderName), logLevel_(level), async_(async), logCycle_(logCycle),
	daily_(daily), retentionDays_(retentionDays), maxSize_(maxSize), fileSize_(0), exit_(false),
	currentFileIndex_(getMaxLogSequence() + 1) {

	if (async_) {
		logThread_ = std::thread(&Logger::logThreadFunction, this);
	}

	checkThread_ = std::thread(&Logger::checkThreadFunction, this);
}

Logger::~Logger() {
	exit_ = true;
	if (async_ && logThread_.joinable()) {
		logThread_.join();
	}
	if (checkThread_.joinable()) {
		checkThread_.join();
	}
}

void Logger::setLogLevel(LogLevel level) {
	logLevel_ = level;
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
			std::lock_guard<std::mutex> lock(logQueueMutex_);
			logQueue_.push_back(logStream.str());
			if (logQueue_.size() >= maxQueueSize_) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));// 等待日志打印，防止累积
			}
		}
	}
	else {
		writeToFile(logStream.str());
	}
}


std::string Logger::getCurrentTime() const {
	auto now = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
	auto duration = now_ms.time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration) - std::chrono::duration_cast<std::chrono::seconds>(duration);

	std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	std::tm time;
	localtime_s(&time, &currentTime);
	ss << std::put_time(&time, "%Y-%m-%d %H:%M:%S");
	ss << '.' << std::setw(3) << std::setfill('0') << milliseconds.count();
	return ss.str();
}

std::string Logger::getCurrentDateHour() const {
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
	localtime_s(&tm, &time);
	std::stringstream ss;
	if (daily_) {
		ss << std::put_time(&tm, "%Y%m%d");
	}
	else {
		ss << std::put_time(&tm, "%Y%m%d%H");
	}
	return ss.str();
}

std::string Logger::getLogFileName() const {
	std::stringstream fileName;
	fileName << folderName_ << "/" << getCurrentDateHour() << "_" << currentFileIndex_ << ".log";
	return fileName.str();
}

void Logger::writeToFile(const std::string& message) {
	std::lock_guard<std::mutex> lock(logMutex_);
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
#ifdef _MSC_VER
	// 添加通配符以匹配所有文件
	std::string searchPath = folderName_ + "\\*";

	struct _finddata_t fileInfo;
	intptr_t handle = _findfirst(searchPath.c_str(), &fileInfo);  // 查找第一个文件

	if (handle == -1) {
		std::cerr << "Unable to open directory: " << folderName_ << std::endl;
		return;
	}

	do {
		std::string fileName = fileInfo.name;

		// 过滤文件名，检查是否符合条件：包含 "_" 且以 ".log" 结尾
		if (fileName.find("_") != std::string::npos && fileName.find(".log") != std::string::npos) {
			std::string fullPath = folderName_ + "\\" + fileName;

			struct _stat fileStat;
			if (_stat(fullPath.c_str(), &fileStat) == 0) {
				// 获取文件最后修改时间
				std::time_t lastModifiedTime = fileStat.st_mtime;
				std::chrono::system_clock::time_point fileTimePoint = std::chrono::system_clock::from_time_t(lastModifiedTime);
				auto duration = std::chrono::system_clock::now() - fileTimePoint;
				int daysOld = std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;

				// 如果文件超出保留天数，删除文件
				if (daysOld > retentionDays_) {
					std::cout << "Deleting old log file: " << fullPath << std::endl;
					if (remove(fullPath.c_str()) != 0) {
						std::cerr << "Failed to delete file: " << fullPath << std::endl;
					}
				}
			}
		}
	} while (_findnext(handle, &fileInfo) == 0);  // 查找下一个文件

	// 关闭句柄
	_findclose(handle);
#else
	DIR* dir = opendir(folderName_.c_str());
	if (dir == nullptr) {
		std::cerr << "Unable to open directory: " << folderName_ << std::endl;
		return;
	}
	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		std::string fileName = entry->d_name;
		if (fileName.find("_") != std::string::npos && fileName.find(".log") != std::string::npos) {
			std::string fullPath = folderName_ + "/" + fileName;

			struct stat fileStat;
			if (stat(fullPath.c_str(), &fileStat) == 0) {
				std::time_t lastModifiedTime = fileStat.st_mtime;
				std::chrono::system_clock::time_point fileTimePoint = std::chrono::system_clock::from_time_t(lastModifiedTime);
				auto duration = std::chrono::system_clock::now() - fileTimePoint;
				int daysOld = std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;

				if (daysOld > retentionDays_) {
					std::cout << "Deleting old log file: " << fullPath << std::endl;
					remove(fullPath.c_str());
				}
			}
		}
	}
	closedir(dir);
#endif
}

int Logger::getMaxLogSequence() {
	int maxSequence = -1;  // -1 表示没有找到符合条件的日志文件
	std::string currentDateHour = getCurrentDateHour();
#ifdef _MSC_VER
	// 添加通配符以匹配所有文件
	std::string searchPath = folderName_ + "\\*";

	struct _finddata_t fileInfo;
	intptr_t handle = _findfirst(searchPath.c_str(), &fileInfo);  // 查找第一个文件

	if (handle == -1) {
		std::cerr << "Failed to open directory: " << folderName_ << std::endl;
		return -1;
	}

	// 使用正则表达式匹配文件名
	std::regex pattern(currentDateHour + R"(_(\d+)\.log)"); // 日期+_+序号.log
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
#else
	DIR* dir = opendir(folderName_.c_str());
	if (dir == nullptr) {
		std::cerr << "Failed to open directory: " << folderName_ << std::endl;
		return -1;
	}
	struct dirent* entry;

	std::regex pattern(currentDateHour + R"(_(\d+)\.log)");  // 正则表达式：日期+_+序号.log

	// 遍历目录中的所有文件
	while ((entry = readdir(dir)) != nullptr) {
		const std::string filename = entry->d_name;

		// 如果文件名匹配当前日期和序号模式
		std::smatch match;
		if (std::regex_match(filename, match, pattern)) {
			// 提取序号并转换为整数
			int sequence = std::stoi(match[1].str());
			maxSequence = std::max(maxSequence, sequence);
	}
}

	closedir(dir);  // 关闭目录
	return maxSequence;
#endif
}

void Logger::resetFileIndex() {
	static std::string lastDateHour = getCurrentDateHour();
	if (lastDateHour != getCurrentDateHour()) {
		std::lock_guard<std::mutex> lock(logMutex_);
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

void Logger::logThreadFunction() {
	static auto lastWriteTime = std::chrono::system_clock::now();
	while (!exit_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		std::deque<std::string> logsToWrite;
		{
			auto currentTime = std::chrono::system_clock::now();
			std::lock_guard<std::mutex> lock(logQueueMutex_);
            if(logQueue_.empty()){
                continue;
            }
			if (logQueue_.size() >= maxQueueSize_ || currentTime > lastWriteTime + logCycle_) {
				logQueue_.swap(logsToWrite);
				lastWriteTime = currentTime;
			}
		}

		while (!logsToWrite.empty()) {
			writeToFile(logsToWrite.front());
			logsToWrite.pop_front();
		}
	}

	flushRemainingLogs();
}

void Logger::flushRemainingLogs() {
	std::deque<std::string> logsToWrite;
	{
		std::lock_guard<std::mutex> lock(logMutex_);
		logQueue_.swap(logsToWrite);
	}

	while (!logsToWrite.empty()) {
		writeToFile(logsToWrite.front());
		logsToWrite.pop_front();
	}
}

void Logger::checkThreadFunction() {
	cleanOldLogs();
	auto lastCleanTime = getCurrentTimeMillis();
	while (!exit_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		resetFileIndex();
		ExecuteTaskPeriodically(lastCleanTime, 24 * 60 * 60 * 1000, std::bind(&Logger::cleanOldLogs, this));
	}
}

std::string Logger::logLevelToString(LogLevel level) const {
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
	// 获取当前时间点
	auto now = std::chrono::system_clock::now();
	// 转换为time_point_cast，以毫秒为单位
	auto duration = now.time_since_epoch();
	// 将duration转换为毫秒
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	return millis;
}
