#include "tool_constants.hpp"
#include "tool_utils.hpp"
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
    std::string l_keywordValue{};

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
        "Write:\n"
        "    IPZ Format:\n"
        "        On dbus: "
        "vpd-tool -w -O <DBus Object Path> -R <Record Name> -K <Keyword Name> -V <Keyword Value>\n"
        "        On dbus, Take keyword value from file:\n"
        "              vpd-tool -w -O <EEPROM Path> -R <Record Name> -K <Keyword Name> --file <File Path>\n"
        "        On hardware: "
        "vpd-tool -w -H -O <DBus Object Path> -R <Record Name> -K <Keyword Name> -V <Keyword Value>\n"
        "        On hardware, Take keyword value from file:\n"
        "              vpd-tool -w -H -O <EEPROM Path> -R <Record Name> -K <Keyword Name> --file <File Path>\n"
        "MfgClean:\n"
        "        Flag to clean and reset specific keywords on system VPD to its default value.\n"
        "        Needs: --record --keyword\n"
        "        vpd-tool --mfgClean -R <Record Name> -K <Keyword Name>");

    auto l_objectOption = l_app.add_option("--object, -O", l_vpdPath,
                                           "File path");
    auto l_recordOption = l_app.add_option("--record, -R", l_recordName,
                                           "Record name");
    auto l_keywordOption = l_app.add_option("--keyword, -K", l_keywordName,
                                            "Keyword name");

    auto l_fileOption = l_app.add_option("--file", l_filePath,
                                         "Absolute file path");

    auto l_keywordValueOption =
        l_app.add_option("--value, -V", l_keywordValue,
                         "Keyword value in ascii/hex format."
                         " ascii ex: 01234; hex ex: 0x30313233");

    auto l_hardwareFlag = l_app.add_flag("--Hardware, -H",
                                         "CAUTION: Developer only option.");

    auto l_readFlag = l_app.add_flag("--readKeyword, -r", "Read keyword")
                          ->needs(l_objectOption)
                          ->needs(l_recordOption)
                          ->needs(l_keywordOption);

    auto l_writeFlag = l_app.add_flag("--writeKeyword, -w", "Write keyword")
                           ->needs(l_objectOption)
                           ->needs(l_recordOption)
                           ->needs(l_keywordOption);

    auto l_mfgCleanFlag = l_app.add_flag("--mfgClean", "Manufacturing clean")
                              ->needs(l_recordOption)
                              ->needs(l_keywordOption);

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
    else if (l_writeFlag->count() > 0)
    {
        if ((l_hardwareFlag->count() > 0) &&
            !std::filesystem::exists(l_vpdPath))
        {
            std::cerr << "Given EEPROM file path doesn't exist : " + l_vpdPath
                      << std::endl;
            return l_rc;
        }

        if ((l_fileOption->count() > 0) && !std::filesystem::exists(l_filePath))
        {
            std::cerr
                << "Please provide a valid absolute file path which has keyword value.\nUse --value/--file to give "
                   "keyword value. Refer --help."
                << std::endl;
            return l_rc;
        }
        else if ((l_keywordValueOption->count() > 0) && l_keywordValue.empty())
        {
            std::cerr
                << "Please provide keyword value.\nUse --value/--file to give "
                   "keyword value. Refer --help."
                << std::endl;
            return l_rc;
        }
        else if ((l_fileOption->count() == 0) &&
                 (l_keywordValueOption->count() == 0))
        {
            std::cerr
                << "Please provide keyword value.\nUse --value/--file to give "
                   "keyword value. Refer --help."
                << std::endl;
            return l_rc;
        }

        bool l_isHardwareOperation = ((l_hardwareFlag->count() > 0) ? true
                                                                    : false);

        vpd::VpdTool l_vpdToolObj;
        l_vpdToolObj.writeKeyword(l_vpdPath, l_recordName, l_keywordName,
                                  l_keywordValue, l_isHardwareOperation,
                                  l_filePath);
    }
    else if (!l_mfgCleanFlag->empty())
    {
        constexpr auto MAX_CONFIRMATION_STR_LENGTH{3};
        std::string l_confirmation{};
        std::cout
            << "This option resets some of the system VPD keywords to their default values. Do you really wish to proceed further?[yes/no]:";
        std::cin >> std::setw(MAX_CONFIRMATION_STR_LENGTH) >> l_confirmation;

        if (vpd::utils::equalStrings(l_confirmation, "yes", false) ||
            vpd::utils::equalStrings(l_confirmation, "y", false))
        {
            vpd::VpdTool l_vpdToolObj;
            l_rc = l_vpdToolObj.cleanSystemVpd(l_recordName, l_keywordName);
        }
        else
        {
            l_rc = 0;
        }
    }
    else
    {
        std::cout << l_app.help() << std::endl;
    }
    return l_rc;
}
