#include "Application.h"

#include <exception>
#include <iostream>

int main()
{
    try {
        Application app;
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}