#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    std::vector<wchar_t> executable_path(32768);
    const DWORD executable_path_length = GetModuleFileNameW(nullptr, executable_path.data(), static_cast<DWORD>(executable_path.size()));
    if (!executable_path_length || executable_path_length == executable_path.size())
        return EXIT_FAILURE;

    const std::filesystem::path distribution_directory = std::filesystem::path(executable_path.data()).parent_path();
    const std::filesystem::path engine_path = distribution_directory / L"bin" / L"speed-dreams-2.exe";
    const std::filesystem::path data_directory = distribution_directory / L"share" / L"games" / L"speed-dreams-2";
    const std::filesystem::path libraries_directory = distribution_directory / L"lib" / L"games" / L"speed-dreams-2";
    const std::filesystem::path settings_directory = distribution_directory / L"user-settings";
    const std::filesystem::path initial_settings_directory = distribution_directory / L"initial-settings";
    if (!std::filesystem::exists(engine_path) || !std::filesystem::exists(data_directory / L"tracks" / L"circuit" / L"melbourne-nineteen-ninety-seven" / L"melbourne-nineteen-ninety-seven.xml"))
    {
        MessageBoxW(nullptr, L"The Open Formula distribution is incomplete.", L"Open Formula", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    std::filesystem::create_directories(settings_directory);
    for (const auto& entry : std::filesystem::recursive_directory_iterator(initial_settings_directory))
    {
        const std::filesystem::path destination = settings_directory / std::filesystem::relative(entry.path(), initial_settings_directory);
        if (entry.is_directory())
            std::filesystem::create_directories(destination);
        else if (!std::filesystem::exists(destination))
            std::filesystem::copy_file(entry.path(), destination);
    }

    const std::wstring binaries_directory = (distribution_directory / L"bin").wstring();
    const DWORD environment_length = GetEnvironmentVariableW(L"PATH", nullptr, 0);
    std::vector<wchar_t> environment_path(environment_length + 1);
    GetEnvironmentVariableW(L"PATH", environment_path.data(), static_cast<DWORD>(environment_path.size()));
    SetEnvironmentVariableW(L"PATH", (binaries_directory + L";" + environment_path.data()).c_str());
    SetEnvironmentVariableW(L"OSG_LIBRARY_PATH", (distribution_directory / L"bin" / L"osgPlugins-3.6.5").c_str());
    std::wstring command = L"\"" + engine_path.wstring() + L"\" --datadir \"" + data_directory.wstring()
        + L"\" --libdir \"" + libraries_directory.wstring() + L"\" --bindir \"" + binaries_directory
        + L"\" --localdir \"" + settings_directory.wstring() + L"\" --startrace williams-melbourne-practice";
    STARTUPINFOW startup_information = {};
    startup_information.cb = sizeof(startup_information);
    SECURITY_ATTRIBUTES log_security = {};
    log_security.nLength = sizeof(log_security);
    log_security.bInheritHandle = TRUE;
    HANDLE runtime_log = CreateFileW((settings_directory / L"runtime.log").c_str(), GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, &log_security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (runtime_log != INVALID_HANDLE_VALUE)
    {
        startup_information.dwFlags = STARTF_USESTDHANDLES;
        startup_information.hStdOutput = runtime_log;
        startup_information.hStdError = runtime_log;
        startup_information.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    PROCESS_INFORMATION process_information = {};
    if (!CreateProcessW(engine_path.c_str(), &command[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                        distribution_directory.c_str(), &startup_information, &process_information))
    {
        MessageBoxW(nullptr, L"Speed Dreams could not be started.", L"Open Formula", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    CloseHandle(process_information.hThread);
    CloseHandle(process_information.hProcess);
    if (runtime_log != INVALID_HANDLE_VALUE)
        CloseHandle(runtime_log);
    return EXIT_SUCCESS;
}
