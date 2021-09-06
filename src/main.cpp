
#include "application.h"
using namespace VT;

#include <stdexcept>
#include <iostream>

int main() {
	Application app(800, 600);

	try {
		app.Run();
	}
	catch (std::runtime_error& e) {
		return 1;
	}
    return 0;
}
