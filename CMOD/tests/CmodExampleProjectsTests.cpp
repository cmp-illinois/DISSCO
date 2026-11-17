// Runs the real cmod program on example projects and checks the results.
// -- Tony Chai, 2026
//
// Usage (ctest passes these): CmodExampleProjectsTests <cmod> <examples-dir>
//
// For every .dissco file in <examples-dir>, each test copies it into a fresh
// temporary folder, runs cmod on it, and checks what came out -- not how cmod
// produced it:
//   - cmod finishes successfully,
//   - it writes exactly one sound file and one .particel file,
//   - the sound file opens, matches the project's channel count and sample
//     rate, isn't silent, and contains no NaN/infinite samples,
//   - composing it again gives identical files (same project + same seed =
//     same music), both
//       - in a folder whose name has spaces, an accent and Chinese
//         characters, as a user's folder or Windows user name might, and
//       - from a copy with Windows (CRLF) line endings, as LASSIE saves
//         projects on Windows.
// Output isn't compared against stored "expected" audio, because cmod's
// random numbers come from the platform's rand(), which differs between
// Windows, macOS and Linux.

#ifdef _WIN32
// Lets the test open sound files whose paths aren't in the Windows code page.
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#endif
#include <sndfile.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

namespace {

std::string currentTest;
int currentTestFailures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED [" << currentTest << "]: " << message << '\n';
        ++currentTestFailures;
    }
}

// Thrown by require() to stop just the current test when the following
// checks depend on this one (e.g. no sound file means nothing to inspect).
struct RequirementFailed : std::runtime_error {
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw RequirementFailed(message);
}

std::string readFile(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

// Value of a simple <Tag>value</Tag> setting in the project file.
std::string projectSetting(const std::string& projectXml, const std::string& tag)
{
    const std::regex pattern("<" + tag + R"(>\s*([^<]*?)\s*</)" + tag + ">");
    std::smatch match;
    return std::regex_search(projectXml, match, pattern) ? match[1].str() : "";
}

// A fresh, empty folder under the system temp directory. With
// unusualName, the folder's name has spaces, an accent and Chinese characters.
fs::path freshFolder(const std::string& label, bool unusualName = false)
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path name = unusualName ? fs::path(u8"dissco cmod test Zoë 音乐 ")
                                : fs::path("dissco-cmod-test-");
    name += label + "-" + std::to_string(stamp);
    fs::path folder = fs::temp_directory_path() / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

// Runs cmod on the project the same way LASSIE does, with the project's
// folder as the working directory. Returns cmod's exit code.
int runCmod(const fs::path& cmod, const fs::path& project, const fs::path& log)
{
    const fs::path previous = fs::current_path();
    fs::current_path(project.parent_path());
    // Bug fix: cmod asks "how many times you want to run" and waits for the
    // keyboard. Run from a terminal, ctest passed the keyboard through, so cmod
    // waited until the 300 s timeout. It now gets an empty input, answers
    // nothing, and runs once, as it does on GitHub. -- Tony Chai, 2026
#ifdef _WIN32
    // Built from wide strings, so paths outside the Windows code page reach
    // cmod as they are (a narrow string would turn them into "?").
    std::wstring command = L"\"" + cmod.wstring() + L"\" \"" + project.wstring()
        + L"\" > \"" + log.wstring() + L"\" 2>&1 < NUL";
    command = L"\"" + command + L"\""; // cmd.exe strips one outer pair of quotes
    const int status = _wsystem(command.c_str());
#else
    const std::string command = "\"" + cmod.string() + "\" \"" + project.string()
        + "\" > \"" + log.string() + "\" 2>&1 < /dev/null";
    const int status = std::system(command.c_str());
#endif
    fs::current_path(previous);
#ifdef _WIN32
    return status;
#else
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

struct CmodOutput {
    fs::path folder;
    fs::path soundFile;
    fs::path particelFile;
};

// Copies a project, giving the copy Windows (CRLF) line endings.
void copyWithWindowsLineEndings(const fs::path& from, const fs::path& to)
{
    std::string text = readFile(from);
    std::string converted;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r'))
            converted += '\r';
        converted += text[i];
    }
    std::ofstream(to, std::ios::binary) << converted;
}

// Copies the project into a fresh folder, runs cmod, and checks that it
// finished and wrote exactly one sound file and one particel.
CmodOutput composeInFreshFolder(const fs::path& cmod, const fs::path& original,
                                const std::string& label, bool unusualFolderName = false,
                                bool windowsLineEndings = false)
{
    const fs::path folder = freshFolder(original.stem().string() + "-" + label,
                                        unusualFolderName);
    const fs::path project = folder / original.filename();
    if (windowsLineEndings)
        copyWithWindowsLineEndings(original, project);
    else
        fs::copy_file(original, project);
    const fs::path log = folder / "cmod-output.log";

    const int exitCode = runCmod(cmod, project, log);
    if (exitCode != 0) {
        std::string output = readFile(log);
        if (output.size() > 1500)
            output = "..." + output.substr(output.size() - 1500);
        require(false, "cmod exited with code " + std::to_string(exitCode)
                           + " (run " + label + "). Its output ended with:\n" + output);
    }

    CmodOutput result;
    result.folder = folder;
    int soundFiles = 0;
    if (fs::exists(folder / "SoundFiles")) {
        for (const auto& entry : fs::directory_iterator(folder / "SoundFiles")) {
            ++soundFiles;
            result.soundFile = entry.path();
        }
    }
    require(soundFiles == 1, "expected 1 sound file in SoundFiles/ (run "
                                 + label + "), found " + std::to_string(soundFiles));
    result.particelFile = folder / (original.stem().string() + ".particel");
    require(fs::exists(result.particelFile),
            "no " + result.particelFile.filename().string() + " was written (run "
                + label + ")");
    return result;
}

void checkSoundFile(const fs::path& soundFile, const std::string& projectXml)
{
    SF_INFO info{};
#ifdef _WIN32
    SNDFILE* sound = sf_wchar_open(soundFile.c_str(), SFM_READ, &info);
#else
    SNDFILE* sound = sf_open(soundFile.c_str(), SFM_READ, &info);
#endif
    require(sound != nullptr, "could not open the sound file: "
                                  + std::string(sf_strerror(nullptr)));

    const std::string channels = projectSetting(projectXml, "NumberOfChannels");
    const std::string rate = projectSetting(projectXml, "SampleRate");
    expect(std::to_string(info.channels) == channels,
           "sound file has " + std::to_string(info.channels)
               + " channels, project asks for " + channels);
    expect(std::to_string(info.samplerate) == rate,
           "sound file sample rate is " + std::to_string(info.samplerate)
               + " Hz, project asks for " + rate + " Hz");
    expect(info.frames > 0, "sound file contains no audio");

    double peak = 0.0;
    long long badSamples = 0;
    std::vector<double> buffer(4096 * static_cast<size_t>(std::max(1, info.channels)));
    sf_count_t framesRead;
    while ((framesRead = sf_readf_double(sound, buffer.data(), 4096)) > 0) {
        for (sf_count_t i = 0; i < framesRead * info.channels; ++i) {
            if (!std::isfinite(buffer[i]))
                ++badSamples;
            else
                peak = std::max(peak, std::abs(buffer[i]));
        }
    }
    sf_close(sound);

    expect(badSamples == 0, "sound contains " + std::to_string(badSamples)
                                + " NaN/infinite samples");
    expect(peak > 1e-4, "sound is silent (peak level " + std::to_string(peak) + ")");
}

// Composes the project again, in the way `how` describes, and checks that
// the music is identical to the first run's. A failure here is reported but
// doesn't stop the other runs, since they don't depend on this one.
void expectSameMusicAgain(const fs::path& cmod, const fs::path& original,
                          const CmodOutput& first, const std::string& how,
                          bool unusualFolderName, bool windowsLineEndings)
{
    try {
        const CmodOutput again = composeInFreshFolder(cmod, original, how,
                                                      unusualFolderName, windowsLineEndings);
        expect(readFile(first.soundFile) == readFile(again.soundFile),
               "composing the same project with the same seed " + how
                   + " gave different audio");
        expect(readFile(first.particelFile) == readFile(again.particelFile),
               "composing the same project with the same seed " + how
                   + " gave a different particel");
        fs::remove_all(again.folder);
    } catch (const RequirementFailed& e) {
        expect(false, e.what());
    }
}

void testExampleProject(const fs::path& cmod, const fs::path& original)
{
    const std::string projectXml = readFile(original);
    const CmodOutput first = composeInFreshFolder(cmod, original, "first");
    checkSoundFile(first.soundFile, projectXml);

    // Where the project is kept, and which line endings its file has, must
    // not change the music.
    expectSameMusicAgain(cmod, original, first,
                         "in a folder named with spaces, accents and Chinese", true, false);
    expectSameMusicAgain(cmod, original, first,
                         "from a copy with Windows (CRLF) line endings", false, true);

    fs::remove_all(first.folder);
}

// Runs one test and reports it by name; a failure stops only that test.
void runTest(const std::string& name, const fs::path& cmod, const fs::path& project,
             std::vector<std::string>& failedTests)
{
    currentTest = name;
    currentTestFailures = 0;
    std::cerr << "RUN  " << name << '\n';
    try {
        testExampleProject(cmod, project);
    } catch (const RequirementFailed& e) {
        std::cerr << "FAILED [" << name << "]: " << e.what()
                  << " (stopped this test; later checks depend on it)\n";
        ++currentTestFailures;
    } catch (const std::exception& e) {
        std::cerr << "FAILED [" << name << "]: unexpected exception: " << e.what() << '\n';
        ++currentTestFailures;
    }
    if (currentTestFailures > 0)
        failedTests.push_back(name);
    else
        std::cerr << "OK   " << name << '\n';
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "usage: CmodExampleProjectsTests <cmod> <examples-dir>\n";
        return 2;
    }
    const fs::path cmod = fs::absolute(argv[1]);
    const fs::path examples = fs::absolute(argv[2]);

    std::vector<fs::path> projects;
    for (const auto& entry : fs::directory_iterator(examples))
        if (entry.path().extension() == ".dissco")
            projects.push_back(entry.path());
    std::sort(projects.begin(), projects.end());
    if (projects.empty()) {
        std::cerr << "FAILED: no .dissco files found in " << examples << '\n';
        return 1;
    }

    std::vector<std::string> failedTests;
    for (const fs::path& project : projects)
        runTest("compose(" + project.filename().string() + ")", cmod, project, failedTests);

    if (!failedTests.empty()) {
        std::cerr << '\n' << failedTests.size() << " CMOD example test(s) failed:\n";
        for (const std::string& name : failedTests)
            std::cerr << "  " << name << '\n';
        return 1;
    }
    std::cerr << "\nAll CMOD example tests passed (" << projects.size() << " projects)\n";
    return 0;
}
