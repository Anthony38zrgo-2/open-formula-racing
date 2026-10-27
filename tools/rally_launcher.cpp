#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR lpCmdLine, int)
{
    std::vector<wchar_t> executable_path(32768);
    const DWORD executable_path_length = GetModuleFileNameW(nullptr, executable_path.data(), static_cast<DWORD>(executable_path.size()));
    if (!executable_path_length || executable_path_length == executable_path.size())
        return EXIT_FAILURE;

    const std::filesystem::path distribution_directory = std::filesystem::path(executable_path.data()).parent_path();
    const std::filesystem::path engine_path = distribution_directory / L"bin" / L"speed-dreams-2.exe";
    const std::filesystem::path data_directory = distribution_directory / L"share" / L"games" / L"speed-dreams-2";
    const std::filesystem::path libraries_directory = distribution_directory / L"lib" / L"games" / L"speed-dreams-2";
    const std::filesystem::path settings_directory = distribution_directory / L"user-settings-rally";
    const std::filesystem::path initial_settings_directory = distribution_directory / L"initial-settings";

    if (!std::filesystem::exists(engine_path) ||
        !std::filesystem::exists(data_directory / L"tracks" / L"circuit" / L"russia-stage1" / L"russia-stage1.xml") ||
        !std::filesystem::exists(data_directory / L"cars" / L"models" / L"ford-escort-rs2000" / L"ford-escort-rs2000.xml"))
    {
        MessageBoxW(nullptr, L"The Rally distribution is incomplete. Please check track and car assets.", L"Rally Trophy - Speed Dreams", MB_OK | MB_ICONERROR);
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

    // Asegurar que rust_physics.xml apunta a ford-escort-rs2000
    const std::filesystem::path rust_physics_cfg = settings_directory / L"config" / L"rust_physics.xml";
    std::ofstream rpf(rust_physics_cfg, std::ios::trunc);
    if (rpf.is_open())
    {
        rpf << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            << "<params name=\"Open Formula Rust physics\">\n"
            << "  <section name=\"RustPhysics\">\n"
            << "    <attstr name=\"library\" val=\"modules/simu/vehicle_physics_engine.dll\" />\n"
            << "    <attstr name=\"profile\" val=\"cars/rust/ford-escort-rs2000/vehicle.json\" />\n"
            << "    <attnum name=\"diagnostic telemetry\" val=\"0\" />\n"
            << "    <attnum name=\"diagnostic interval seconds\" val=\"0.04\" />\n"
            << "    <attnum name=\"diagnostic trace\" val=\"0\" />\n"
            << "    <attstr name=\"diagnostic trace directory\" val=\"rustphysics/\" />\n"
            << "    <attnum name=\"integration scenario\" val=\"0\" />\n"
            << "  </section>\n"
            << "</params>\n";
        rpf.close();
    }

    // Asegurar que human.xml usa el ford-escort-rs2000
    const std::filesystem::path human_cfg = settings_directory / L"drivers" / L"human" / L"human.xml";
    std::ofstream hf(human_cfg, std::ios::trunc);
    if (hf.is_open())
    {
        hf << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<!DOCTYPE params SYSTEM \"params.dtd\">\n"
           << "<params name=\"human\" version=\"1.1\">\n"
           << "  <section name=\"Robots\">\n"
           << "    <section name=\"index\">\n"
           << "      <section name=\"1\">\n"
           << "        <attstr name=\"name\" val=\"Rally Driver\"/>\n"
           << "        <attstr name=\"short name\" val=\"Driver\"/>\n"
           << "        <attstr name=\"code name\" val=\"RAL\"/>\n"
           << "        <attstr name=\"car name\" val=\"ford-escort-rs2000\"/>\n"
           << "        <attnum name=\"race number\" val=\"1\"/>\n"
           << "        <attnum name=\"red\" val=\"0.2\"/>\n"
           << "        <attnum name=\"green\" val=\"0.4\"/>\n"
           << "        <attnum name=\"blue\" val=\"0.9\"/>\n"
           << "        <attstr name=\"type\" val=\"human\"/>\n"
           << "        <attstr name=\"skill level\" val=\"pro\"/>\n"
           << "        <attstr name=\"nation\" val=\"GB\"/>\n"
           << "      </section>\n"
           << "    </section>\n"
           << "  </section>\n"
           << "</params>\n";
        hf.close();
    }

    // Copiar racemanager si no existe
    const std::filesystem::path raceman_dst = settings_directory / L"config" / L"raceman" / L"rally-russia-stage1.xml";
    const std::filesystem::path raceman_src = data_directory / L"config" / L"raceman" / L"rally-russia-stage1.xml";
    if (std::filesystem::exists(raceman_src) && !std::filesystem::exists(raceman_dst))
    {
        std::filesystem::create_directories(raceman_dst.parent_path());
        std::filesystem::copy_file(raceman_src, raceman_dst);
    }

    const std::wstring binaries_directory = (distribution_directory / L"bin").wstring();
    const DWORD environment_length = GetEnvironmentVariableW(L"PATH", nullptr, 0);
    std::vector<wchar_t> environment_path(environment_length + 1);
    GetEnvironmentVariableW(L"PATH", environment_path.data(), static_cast<DWORD>(environment_path.size()));
    SetEnvironmentVariableW(L"PATH", (binaries_directory + L";" + environment_path.data()).c_str());
    SetEnvironmentVariableW(L"OSG_LIBRARY_PATH", (distribution_directory / L"bin" / L"osgPlugins-3.6.5").c_str());

    std::wstring command = L"\"" + engine_path.wstring() + L"\" --datadir \"" + data_directory.wstring()
        + L"\" --libdir \"" + libraries_directory.wstring() + L"\" --bindir \"" + binaries_directory
        + L"\" --localdir \"" + settings_directory.wstring() + L"\" --startrace rally-russia-stage1";

    if (lpCmdLine && *lpCmdLine)
    {
        command += L" ";
        command += lpCmdLine;
    }

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
        MessageBoxW(nullptr, L"Speed Dreams could not be started for Rally.", L"Rally Trophy - Speed Dreams", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    CloseHandle(process_information.hThread);
    CloseHandle(process_information.hProcess);
    if (runtime_log != INVALID_HANDLE_VALUE)
        CloseHandle(runtime_log);

    return EXIT_SUCCESS;
}
