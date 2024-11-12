#include "constants.hpp"
#include "types.hpp"
#include "vpd_tool.hpp"

#include <CLI/CLI.hpp>

#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    int l_rc = -1;
    CLI::App l_app{"VPD Command Line Tool"};

    std::string l_vpdPath{};
    std::string l_recordName{};
    std::string l_keywordName{};
    std::string l_filePath{};

    l_app.footer(
        "Read:\n"
        "    IPZ Format:\n"
        "        From dbus to console: "
        "vpd-tool -r -O <DBus Object Path> -R <Record Name> -K <Keyword Name>\n"
        "        From dbus to file: "
        "vpd-tool -r -O <EEPROM Path> -R <Record Name> -K <Keyword Name> --file <File Path>\n"
        "        From hardware to console: "
        "vpd-tool -r -H -O <DBus Object Path> -R <Record Name> -K <Keyword Name>\n"
        "        From hardware to file: "
        "vpd-tool -r -H -O <EEPROM Path> -R <Record Name> -K <Keyword Name> --file <File Path>");

    auto l_objectOption = l_app.add_option("--object, -O", l_vpdPath,
                                           "File path");
    auto l_recordOption = l_app.add_option("--record, -R", l_recordName,
                                           "Record name");
    auto l_keywordOption = l_app.add_option("--keyword, -K", l_keywordName,
                                            "Keyword name");

    auto l_fileOption = l_app.add_option("--file", l_filePath,
                                         "Absolute file path");

    auto l_readFlag = l_app.add_flag("--readKeyword, -r", "Read keyword")
                          ->needs(l_objectOption)
                          ->needs(l_recordOption)
                          ->needs(l_keywordOption);

    auto l_hardwareFlag = l_app.add_flag("--Hardware, -H",
                                         "CAUTION: Developer only option.");

    CLI11_PARSE(l_app, argc, argv);

    if (*l_objectOption && l_vpdPath.empty())
    {
        throw std::runtime_error("Given path is empty.");
    }

    if (*l_recordOption && (l_recordName.size() != vpd::constants::RECORD_SIZE))
    {
        throw std::runtime_error("Record " + l_recordName +
                                 " is not supported.");
    }

    if (*l_keywordOption &&
        (l_keywordName.size() != vpd::constants::KEYWORD_SIZE))
    {
        throw std::runtime_error("Keyword " + l_keywordName +
                                 " is not supported.");
    }

    if (*l_recordOption && (l_recordName.size() != vpd::constants::RECORD_SIZE))
    {
        throw std::runtime_error("Record " + l_recordName + " not supported.");
    }

    bool l_isHardwareOperation = ((*l_hardwareFlag) ? true : false);

    (void)l_fileOption;

    if (*l_readFlag)
    {
        vpd::VpdTool l_vpdToolObj;
        l_rc = l_vpdToolObj.readKeyword(l_vpdPath, l_recordName, l_keywordName,
                                        l_isHardwareOperation, l_filePath);
    }
    else
    {
        std::cout << l_app.help() << std::endl;
    }
    return l_rc;
}
