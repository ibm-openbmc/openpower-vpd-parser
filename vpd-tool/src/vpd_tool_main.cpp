#include "tool_constants.hpp"
#include "tool_json_utility.hpp"
#include "tool_utils.hpp"
#include "vpd_tool.hpp"

#include <CLI/CLI.hpp>

#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    int l_rc = vpd::constants::FAILURE;
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
        "vpd-tool -r -H -O <EEPROM Path> -R <Record Name> -K <Keyword Name> --file <File Path>\n"
        "Dump Object:\n"
        "    From dbus to console: "
        "vpd-tool -o -O <DBus Object Path>");

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

    auto l_dumpObjFlag = l_app.add_flag("--dumpObject, -o", "Dump Object")
                             ->needs(l_objectOption);

    // ToDo: Take offset value from user for hardware path.

    CLI11_PARSE(l_app, argc, argv);

    if (!l_objectOption->empty() && l_vpdPath.empty())
    {
        std::cout << "Given path is empty." << std::endl;
        return l_rc;
    }

    if (!l_recordOption->empty() &&
        (l_recordName.size() != vpd::constants::RECORD_SIZE))
    {
        std::cerr << "Record " << l_recordName << " is not supported."
                  << std::endl;
        return l_rc;
    }

    if (!l_keywordOption->empty() &&
        (l_keywordName.size() != vpd::constants::KEYWORD_SIZE))
    {
        std::cerr << "Keyword " << l_keywordName << " is not supported."
                  << std::endl;
        return l_rc;
    }

    (void)l_fileOption;

    if (!l_readFlag->empty())
    {
        std::error_code l_ec;

        if (!l_hardwareFlag->empty() &&
            !std::filesystem::exists(l_vpdPath, l_ec))
        {
            std::string l_errMessage{"Given EEPROM file path doesn't exist : " +
                                     l_vpdPath};

            if (l_ec)
            {
                l_errMessage += ". filesystem call exists failed, reason: " +
                                l_ec.message();
            }

            std::cerr << l_errMessage << std::endl;
            return l_rc;
        }

        bool l_isHardwareOperation = (!l_hardwareFlag->empty() ? true : false);
        vpd::VpdTool l_vpdToolObj;

        l_rc = l_vpdToolObj.readKeyword(l_vpdPath, l_recordName, l_keywordName,
                                        l_isHardwareOperation, l_filePath);
    }
    else if (!l_dumpObjFlag->empty())
    {
        vpd::VpdTool l_vpdToolObj;
        l_rc = l_vpdToolObj.dumpObject(l_vpdPath);
    }
    else
    {
        std::cout << l_app.help() << std::endl;
    }
    return l_rc;
}
