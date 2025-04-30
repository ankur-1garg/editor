#include "Editor.hpp"
#include "frontend/Terminal.hpp" // Concrete frontend
#include <iostream>
#include <filesystem>
#include <cstdlib>    // For getenv
#include <stdexcept>  // For exception handling

// Helper to find config file path (Improved slightly)
std::filesystem::path getConfigPath() {
    std::filesystem::path configPath;
    // 1. Check XDG_CONFIG_HOME or default ~/.config
    const char* xdgConfigHome = getenv("XDG_CONFIG_HOME");
    if (xdgConfigHome && *xdgConfigHome) {
        configPath = std::filesystem::path(xdgConfigHome) / "lite_cpp" / "config.lite";
        if (std::filesystem::exists(configPath)) return configPath;
    } else {
        const char* home = getenv("HOME");
        if (home && *home) {
             configPath = std::filesystem::path(home) / ".config" / "lite_cpp" / "config.lite";
             if (std::filesystem::exists(configPath)) return configPath;
        }
    }

    // 2. Fallback to HOME directory (e.g., ~/.lite_cpp_config.lite)
    const char* home = getenv("HOME");
     if (home && *home) {
         configPath = std::filesystem::path(home) / ".lite_cpp_config.lite";
         if (std::filesystem::exists(configPath)) return configPath;
    }

    // 3. Fallback to executable directory (or current dir if path unknown)
    // This part is platform-dependent to get executable path reliably.
    // For simplicity, just use relative path here.
    configPath = "config.lite";
    return configPath;
}


int main(int argc, char* argv[]) {
    try {
        // 1. Initialize Frontend
        auto frontend = std::make_unique<Terminal>();

        // 2. Create Editor (takes ownership of frontend)
        Editor editor(std::move(frontend));

        // 3. Load Configuration
        std::filesystem::path configPath = getConfigPath();
        if (std::filesystem::exists(configPath)) {
            editor.loadConfig(configPath);
            // No need to print status here, loadConfig does it
        } else {
             if(editor.getFrontend()) editor.getFrontend()->setStatus("Info: No config file found at expected locations.");
        }

        // 4. Handle Command Line Arguments
        if (argc > 1) {
            // TODO: Handle multiple file arguments?
            editor.openFile(argv[1]);
        }

        // 5. Run the Editor's Main Loop
        editor.run();

    } catch (const std::exception& e) {
        // Catch initialization errors or major failures
        // Ensure terminal is restored if possible before printing
        // (Ideally, Frontend destructor handles this via RAII)
        std::cerr << "\nFATAL ERROR: " << e.what() << std::endl;
        // Attempt graceful shutdown if possible (e.g., if frontend exists)
        // Terminal destructor should call endwin() if initialized.
        return 1;
    } catch (...) {
         std::cerr << "\nFATAL UNKNOWN ERROR." << std::endl;
         return 1;
    }

    return 0;
}