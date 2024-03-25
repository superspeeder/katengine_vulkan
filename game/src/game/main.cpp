#include "game/game.hpp"
#include <iostream>

int main() {
    int ec = EXIT_SUCCESS;
    try {
        std::unique_ptr<game::Game> g = std::make_unique<game::Game>();

        g->launch();
        g->context()->device().waitIdle();

    } catch (const std::exception &e) {
        std::cerr << "Fatally crashed: " << e.what() << std::endl;
        std::cerr << "Fatal error encountered. Exiting." << std::endl;
        ec = EXIT_FAILURE;
    }

    glfwTerminate();
    
    return ec;
}
