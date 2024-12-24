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

int GetConsoleHeight() {
	// 获取标准输出的控制台句柄
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// 检查句柄是否有效
	if (hConsole == INVALID_HANDLE_VALUE) {
		return -1; // 返回 -1 表示出错
	}

	// 获取控制台屏幕缓冲区信息
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
		return -1; // 返回 -1 表示出错
	}

	// 计算控制台窗口的高度
	int consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

	return consoleHeight;
}

int main() {
	// 逐字增加
	std::string msg = "Hello world in C++ by Visual Studio 2022. Happy 2025 Chinese New Year!";
	for (int i = 1; i <= msg.size(); ++i) {
		std::string subString = msg.substr(0, i);
		for (int j = 0; j < GetConsoleHeight(); ++j) {
			Sleep(1);
			Logger::console(subString);
		}
	}

	//// 日志对象创建
	//Logger logger;// 默认"logs"文件夹
	//Logger logger("logxx");// 指定日志路径
	//Logger logger("logs", Logger::LogLevel::LOG_INFO, false, true);// 异步日志

	//// 日志基础测试：控制台日志
	//Logger::console("Do something successfully.");
	//Logger::console("Do something result %s.", "success");
	//Logger::console("Error code %d.", -13936);

	//// 日志基础测试：文件日志
	//logger.debug("Do something successfully.");// 默认日志等级为info，不会打印debug日志
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