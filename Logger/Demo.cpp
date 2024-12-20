#include "Logger.h"

template <typename Func>
void performanceTest(Func&& func, uint64_t count = 1000000) {
	uint64_t startTime = Logger::getCurrentTimeMillis();
	for (auto i = 0; i < count; ++i) {
		func();
	}
	uint64_t stopTime = Logger::getCurrentTimeMillis();
	Logger::console("The execution of %lu times takes %lu milliseconds | %lu times per millisecond", count, stopTime - startTime, count / (stopTime - startTime));
}

int main() {
	// 日志对象创建
	Logger logger;// 默认"logs"文件夹
	//Logger logger("logxx");// 指定日志路径
	//Logger logger("logs", Logger::LogLevel::LOG_INFO, false, true);// 异步日志

	//// 日志基础测试
	//Logger::console("Do something successfully.");
	//Logger::console("Do something result %s.", "success");
	//Logger::console("Error code %d.", -13936);

	logger.debug("Do something successfully.");// 默认日志等级为info，不会打印debug日志
	//logger.setLogLevel(Logger::LOG_DEBUG);// 设置打印等级
	//logger.debug("Do something successfully.");
	//logger.info("Do something successfully.");
	//logger.warn("Do something result %s.", "success");
	//logger.error("Error code %d.", -13936);

	//// 日志性能测试
	//auto loggerLambda = [&logger]() {
	//	return logger.info("Hello World");
	//};
	//for (auto i = 0; i < 10; ++i) {
	//	performanceTest(loggerLambda);
	//}
	return 0;
}