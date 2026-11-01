// Tests for the CMOD functions that read from the event being built:
// CURRENT_CHILD_NUM, CURRENT_TYPE, CURRENT_LAYER, PREVIOUS_CHILD_DURATION,
// AVAILABLE_EDU, CURRENT_PARTIAL_NUM, and GetPattern (which remembers its
// place between children). -- Tony Chai, 2026
//
// Usage (ctest passes this): CmodEventFunctionsTests <cmod>
//
// These functions only have a value while a piece is being composed, so each
// test writes a small project that uses one of them in a child's Start Time,
// Type, Duration, frequency or partial deviation, runs the real cmod program
// on it, and checks the numbers cmod reports in the project's .particel file
// (the start, duration and name of every child, and the frequencies of every
// sound). Sound synthesis is turned off to keep the runs fast; the particel
// is written either way.

#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

namespace {

std::string currentTest;
int currentTestFailures = 0;
fs::path cmodProgram;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED [" << currentTest << "]: " << message << '\n';
        ++currentTestFailures;
    }
}

// Thrown by require() to stop just the current test when the following
// checks depend on this one (e.g. cmod failed, so there's no particel).
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

// ---------------------------------------------------------------------------
// Writing test projects

// <Fun><Name>name</Name><Tag>value</Tag>...</Fun>, as LASSIE writes it.
std::string fun(const std::string& name,
                const std::vector<std::pair<std::string, std::string>>& fields = {})
{
    std::string xml = "<Fun><Name>" + name + "</Name>";
    for (const auto& [tag, value] : fields)
        xml += "<" + tag + ">" + value + "</" + tag + ">";
    return xml + "</Fun>";
}

const std::string childNum = fun("CURRENT_CHILD_NUM");

// One choice in a parent's Layers list. Its position across all layers
// (counting from 0) is the number the parent's Type field picks it with.
struct Choice {
    std::string name;
    int eventType; // 1 High, 2 Mid, 3 Low, 4 Bottom
};

// A Top/High/Mid/Low/Bottom event and how it builds its children.
// Units: 0 = fraction of the parent, 1 = EDU, 2 = seconds.
struct EventSpec {
    int eventType = 0;
    std::string name = "0";
    std::string children = "1";       // Number of Children to Create
    std::string method = "0";         // 0 Continuum, 1 Sweep, 2 Discrete
    std::string start = "0";
    std::string type = "0";
    std::string duration = "1";
    std::string startUnit = "2";
    std::string durationUnit = "2";
    std::vector<std::vector<Choice>> layers;
    std::string frequency = "440";    // Bottom only, in Hz
};

std::string eventXml(const EventSpec& event)
{
    std::string layers;
    for (const auto& layer : event.layers) {
        layers += "<Layer><ByLayer/><DiscretePackages>";
        for (const Choice& choice : layer) {
            layers += "<Package><EventName>" + choice.name + "</EventName><EventType>"
                + std::to_string(choice.eventType) + "</EventType><Weight>1</Weight>"
                "<AttackEnvelope/><AttackEnvelopeScale>1</AttackEnvelopeScale>"
                "<DurationEnvelope/><DurationEnvelopeScale>1</DurationEnvelopeScale></Package>";
        }
        layers += "</DiscretePackages></Layer>";
    }

    std::string xml = "<Event orderInPalette=\"\"><EventType>" + std::to_string(event.eventType)
        + "</EventType><Name>" + event.name + "</Name>"
        "<MaxChildDuration>1000</MaxChildDuration><EDUPerBeat>60</EDUPerBeat>"
        "<TimeSignature><Entry1>4</Entry1><Entry2>4</Entry2></TimeSignature>"
        "<Tempo><MethodFlag>0</MethodFlag><Prefix>0</Prefix><NoteValue>2</NoteValue>"
        "<FractionEntry1/><FractionEntry2/><ValueEntry>60</ValueEntry></Tempo>"
        "<NumberOfChildren><MethodFlag>0</MethodFlag><Entry1>" + event.children
        + "</Entry1><Entry2/><Entry3/></NumberOfChildren>"
        "<ChildEventDefinition><Entry1>" + event.start + "</Entry1><Entry2>" + event.type
        + "</Entry2><Entry3>" + event.duration + "</Entry3><AttackSieve/><DurationSieve/>"
        "<DefinitionFlag>" + event.method + "</DefinitionFlag><StartTypeFlag>" + event.startUnit
        + "</StartTypeFlag><DurationTypeFlag>" + event.durationUnit + "</DurationTypeFlag>"
        "</ChildEventDefinition><Layers>" + layers + "</Layers>"
        "<Spatialization/><Reverb/><Filter/>";
    if (event.eventType == 4) {
        xml += "<ExtraInfo><FrequencyInfo><FrequencyFlag>2</FrequencyFlag>"
               "<FrequencyContinuumFlag>0</FrequencyContinuumFlag><FrequencyEntry1>"
            + event.frequency + "</FrequencyEntry1><FrequencyEntry2/></FrequencyInfo>"
               "<Loudness>45</Loudness><Phase>0</Phase><Spatialization>"
            + fun("SPA", {{"Method", "STEREO"}, {"Apply", "SOUND"},
                          {"Channels", "<Partials><P>" + fun("EnvLib", {{"Env", "2"}, {"Scale", "0.5"}})
                                           + "</P></Partials>"}})
            + "</Spatialization><Reverb/><Filter/>"
               "<ModifierUsage samplingScope=\"per-sound\" version=\"1\"/><Modifiers/></ExtraInfo>";
    } else {
        xml += "<Modifiers/>";
    }
    return xml + "</Event>";
}

// A Bottom event that makes `sounds` sounds, each lasting the whole Bottom,
// all using the spectrum "sp1".
EventSpec soundEvent(const std::string& name, const std::string& sounds = "1",
                     const std::string& frequency = "440")
{
    EventSpec bottom;
    bottom.eventType = 4;
    bottom.name = name;
    bottom.children = sounds;
    bottom.startUnit = "0";
    bottom.durationUnit = "0";
    bottom.layers = {{{"sp1", 5}}};
    bottom.frequency = frequency;
    return bottom;
}

// The spectrum "sp1": `partials` partials, each with the same loudness
// envelope, and the given partial Deviation.
std::string spectrumXml(int partials, const std::string& deviation)
{
    std::string spectrum;
    for (int i = 0; i < partials; ++i)
        spectrum += "<Partial>" + fun("EnvLib", {{"Env", "1"}, {"Scale", "1"}}) + "</Partial>";
    return "<Event orderInPalette=\"0\"><EventType>5</EventType><Name>sp1</Name>"
           "<NumberOfPartials>" + std::to_string(partials) + "</NumberOfPartials>"
           "<Deviation>" + deviation + "</Deviation><GenerateSpectrum/>"
           "<Spectrum>" + spectrum + "</Spectrum></Event>";
}

// A 30-second piece (1800 EDU: 60 BPM, 60 EDU per beat) whose top event is
// named "0", with synthesis off and the particel on.
std::string projectXml(const std::vector<EventSpec>& events, int partials = 1,
                       const std::string& deviation = "0")
{
    std::string xml =
        "<?xml version='1.0' encoding='UTF-8'?><ProjectRoot><ProjectConfiguration>"
        "<Title>event_functions</Title><FileFlag>THMLBsnv</FileFlag><TopEvent>0</TopEvent>"
        "<PieceStartTime>0</PieceStartTime><Duration>30</Duration><Synthesis>False</Synthesis>"
        "<Score>False</Score><GrandStaff>False</GrandStaff><NumberOfStaff>1</NumberOfStaff>"
        "<NumberOfChannels>2</NumberOfChannels><SampleRate>44100</SampleRate>"
        "<SampleSize>24</SampleSize><NumberOfThreads>1</NumberOfThreads>"
        "<OutputParticel>True</OutputParticel><Seed>event-functions</Seed>"
        "</ProjectConfiguration><NoteModifiers><DefaultModifiers/><CustomModifiers/></NoteModifiers>"
        "<EnvelopeLibrary>\n2\nEnvelope 1\n3\n"
        "0.000     0.000     LINEAR              FLEXIBLE    0.500\n"
        "0.500     1.000     LINEAR              FLEXIBLE    0.500\n"
        "1.000     0.000\n"
        "Envelope 2\n2\n"
        "0.000     1.000     LINEAR              FLEXIBLE    1.000\n"
        "1.000     1.000\n</EnvelopeLibrary><MarkovModelLibrary>\n0\n</MarkovModelLibrary><Events>";
    for (const EventSpec& event : events)
        xml += eventXml(event);
    return xml + spectrumXml(partials, deviation) + "</Events></ProjectRoot>";
}

// ---------------------------------------------------------------------------
// Reading cmod's report (.particel)

// One section of the particel, e.g. an event, a "Continuum" step that placed
// a child, or a "Sound". Lines look like:
//   +-Name             starts a section inside the current one
//   | . Key: Value     a property of the current section
//   | End Name         ends the current section
struct ReportNode {
    std::string name;
    std::vector<std::pair<std::string, std::string>> properties;
    std::vector<ReportNode> sections;

    // The property's value ("" if missing), including units, e.g. "2.00 sec.".
    std::string property(const std::string& key) const
    {
        for (const auto& [k, v] : properties)
            if (k == key)
                return v;
        return "";
    }

    const ReportNode* section(const std::string& sectionName) const
    {
        for (const ReportNode& s : sections)
            if (s.name == sectionName)
                return &s;
        return nullptr;
    }
};

ReportNode parseParticel(const std::string& text)
{
    ReportNode root;
    std::vector<ReportNode*> open; // the sections we're inside, innermost last
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        std::size_t pos = 0;
        bool startsSection = false;
        while (line.compare(pos, 2, "| ") == 0 || line.compare(pos, 2, "+-") == 0) {
            startsSection = line.compare(pos, 2, "+-") == 0;
            pos += 2;
        }
        const std::string content = line.substr(pos);
        if (content.empty())
            continue;
        if (startsSection || open.empty()) {
            ReportNode& parent = open.empty() ? root : *open.back();
            parent.sections.push_back(ReportNode{content, {}, {}});
            open.push_back(&parent.sections.back());
        } else if (content.rfind(". ", 0) == 0) {
            const std::size_t colon = content.find(": ");
            if (colon != std::string::npos)
                open.back()->properties.emplace_back(content.substr(2, colon - 2),
                                                     content.substr(colon + 2));
        } else if (content.rfind("End ", 0) == 0) {
            open.pop_back();
        }
    }
    return root;
}

// Every section named `name`, anywhere in the report, in order.
void findAll(const ReportNode& node, const std::string& name,
             std::vector<const ReportNode*>& found)
{
    for (const ReportNode& s : node.sections) {
        if (s.name == name)
            found.push_back(&s);
        findAll(s, name, found);
    }
}

std::vector<const ReportNode*> findAll(const ReportNode& report, const std::string& name)
{
    std::vector<const ReportNode*> found;
    findAll(report, name, found);
    return found;
}

// The number at the start of a value like "2.00 sec." (NaN if none).
double number(const std::string& value)
{
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        return std::nan("");
    }
}

// How a parent placed one child, as reported in its Continuum/Sweep section.
struct Placement {
    std::string name;
    double start = 0;            // as entered, in the parent's Start Time unit
    double duration = 0;         // as entered, in the parent's Duration unit
    double startSeconds = 0;
    double durationSeconds = 0;
    std::string startEDU;        // e.g. "450", or "indeterminate"
    std::string durationEDU;
};

std::vector<Placement> placements(const ReportNode& parent)
{
    std::vector<Placement> result;
    for (const ReportNode& step : parent.sections) {
        if (step.name != "Continuum" && step.name != "Sweep" && step.name != "Discrete")
            continue;
        Placement p;
        p.name = step.property("Name");
        if (const ReportNode* entered = step.section("Parameters")) {
            p.start = number(entered->property("Start"));
            p.duration = number(entered->property("Duration"));
        }
        if (const ReportNode* seconds = step.section("Seconds")) {
            p.startSeconds = number(seconds->property("Start"));
            p.durationSeconds = number(seconds->property("Duration"));
        }
        if (const ReportNode* edu = step.section("EDU")) {
            p.startEDU = edu->property("Start");
            p.durationEDU = edu->property("Duration");
        }
        result.push_back(p);
    }
    return result;
}

std::string listText(const std::vector<double>& values)
{
    std::ostringstream out;
    out << '{';
    for (std::size_t i = 0; i < values.size(); ++i)
        out << (i ? ", " : "") << values[i];
    return out.str() + '}';
}

std::string listText(const std::vector<std::string>& values)
{
    std::string out = "{";
    for (std::size_t i = 0; i < values.size(); ++i)
        out += (i ? ", " : "") + values[i];
    return out + "}";
}

// Checks a list of reported numbers against what's expected. The particel
// rounds to 2 decimals, so values only need to agree to within 0.005.
void expectValues(const std::vector<double>& actual, const std::vector<double>& expected,
                  const std::string& what)
{
    bool same = actual.size() == expected.size();
    for (std::size_t i = 0; same && i < actual.size(); ++i)
        same = std::abs(actual[i] - expected[i]) < 0.006;
    expect(same, what + ": expected " + listText(expected) + ", got " + listText(actual));
}

template <typename Field>
std::vector<double> column(const std::vector<Placement>& list, Field field)
{
    std::vector<double> values;
    for (const Placement& p : list)
        values.push_back(p.*field);
    return values;
}

std::vector<std::string> names(const std::vector<Placement>& list)
{
    std::vector<std::string> values;
    for (const Placement& p : list)
        values.push_back(p.name);
    return values;
}

// ---------------------------------------------------------------------------
// Running cmod

// A fresh, empty folder under the system temp directory.
fs::path freshFolder(const std::string& label)
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path folder = fs::temp_directory_path()
        / ("dissco-cmod-event-test-" + label + "-" + std::to_string(stamp));
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

// Writes the project into a fresh folder, runs cmod on it there (the way
// LASSIE does), and returns the parsed particel. Stops the test if cmod
// fails or writes no particel.
ReportNode compose(const std::string& project)
{
    std::string label;
    for (char c : currentTest)
        label += std::isalnum(static_cast<unsigned char>(c)) ? c : '-';
    const fs::path folder = freshFolder(label);
    const fs::path projectFile = folder / "event_functions.dissco";
    std::ofstream(projectFile, std::ios::binary) << project;
    const fs::path log = folder / "cmod-output.log";

    const fs::path previous = fs::current_path();
    fs::current_path(folder);
    std::string command = "\"" + cmodProgram.string() + "\" \"" + projectFile.string()
        + "\" > \"" + log.string() + "\" 2>&1";
#ifdef _WIN32
    command = "\"" + command + "\""; // cmd.exe strips one outer pair of quotes
    const int exitCode = std::system(command.c_str());
#else
    const int status = std::system(command.c_str());
    const int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
    fs::current_path(previous);

    if (exitCode != 0) {
        std::string output = readFile(log);
        if (output.size() > 1500)
            output = "..." + output.substr(output.size() - 1500);
        fs::remove_all(folder);
        require(false, "cmod exited with code " + std::to_string(exitCode)
                           + ". Its output ended with:\n" + output);
    }
    const fs::path particel = folder / "event_functions.particel";
    require(fs::exists(particel), "cmod wrote no particel");
    ReportNode report = parseParticel(readFile(particel));
    fs::remove_all(folder);
    return report;
}

// The children placed by the first event named `name`. Stops the test if
// that event isn't in the report.
std::vector<Placement> placementsOf(const ReportNode& report, const std::string& name)
{
    const auto events = findAll(report, name);
    require(!events.empty(), "event '" + name + "' isn't in the particel");
    return placements(*events.front());
}

// ---------------------------------------------------------------------------
// Tests

// CURRENT_CHILD_NUM is the number of the child being built: 0, 1, 2, ...
void testCurrentChildNum()
{
    // In a parent's Start Time: children start at 0, 2, 4, 6 seconds.
    EventSpec top;
    top.children = "4";
    top.start = childNum + " * 2";
    top.layers = {{{"s1", 4}}};
    const auto placed = placementsOf(compose(projectXml({top, soundEvent("s1")})), "0");
    expectValues(column(placed, &Placement::startSeconds), {0, 2, 4, 6},
                 "Start Time = CURRENT_CHILD_NUM * 2, child starts in seconds");

    // In a Bottom's frequency, it counts the sounds that Bottom makes.
    EventSpec one;
    one.layers = {{{"s1", 4}}};
    const ReportNode report =
        compose(projectXml({one, soundEvent("s1", "3", "440 + 10 * " + childNum)}));
    std::vector<double> frequencies;
    for (const ReportNode* sound : findAll(report, "Sound"))
        frequencies.push_back(number(sound->property("Base Frequency")));
    expectValues(frequencies, {440, 450, 460},
                 "Bottom frequency = 440 + 10 * CURRENT_CHILD_NUM, base frequencies of its 3 sounds");
}

// CURRENT_TYPE is the Type just chosen for the child being built, so a
// Duration can depend on which event was picked.
void testCurrentType()
{
    EventSpec top;
    top.children = "4";
    top.type = fun("Select", {{"List", "0, 1, 1, 0"}, {"Index", childNum}});
    top.duration = fun("CURRENT_TYPE") + " + 1";
    top.layers = {{{"sA", 4}, {"sB", 4}}};
    const auto placed =
        placementsOf(compose(projectXml({top, soundEvent("sA"), soundEvent("sB")})), "0");
    expect(names(placed) == std::vector<std::string>{"sA", "sB", "sB", "sA"},
           "Type = Select 0, 1, 1, 0: expected children {sA, sB, sB, sA}, got "
               + listText(names(placed)));
    expectValues(column(placed, &Placement::durationSeconds), {1, 2, 2, 1},
                 "Duration = CURRENT_TYPE + 1, child durations in seconds");
}

// CURRENT_LAYER is the layer (0, 1, ...) holding the child just chosen.
void testCurrentLayer()
{
    EventSpec top;
    top.children = "3";
    top.type = childNum;
    top.duration = fun("CURRENT_LAYER") + " + 1";
    top.layers = {{{"sA", 4}}, {{"sB", 4}, {"sC", 4}}};
    const auto placed = placementsOf(
        compose(projectXml({top, soundEvent("sA"), soundEvent("sB"), soundEvent("sC")})), "0");
    expect(names(placed) == std::vector<std::string>{"sA", "sB", "sC"},
           "Type = CURRENT_CHILD_NUM over layers {sA} {sB, sC}: expected children {sA, sB, sC}, got "
               + listText(names(placed)));
    expectValues(column(placed, &Placement::durationSeconds), {1, 2, 2},
                 "Duration = CURRENT_LAYER + 1 with sA in layer 0 and sB, sC in layer 1");
}

// PREVIOUS_CHILD_DURATION is where the previous child ended (0 for the
// first one), so using it as the Start Time puts children back to back.
void testPreviousChildDuration()
{
    // Continuum reads it from the Start Time field.
    EventSpec top;
    top.children = "4";
    top.start = fun("PREVIOUS_CHILD_DURATION");
    top.duration = childNum + " + 1";
    top.layers = {{{"s1", 4}}};
    auto placed = placementsOf(compose(projectXml({top, soundEvent("s1")})), "0");
    expectValues(column(placed, &Placement::startSeconds), {0, 1, 3, 6},
                 "Continuum, Start Time = PREVIOUS_CHILD_DURATION, durations 1-4: child starts");

    // Sweep, the way the tutorial sets it up, gives the same placement.
    top.method = "1";
    placed = placementsOf(compose(projectXml({top, soundEvent("s1")})), "0");
    expectValues(column(placed, &Placement::startSeconds), {0, 1, 3, 6},
                 "Sweep, Start Time = PREVIOUS_CHILD_DURATION, durations 1-4: child starts");
    expectValues(column(placed, &Placement::durationSeconds), {1, 2, 3, 4},
                 "Sweep, durations 1-4: child durations");
}

// AVAILABLE_EDU is the parent's length in EDU (here 30 s = 1800 EDU).
void testAvailableEdu()
{
    EventSpec top;
    top.children = "4";
    top.start = childNum + " * " + fun("AVAILABLE_EDU") + " / 4";
    top.duration = fun("AVAILABLE_EDU") + " / 4";
    top.startUnit = "1";
    top.durationUnit = "1";
    top.layers = {{{"s1", 4}}};
    const auto placed = placementsOf(compose(projectXml({top, soundEvent("s1")})), "0");
    expectValues(column(placed, &Placement::start), {0, 450, 900, 1350},
                 "Start Time = CURRENT_CHILD_NUM * AVAILABLE_EDU / 4, in EDU");
    expectValues(column(placed, &Placement::duration), {450, 450, 450, 450},
                 "Duration = AVAILABLE_EDU / 4, in EDU");
    expectValues(column(placed, &Placement::startSeconds), {0, 7.5, 15, 22.5},
                 "quarters of a 30-second parent: child starts in seconds");
}

// The frequencies of a single 440 Hz sound's partials ("Fundamental",
// "Partial 1", ...), when the spectrum's partial Deviation is `deviation`.
std::vector<double> partialFrequencies(int partials, const std::string& deviation)
{
    EventSpec one;
    one.layers = {{{"s1", 4}}};
    const ReportNode report = compose(projectXml({one, soundEvent("s1")}, partials, deviation));
    const auto sounds = findAll(report, "Sound");
    require(sounds.size() == 1, "expected 1 sound, found " + std::to_string(sounds.size()));
    const ReportNode* section = sounds.front()->section("Partials");
    require(section != nullptr, "the sound has no Partials section");

    std::vector<double> frequencies{number(section->property("Fundamental"))};
    for (int i = 1; i < partials; ++i)
        frequencies.push_back(number(section->property("Partial " + std::to_string(i))));
    return frequencies;
}

// Checks partial `i` (of 440 Hz) moved off its harmonic by at most
// `deviation` x 440 Hz (the deviation is random, but never exactly 0).
void expectDeviated(const std::vector<double>& frequencies, int i, double deviation,
                    const std::string& what)
{
    const double harmonic = 440.0 * (i + 1);
    const double offBy = std::abs(frequencies[i] - harmonic);
    expect(offBy > 0.005 && offBy <= deviation * 440 + 0.005,
           what + ": expected Partial " + std::to_string(i) + " within "
               + std::to_string(deviation * 440) + " Hz of " + std::to_string(harmonic)
               + " but not on it, got " + std::to_string(frequencies[i]));
}

// CURRENT_PARTIAL_NUM is the partial being built (0 = fundamental). Here the
// partial Deviation is 0.1 x (partial - 1): 0 for partial 1, so it sits
// exactly on its harmonic, and 0.1, 0.2 for partials 2, 3, which move off.
void testCurrentPartialNum()
{
    const auto frequencies =
        partialFrequencies(4, fun("CURRENT_PARTIAL_NUM") + " * 0.1 - 0.1");
    require(frequencies.size() == 4, "expected 4 partials");
    expectValues({frequencies[0], frequencies[1]}, {440, 880},
                 "Deviation 0.1 x (partial - 1): Fundamental and Partial 1 in Hz");
    expectDeviated(frequencies, 2, 0.1, "Deviation 0.1 x (partial - 1)");
    expectDeviated(frequencies, 3, 0.2, "Deviation 0.1 x (partial - 1)");
}

// A comparison such as >= can sit next to a function in a field, e.g.
// (CURRENT_PARTIAL_NUM >= 2) * 0.4 (1 when true, 0 when false).
void testComparisonNextToFunction()
{
    const auto frequencies =
        partialFrequencies(4, "(" + fun("CURRENT_PARTIAL_NUM") + " >= 2) * 0.4");
    require(frequencies.size() == 4, "expected 4 partials");
    expectValues({frequencies[0], frequencies[1]}, {440, 880},
                 "Deviation (CURRENT_PARTIAL_NUM >= 2) * 0.4: Fundamental and Partial 1 in Hz");
    expectDeviated(frequencies, 2, 0.4, "Deviation (CURRENT_PARTIAL_NUM >= 2) * 0.4");
    expectDeviated(frequencies, 3, 0.4, "Deviation (CURRENT_PARTIAL_NUM >= 2) * 0.4");
}

// GetPattern remembers where it is: each child gets the next value of the
// pattern, and two different GetPatterns keep their own places.
void testGetPatternRemembers()
{
    const auto getPattern = [](const std::string& origin, const std::string& intervals) {
        return fun("GetPattern", {{"Method", "IN_ORDER"}, {"Offset", origin},
                                  {"Pattern", fun("MakePattern", {{"List", intervals}})}});
    };
    EventSpec top;
    top.children = "6";
    top.start = getPattern("0", "3");        // 0, 3, then repeats
    top.duration = getPattern("1", "1, 2");  // 1, 2, 4, then repeats
    top.layers = {{{"s1", 4}}};
    const auto placed = placementsOf(compose(projectXml({top, soundEvent("s1")})), "0");
    expectValues(column(placed, &Placement::startSeconds), {0, 3, 0, 3, 0, 3},
                 "Start Time = GetPattern from 0, intervals 3: child starts");
    expectValues(column(placed, &Placement::durationSeconds), {1, 2, 4, 1, 2, 4},
                 "Duration = GetPattern from 1, intervals 1, 2: child durations");
}

// Each parent event has its own GetPattern memory: two copies of the same
// High event both start their pattern from the beginning.
void testGetPatternPerParent()
{
    EventSpec top;
    top.children = "2";
    top.start = childNum + " * 10";
    top.duration = "10";
    top.layers = {{{"h", 1}}};

    EventSpec high;
    high.eventType = 1;
    high.name = "h";
    high.children = "3";
    high.duration = fun("GetPattern", {{"Method", "IN_ORDER"}, {"Offset", "1"},
                                       {"Pattern", fun("MakePattern", {{"List", "1, 1, 1"}})}});
    high.layers = {{{"s1", 4}}};

    const ReportNode report = compose(projectXml({top, high, soundEvent("s1")}));
    const auto highs = findAll(report, "h");
    require(highs.size() == 2, "expected 2 'h' events, found " + std::to_string(highs.size()));
    for (std::size_t i = 0; i < highs.size(); ++i)
        expectValues(column(placements(*highs[i]), &Placement::durationSeconds), {1, 2, 3},
                     "'h' number " + std::to_string(i + 1)
                         + ", Duration = GetPattern from 1, intervals 1, 1, 1: child durations");
}

// Runs one test and reports it by name; a failure stops only that test.
void runTest(const std::string& name, void (*test)(), std::vector<std::string>& failedTests)
{
    currentTest = name;
    currentTestFailures = 0;
    std::cerr << "RUN  " << name << '\n';
    try {
        test();
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
    if (argc != 2) {
        std::cerr << "usage: CmodEventFunctionsTests <cmod>\n";
        return 2;
    }
    cmodProgram = fs::absolute(argv[1]);

    std::vector<std::string> failedTests;
    runTest("CURRENT_CHILD_NUM", testCurrentChildNum, failedTests);
    runTest("CURRENT_TYPE", testCurrentType, failedTests);
    runTest("CURRENT_LAYER", testCurrentLayer, failedTests);
    runTest("PREVIOUS_CHILD_DURATION", testPreviousChildDuration, failedTests);
    runTest("AVAILABLE_EDU", testAvailableEdu, failedTests);
    runTest("CURRENT_PARTIAL_NUM", testCurrentPartialNum, failedTests);
    runTest("GetPattern remembers its place", testGetPatternRemembers, failedTests);
    runTest("GetPattern memory is per parent", testGetPatternPerParent, failedTests);
    runTest("comparison next to a function", testComparisonNextToFunction, failedTests);

    if (!failedTests.empty()) {
        std::cerr << '\n' << failedTests.size() << " CMOD event function test(s) failed:\n";
        for (const std::string& name : failedTests)
            std::cerr << "  " << name << '\n';
        return 1;
    }
    std::cerr << "\nAll CMOD event function tests passed\n";
    return 0;
}
