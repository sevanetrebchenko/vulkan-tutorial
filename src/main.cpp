
#include "application.h"
using namespace VT;

#include <stdexcept>
#include <iostream>

int main() {
	Application app(1920, 1080);

	try {
		app.Run();
	}
	catch (std::runtime_error& e) {
	    std::cerr << e.what() << std::endl;
		return 1;
	}
    return 0;
}
