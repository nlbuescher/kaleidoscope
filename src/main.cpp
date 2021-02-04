#include <iostream>

#include "kaleidoscope/compiler.hpp"

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

	compiler.print(llvm::outs());

	std::cout << "finished\n";
}
