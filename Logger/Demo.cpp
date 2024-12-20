#include "Logger.h"
#include <iostream>

template <typename Func>
void performanceTest(Func&& func, uint64_t count = 1000000) {
	uint64_t startTime = Logger::getCurrentTimeMillis();

	// ִ�д���ĺ���
	for (auto i = 0; i < count; ++i) {
		func();  // ֱ�ӵ��ô���ĺ���
	}
	uint64_t stopTime = Logger::getCurrentTimeMillis();
	Logger::console("The execution of %lu times takes %lu milliseconds | %lu times per millisecond", count, stopTime - startTime, count / (stopTime - startTime));
}

void loggerTest() {
	// ��־���󴴽�
	Logger logger("logs");
	//Logger logger("logs", Logger::LogLevel::LOG_INFO, false, true);// �첽��־

	// ��־��������
	Logger::console("codesnippet::main. do something successfully.");
	Logger::console("codesnippet::main. do something result %s.", "success");
	Logger::console("codesnippet::main. error code %d.", -13936);

	logger.debug("codesnippet::main. do something successfully.");//Ĭ����־�ȼ�Ϊinfo�������ӡdebug��־
	logger.setLogLevel(Logger::LOG_DEBUG);// ���ô�ӡ�ȼ�
	logger.debug("codesnippet::main. do something successfully.");
	logger.info("codesnippet::main. do something successfully.");
	logger.warn("codesnippet::main. do something result %s.", "success");
	logger.error("codesnippet::main. error code %d.", -13936);

	//��־���ܲ���
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