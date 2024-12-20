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
	// ��־���󴴽�
	Logger logger("logs");
	//Logger logger("logs", Logger::LogLevel::LOG_INFO, false, true);// �첽��־

	// ��־��������
	Logger::console("Do something successfully.");
	Logger::console("Do something result %s.", "success");
	Logger::console("Error code %d.", -13936);

	logger.debug("Do something successfully.");//Ĭ����־�ȼ�Ϊinfo�������ӡdebug��־
	logger.setLogLevel(Logger::LOG_DEBUG);// ���ô�ӡ�ȼ�
	logger.debug("Do something successfully.");
	logger.info("Do something successfully.");
	logger.warn("Do something result %s.", "success");
	logger.error("Error code %d.", -13936);

	//��־���ܲ���
	auto loggerLambda = [&logger]() {
		return logger.info("Hello World");
	};
	for (auto i = 0; i < 10; ++i) {
		performanceTest(loggerLambda);
	}
	return 0;
}