#include "game/game.hpp"
#include <iostream>

int main(int argc, char** argv) {
    int ec = EXIT_SUCCESS;
    try {
        std::filesystem::path resources_dir;
        if (argc > 1) {
            resources_dir = argv[1];
        } else {
            resources_dir = std::filesystem::current_path() / "resources";
        }

        std::unique_ptr<game::Game> g = std::make_unique<game::Game>(resources_dir);

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
