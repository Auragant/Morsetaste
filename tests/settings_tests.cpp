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
    return value.theme == morse::ThemeId::System && value.frequency == 650 &&
        value.outputKey == morse::spaceKey && value.customOutputKeys.empty();
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
            "session-only audio COM and topmost states are not persisted");
    require(availableOutputKeys({}) == std::vector<unsigned>({spaceKey,leftCtrlKey}),
            "first-run dropdown contains exactly SPACE and Left CTRL");
    const auto legacy = parseSettings("[MorseBridge]\nTheme=dark\nFrequency=700\n");
    require(legacy.theme == ThemeId::Dark && legacy.frequency == 700 && legacy.outputKey == spaceKey &&
            legacy.customOutputKeys.empty(), "old INI retains theme and frequency and defaults output to SPACE");
    Settings custom;
    require(addCustomOutputKey(custom,0x70) && addCustomOutputKey(custom,0xA3) && addCustomOutputKey(custom,0x41),
            "function modifier and letter keys can be added in user order");
    require(!addCustomOutputKey(custom,0x70) && !addCustomOutputKey(custom,spaceKey) &&
            !addCustomOutputKey(custom,leftCtrlKey) && !addCustomOutputKey(custom,0),
            "duplicates defaults and unsupported virtual keys are rejected");
    require(availableOutputKeys(custom) == std::vector<unsigned>({spaceKey,leftCtrlKey,0x70,0xA3,0x41}),
            "custom dropdown follows permanent defaults in insertion order");
    custom.outputKey = 0xA3;
    const auto encodedCustom = serializeSettings(custom);
    require(encodedCustom.find("OutputKey=Right CTRL\nCustomOutputKeys=F1;Right CTRL;A\n") != std::string::npos,
            "selection and list are serialized with stable readable names");
    const auto decodedCustom = parseSettings(encodedCustom);
    require(decodedCustom.outputKey == custom.outputKey && decodedCustom.customOutputKeys == custom.customOutputKeys,
            "selection and ordered custom list survive round trip");
    const auto reordered = parseSettings("[MorseBridge]\nOutputKey= f1 \nCustomOutputKeys= a ; F1 ; right ctrl \n");
    require(reordered.outputKey == 0x70 && reordered.customOutputKeys == std::vector<unsigned>({0x41,0x70,0xA3}),
            "selection before list plus name casing and spacing are supported");
    const auto damaged = parseSettings("[MorseBridge]\nOutputKey=F2\nCustomOutputKeys=SPACE;Left CTRL;;bogus;F1;f1;Right CTRL;CTRL+A;999;F25;\n");
    require(damaged.outputKey == spaceKey && damaged.customOutputKeys == std::vector<unsigned>({0x70,0xA3}),
            "malformed list drops unsupported and duplicate entries and unavailable selection falls back to SPACE");
    require(parseSettings("[MorseBridge]\nOutputKey=LEFT CTRL\n").outputKey == leftCtrlKey,
            "Left CTRL is selectable without being a custom entry");
    require(defaults(parseSettings("[MorseBridge]\nOutputKey=CTRL+A\n[Other]\nCustomOutputKeys=F1\nOutputKey=F1\n")),
            "unsupported combinations and foreign output settings cannot alter defaults");
    removeCustomOutputKey(custom,0x70);
    require(custom.outputKey == 0xA3 && custom.customOutputKeys == std::vector<unsigned>({0xA3,0x41}),
            "removing another key preserves active choice and remaining order");
    removeCustomOutputKey(custom,0xA3);
    require(custom.outputKey == spaceKey && custom.customOutputKeys == std::vector<unsigned>({0x41}),
            "removing active custom key returns selection to SPACE");
    custom.outputKey = leftCtrlKey;
    removeCustomOutputKey(custom,leftCtrlKey);
    require(custom.outputKey == leftCtrlKey, "permanent defaults cannot be removed");
    Settings bounded;
    for (const auto& key : supportedOutputKeys()) addCustomOutputKey(bounded,key.vk);
    require(bounded.customOutputKeys.size() == maxCustomOutputKeys &&
            availableOutputKeys(bounded).size() == maxCustomOutputKeys + 2,
            "custom list is limited to 32 keys");
    std::string oversizedList = "[MorseBridge]\nCustomOutputKeys=";
    for (const auto& key : supportedOutputKeys()) { oversizedList += key.name; oversizedList += ';'; }
    require(parseSettings(oversizedList).customOutputKeys == bounded.customOutputKeys,
            "persisted list limit keeps first valid unique keys in order");
    Settings unsanitized;
    unsanitized.customOutputKeys = {0,spaceKey,0x70,0x70,leftCtrlKey,9999,0x41};
    unsanitized.outputKey = 0x71;
    const auto sanitized = parseSettings(serializeSettings(unsanitized));
    require(sanitized.customOutputKeys == std::vector<unsigned>({0x70,0x41}) && sanitized.outputKey == spaceKey,
            "saving sanitizes invalid in-memory custom list and selection");
    for (const auto& key : supportedOutputKeys()) {
        require(findOutputKey(key.vk) == &key && parseOutputKey(key.name) == &key,
                "supported names and key identities are unique and reversible");
        require(key.vk > 0 && key.vk < 255, "supported identifiers fit Windows virtual-key range");
    }
    for (const auto* invalid : {"", "CTRL", "SHIFT", "ALT", "Left WIN", "PAUSE", "PRINTSCREEN", "F0", "A+B", "0x20"})
        require(!parseOutputKey(invalid), "ambiguous unsupported and compound key names are rejected");
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
    custom.outputKey = 0x41;
    require(saveSettings(path,custom), "custom list and active key save to existing settings file");
    loaded = loadSettings(path);
    require(loaded.customOutputKeys == custom.customOutputKeys && loaded.outputKey == 0x41,
            "custom list and selected key survive a settings file restart");
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
