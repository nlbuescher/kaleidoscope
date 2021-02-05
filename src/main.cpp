#include <iostream>

#include "kaleidoscope/compiler.hpp"

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT double putchard(double a_x)
{
	std::cout << static_cast<char>(a_x) << '\n';
	return 0;
}

extern "C" DLLEXPORT double printd(double a_x)
{
	std::cout << a_x << '\n';
	return 0;
}

int main()
{
	Kaleidoscope::Compiler compiler{};

	std::string line;
	while (true) {
		std::cout << "ready> ";
		std::getline(std::cin, line);

		if (line == "exit()") { break; }

		compiler.handle(line);
	}

	std::cout << "finished\n";
}
