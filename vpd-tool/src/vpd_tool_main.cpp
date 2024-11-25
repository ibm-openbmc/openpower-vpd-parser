#include "tool_constants.hpp"
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
        "vpd-tool -r -H -O <EEPROM Path> -R <Record Name> -K <Keyword Name> --file <File Path>\n"
        "\nDump Object:\n"
        "        From DBus to console: "
        "vpd-tool --dumpObject/-o --object/-O <DBus Object Path>");

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

    auto l_dumpInventoryFlag = l_app.add_flag("--dumpInventory, -i",
                                              "Dump inventory");

    CLI11_PARSE(l_app, argc, argv);

    if ((l_objectOption->count() > 0) && l_vpdPath.empty())
    {
        std::cout << "Given path is empty." << std::endl;
        return l_rc;
    }

    if ((l_recordOption->count() > 0) &&
        (l_recordName.size() != vpd::constants::RECORD_SIZE))
    {
        std::cerr << "Record " << l_recordName << " is not supported."
                  << std::endl;
        return l_rc;
    }

    if ((l_keywordOption->count() > 0) &&
        (l_keywordName.size() != vpd::constants::KEYWORD_SIZE))
    {
        std::cerr << "Keyword " << l_keywordName << " is not supported."
                  << std::endl;
        return l_rc;
    }

    (void)l_fileOption;

    if (l_readFlag->count() > 0)
    {
        if ((l_hardwareFlag->count() > 0) &&
            !std::filesystem::exists(l_vpdPath))
        {
            std::cerr << "Given EEPROM file path doesn't exist : " + l_vpdPath
                      << std::endl;
            return l_rc;
        }

        bool l_isHardwareOperation = ((l_hardwareFlag->count() > 0) ? true
                                                                    : false);
        vpd::VpdTool l_vpdToolObj;

        l_rc = l_vpdToolObj.readKeyword(l_vpdPath, l_recordName, l_keywordName,
                                        l_isHardwareOperation, l_filePath);
    }
    else if (l_dumpObjFlag->count() > 0)
    {
        if (l_objectOption->count() > 0)
        {
            vpd::VpdTool l_vpdToolObj;

            nlohmann::json l_resultInJson = nlohmann::json::array({});
            l_rc = l_vpdToolObj.dumpObject(l_vpdPath, l_resultInJson);

            if (0 == l_rc)
            {
                vpd::utils::printJson(l_resultInJson);
            }
        }
    }
    else if (*l_dumpInventoryFlag)
    {
        vpd::VpdTool l_vpdToolObj;
        l_rc = l_vpdToolObj.dumpInventory();
    }
    else
    {
        std::cout << l_app.help() << std::endl;
    }
    return l_rc;
}
