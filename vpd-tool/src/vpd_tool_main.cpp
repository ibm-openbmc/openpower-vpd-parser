#include "vpd_tool.hpp"

#include <CLI/CLI.hpp>

#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    int l_retValue = 0;
    try
    {
        CLI::App l_app{
            "VPD Command line tool to dump the inventory and to read and "
            "update the keywords"};

        std::string l_objectPath{};
        std::string l_recordName{};
        std::string l_keyword{};
        std::string l_filePath{};

        auto l_objectOption = l_app.add_option("--object, -O", l_objectPath,
                                               "Enter the Object Path");
        auto l_recordOption = l_app.add_option("--record, -R", l_recordName,
                                               "Enter the Record Name");
        auto l_keywordOption = l_app.add_option("--keyword, -K", l_keyword,
                                                "Enter the Keyword");

        auto l_readOption =
            l_app
                .add_flag(
                    "--readKeyword, -r",
                    "Read the data of the given keyword. { "
                    "vpd-tool-exe --readKeyword/-r --object/-O "
                    "\"object-name\" --record/-R \"record-name\" --keyword/-K "
                    "\"keyword-name\" }")
                ->needs(l_objectOption)
                ->needs(l_recordOption)
                ->needs(l_keywordOption);

        auto l_hardwareOption = l_app.add_flag(
            "--Hardware, -H",
            "This is a supplementary flag to read/write directly from/to hardware. "
            "User should provide valid hardware/eeprom path (and not dbus object "
            "path) in the -O/--object path. CAUTION: Developer Only Option.");

        CLI11_PARSE(l_app, argc, argv);

        if ((*l_keywordOption) && (l_keyword.size() != 2))
        {
            throw std::runtime_error("Keyword " + l_keyword +
                                     " not supported.");
        }

        bool l_isHardwareOperation = false;
        if (*l_hardwareOption)
        {
            l_isHardwareOperation = true;

            if (!std::filesystem::exists(
                    l_objectPath)) // if dbus object path is given or
                                   // invalid eeprom path is given
            {
                std::string l_errorMsg = "Invalid EEPROM path : ";
                l_errorMsg += l_objectPath;
                l_errorMsg +=
                    ". The given EEPROM path doesn't exist. Provide valid "
                    "EEPROM path when -H flag is used. Refer help option. ";
                throw std::runtime_error(l_errorMsg);
            }
        }

        if (*l_readOption)
        {
            vpd::VpdTool l_vpdToolObj;
            l_vpdToolObj.readKeyword(l_objectPath, l_recordName, l_keyword,
                                     l_isHardwareOperation);
        }
        else
        {
            throw std::runtime_error(
                "One of the valid options is required.\nRefer "
                "--help for list of options.");
        }
    }
    catch (const std::exception& l_ex)
    {
        std::cerr << l_ex.what() << std::endl;
        l_retValue = -1;
    }

    return l_retValue;
}
