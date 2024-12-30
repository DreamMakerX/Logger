#include "Logger.h"

template <typename Func>
void performanceTest(Func&& func, uint64_t count = 1000000) {
	uint64_t startTime = Logger::getCurrentTimestamp();
	for (auto i = 0; i < count; ++i) {
		func();
	}
	uint64_t stopTime = Logger::getCurrentTimestamp();
	Logger::console("The execution of %lu times takes %lu milliseconds | %lu times per millisecond", count, stopTime - startTime, count / (stopTime - startTime));
}

int main() {
	//// 日志对象创建
	Logger logger;// 默认"logs"文件夹
	//Logger logger("logxx");// 指定日志路径
	//Logger logger("logs", Logger::LOG_INFO, false, true);// 异步日志

	// 创建日志文件夹（可选）
	logger.createFolder();
	
	// 获取16进制表示
	std::string msg = "Application started successfully.";
	Logger::console(Logger::getHexString(msg.c_str(), msg.size()));

	// 日志基础测试：控制台日志
	Logger::console("Application started successfully.");
	Logger::console("Low memory detected. Consider freeing some %s.", "resources");
	Logger::console("Failed to open the no.%d configuration file. Please check the path.", 13936);

	// 日志基础测试：输出窗口日志
	Logger::output("Application started successfully.");
	Logger::output("Low memory detected. Consider freeing some %s.", "resources");
	Logger::output("Failed to open the no.%d configuration file. Please check the path.", 13936);

	// 日志基础测试：文件日志
	logger.debug("Application started successfully.");// 默认日志等级为info，不会打印debug日志
	logger.setLogLevel(Logger::LOG_DEBUG);// 设置打印等级
	logger.debug("Application started successfully.");
	logger.info("The user has logged in successfully.");
	logger.warn("Low memory detected. Consider freeing some %s.", "resources");
	logger.error("Failed to open the no.%d configuration file. Please check the path.", 13936);

	// 日志性能测试
	auto loggerLambda = [&logger]() {
		return logger.info("Hello, World!");
	};
	for (auto i = 0; i < 10; ++i) {
		performanceTest(loggerLambda);
	}
	return 0;
}