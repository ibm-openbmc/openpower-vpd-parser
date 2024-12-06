#include "config.h"

#include "vpd_tool.hpp"

#include "tool_constants.hpp"
#include "tool_types.hpp"
#include "tool_utils.hpp"

#include <iostream>
#include <tuple>

namespace vpd
{
int VpdTool::readKeyword(const std::string& i_vpdPath,
                         const std::string& i_recordName,
                         const std::string& i_keywordName,
                         const bool i_onHardware,
                         const std::string& i_fileToSave) const noexcept
{
    int l_rc = constants::FAILURE;
    try
    {
        types::DbusVariantType l_keywordValue;
        if (i_onHardware)
        {
            l_keywordValue = utils::readKeywordFromHardware(
                i_vpdPath, std::make_tuple(i_recordName, i_keywordName));
        }
        else
        {
            std::string l_inventoryObjectPath(constants::baseInventoryPath +
                                              i_vpdPath);

            l_keywordValue = utils::readDbusProperty(
                constants::inventoryManagerService, l_inventoryObjectPath,
                constants::ipzVpdInfPrefix + i_recordName, i_keywordName);
        }

        if (const auto l_value =
                std::get_if<types::BinaryVector>(&l_keywordValue);
            l_value && !l_value->empty())
        {
            // ToDo: Print value in both ASCII and hex formats
            const std::string& l_keywordStrValue =
                utils::getPrintableValue(*l_value);

            if (i_fileToSave.empty())
            {
                nlohmann::json l_resultInJson = nlohmann::json::object({});
                nlohmann::json l_keywordValInJson = nlohmann::json::object({});

                l_keywordValInJson.emplace(i_keywordName, l_keywordStrValue);
                l_resultInJson.emplace(i_vpdPath, l_keywordValInJson);

                utils::printJson(l_resultInJson);
            }
            else
            {
                // TODO: Write result to a given file path.
            }
            l_rc = constants::SUCCESS;
        }
        else
        {
            // TODO: Enable logging when verbose is enabled.
            // std::cout << "Invalid data type or empty data received." <<
            // std::endl;
        }
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Read keyword's value for path: " << i_vpdPath
                  << ", Record: " << i_recordName
                  << ", Keyword: " << i_keywordName
                  << " is failed, exception: " << l_ex.what() << std::endl;*/
    }
    return l_rc;
}

int VpdTool::writeKeyword(const std::string& i_vpdPath,
                          const std::string& i_recordName,
                          const std::string& i_keywordName,
                          const std::string& i_keywordValue,
                          const bool i_onHardware,
                          const std::string& i_filePath) const noexcept
{
    int l_rc = -1;
    try
    {
        if (i_vpdPath.empty() || i_recordName.empty() || i_keywordName.empty())
        {
            throw std::runtime_error("Received input is empty.");
        }

        if (!i_filePath.empty())
        {
            // ToDo: Take keyword value from the file
        }

        std::string l_vpdPath{i_vpdPath};
        if (!i_onHardware)
        {
            l_vpdPath = constants::baseInventoryPath + i_vpdPath;
        }

        auto l_paramsToWrite =
            std::make_tuple(i_recordName, i_keywordName,
                            utils::convertToBinary(i_keywordValue));
        l_rc = utils::writeKeyword(
            constants::vpdManagerService, constants::vpdManagerObjectPath,
            constants::vpdManagerInfName, l_vpdPath, l_paramsToWrite);
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Write keyword's value for path: " << i_vpdPath
                  << ", Record: " << i_recordName
                  << ", Keyword: " << i_keywordName
                  << " is failed. Exception: " << l_ex.what() << std::endl;*/
    }
    return l_rc;
}

int VpdTool::cleanSystemVpd(const std::string& i_recordName,
                            const std::string& i_keywordName) const noexcept
{
    int l_rc{-1};

    // Map of Keywords which can be reset by vpd-tool manufacturing clean
    // option
    // {Record1 : {{Keyword1, Default value1},{Keyword2, Default value2} }},
    // {Record2 : {{Keyword1, Default value1},{Keyword2, Default value2} }}
    static const types::MfgCleanRecordMap l_mfgCleanKeywordMap{
        {"UTIL",
         {{"D0", types::BinaryVector(1, 0x00)},
          {"D1", types::BinaryVector(1, 0x00)},
          {"F0", types::BinaryVector(8, 0x00)},
          {"F5", types::BinaryVector(16, 0x00)},
          {"F6", types::BinaryVector(16, 0x00)}}},
        {"VSYS",
         {{"BR", types::BinaryVector(2, 0x20)},
          {"TM", types::BinaryVector(8, 0x20)},
          {"RG", types::BinaryVector(4, 0x20)},
          {"SE", types::BinaryVector(7, 0x20)},
          {"SU", types::BinaryVector(6, 0x20)},
          {"RB", types::BinaryVector(4, 0x20)},
          {"WN", types::BinaryVector(12, 0x20)},
          {"FV", types::BinaryVector(32, 0x20)}}},
        {"VCEN", {{"SE", types::BinaryVector(7, 0x20)}}}};

    // search for the record in map
    if (const auto l_recordEntry = l_mfgCleanKeywordMap.find(i_recordName);
        l_recordEntry != l_mfgCleanKeywordMap.end())
    {
        // search for the keyword in map
        if (const auto l_keyWordEntry =
                l_recordEntry->second.find(i_keywordName);
            l_keyWordEntry != l_recordEntry->second.end())
        {
            try
            {
                // read the default value from map
                const auto& l_defaultKeyWordValue = l_keyWordEntry->second;

                // read the keyword value from hardware
                const auto l_keywordValue = utils::readKeywordFromHardware(
                    SYSTEM_VPD_FILE_PATH,
                    std::make_tuple(i_recordName, i_keywordName));

                if (const auto l_keywordValueHW =
                        std::get_if<types::BinaryVector>(&l_keywordValue);
                    l_keywordValueHW && !l_keywordValueHW->empty())
                {
                    if (l_defaultKeyWordValue != *l_keywordValueHW)
                    {
                        // update the keyword value to default value in Primary,
                        // Backup and D-Bus
                        const std::string& l_defaultKeyWordValueStr =
                            utils::getPrintableValue(l_defaultKeyWordValue);

                        l_rc = writeKeyword(SYSTEM_VPD_FILE_PATH, i_recordName,
                                            i_keywordName,
                                            l_defaultKeyWordValueStr, true);
                    }
                }
                else
                {
                    // TODO: Enable logging when verbose is enabled.
                    // std::cerr << "Invalid value read from hardware for
                    // Record: "
                    //           << i_recordName << " Keyword: " <<
                    //           i_keywordName
                    //           << std::endl;
                }
            }
            catch (const std::exception& l_ex)
            {
                // TODO: Enable logging when verbose is enabled.
                /*std::cerr << "Failed to reset Record: " << i_recordName
                          << " Keyword: " << i_keywordName
                          << " due to error: " << l_ex.what() << std::endl;*/
            }
        }
        else
        {
            std::cout << "Keyword " << i_keywordName << " not supported"
                      << std::endl;
        }
    }
    else
    {
        std::cout << "Record " << i_recordName << " not supported" << std::endl;
    }
    return l_rc;
}

} // namespace vpd
