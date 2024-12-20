#include "Logger.h"
#include <iostream>

template <typename Func>
void performanceTest(Func&& func, uint64_t count = 1000000) {
	uint64_t startTime = Logger::getCurrentTimeMillis();

	// 执行传入的函数
	for (auto i = 0; i < count; ++i) {
		func();  // 直接调用传入的函数
	}
	uint64_t stopTime = Logger::getCurrentTimeMillis();
	Logger::console("The execution of %lu times takes %lu milliseconds | %lu times per millisecond", count, stopTime - startTime, count / (stopTime - startTime));
}

void loggerTest() {
	// 日志对象创建
	Logger logger("logs");
	//Logger logger("logs", Logger::LogLevel::LOG_INFO, false, true);// 异步日志

	// 日志基础测试
	Logger::console("codesnippet::main. do something successfully.");
	Logger::console("codesnippet::main. do something result %s.", "success");
	Logger::console("codesnippet::main. error code %d.", -13936);

	logger.debug("codesnippet::main. do something successfully.");//默认日志等级为info，不会打印debug日志
	logger.setLogLevel(Logger::LOG_DEBUG);// 设置打印等级
	logger.debug("codesnippet::main. do something successfully.");
	logger.info("codesnippet::main. do something successfully.");
	logger.warn("codesnippet::main. do something result %s.", "success");
	logger.error("codesnippet::main. error code %d.", -13936);

	//日志性能测试
	auto loggerLambda = [&logger]() {
		return logger.info("Hello World");
	};
	for (auto i = 0; i < 10; ++i) {
		performanceTest(loggerLambda);
	}
}

int main() {
	loggerTest();
	return 0;
}