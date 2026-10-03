// SPDX-License-Identifier: GPL-3.0-only
#include "../windows/settings.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
int checks = 0;
void require(bool condition, const char* name) {
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); }
}
bool defaults(const morse::Settings& value) {
    return value.theme == morse::ThemeId::System && value.frequency == 650;
}
}
int main() {
    using namespace morse;
    require(defaults(parseSettings("")), "empty settings use system and 650 Hz");
    for (const auto theme : {ThemeId::System, ThemeId::Light, ThemeId::Dark,
                            ThemeId::Midnight, ThemeId::Amber, ThemeId::Matrix}) {
        for (const auto frequency : {400u, 650u, 1000u}) {
            const auto decoded = parseSettings(serializeSettings({theme, frequency}));
            require(decoded.theme == theme && decoded.frequency == frequency, "all themes and frequency boundaries round trip");
        }
    }
    for (const auto bad : {"", "399", "1001", "-650", "+650", "650.0", "650hz", "99999999999999999999"}) {
        unsigned result = 777;
        require(!parseFrequency(bad, result) && result == 777, "invalid frequency cannot replace previous value");
        require(parseSettings(std::string("[MorseBridge]\nTheme=amber\nFrequency=") + bad).frequency == 650,
                "invalid persisted frequency uses 650 Hz");
    }
    const auto tolerant = parseSettings("\xef\xbb\xbf ; comment\r\n[ morsebridge ]\r\n theme = MATRIX \r\n frequency = 400 \r\nAudio=on\nTopmost=on\nCOM=COM9\n");
    require(tolerant.theme == ThemeId::Matrix && tolerant.frequency == 400, "UTF-8 BOM comments spacing CRLF and case supported");
    require(defaults(parseSettings("Theme=dark\nFrequency=1000\n[Other]\nTheme=matrix\nFrequency=400\n")),
            "foreign and unsectioned settings ignored");
    require(defaults(parseSettings("[MorseBridge]\nTheme=unknown\nFrequency=invalid\n")), "unknown theme and invalid frequency use defaults");
    const auto partial = parseSettings("[MorseBridge]\nTheme=midnight\nFrequency=1000\n[Other]\nFrequency=400\n");
    require(partial.theme == ThemeId::Midnight && partial.frequency == 1000, "other sections do not override application values");
    require(defaults(parseSettings(std::string(16385, 'x'))), "oversized contents are bounded");
    require(defaults(parseSettings(serializeSettings({static_cast<ThemeId>(99), 1}))), "invalid in-memory values are sanitized before saving");
    const auto saved = serializeSettings({ThemeId::Dark, 650});
    require(saved.find("Audio") == std::string::npos && saved.find("COM") == std::string::npos && saved.find("Topmost") == std::string::npos,
            "only appearance and frequency are persisted");
    const auto automaticLight = paletteFor(ThemeId::System, false, false);
    const auto automaticDark = paletteFor(ThemeId::System, true, false);
    require(!automaticLight.dark && automaticDark.dark && automaticLight.background != automaticDark.background,
            "system palette resolves according to supplied system mode");
    require(!paletteFor(ThemeId::Light, true, false).dark && paletteFor(ThemeId::Dark, false, false).dark,
            "explicit themes ignore system preference");
    require(paletteFor(ThemeId::Amber, false, true).text == paletteFor(ThemeId::Matrix, false, true).text,
            "contrast fallback overrides decorative themes");

    const auto fixtureParent = std::filesystem::absolute("build");
    const auto testRoot = fixtureParent /
        ("morse-settings-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto path = testRoot / "nested" / "settings.ini";
    require(defaults(loadSettings(path)), "missing file uses defaults without creating files");
    require(!std::filesystem::exists(testRoot), "reading missing settings has no disk side effects");
    require(saveSettings(path, {ThemeId::Midnight, 400}), "saving creates application settings directory");
    auto loaded = loadSettings(path);
    require(loaded.theme == ThemeId::Midnight && loaded.frequency == 400, "written settings reopen correctly");
    require(saveSettings(path, {ThemeId::Matrix, 1000}), "atomic replacement overwrites an existing settings file");
    loaded = loadSettings(path);
    require(loaded.theme == ThemeId::Matrix && loaded.frequency == 1000, "replacement values survive restart");
    {
        std::ofstream oversized(path, std::ios::binary | std::ios::trunc);
        oversized << std::string(16385, 'x');
    }
    require(defaults(loadSettings(path)), "oversized file read is bounded");
    const auto blocked = testRoot / "blocked";
    { std::ofstream file(blocked); file << "regular file"; }
    require(!saveSettings(blocked / "settings.ini", {ThemeId::Dark, 650}), "unwritable parent fails without throwing");
    require(!saveSettings(std::filesystem::path{}, {}), "empty settings location fails safely");
    require(defaults(loadSettings(std::filesystem::path{})), "empty settings location uses defaults");
    require(std::filesystem::weakly_canonical(testRoot).parent_path() == std::filesystem::weakly_canonical(fixtureParent),
            "fixture cleanup stays inside the build directory");
    std::error_code error;
    std::filesystem::remove_all(testRoot, error);
    require(!error, "temporary persistence fixtures removed");
    std::cout << "PASS: " << checks << " settings checks\n";
}
