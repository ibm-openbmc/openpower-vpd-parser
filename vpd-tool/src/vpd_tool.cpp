#include "vpd_tool.hpp"

#include "tool_constants.hpp"
#include "tool_types.hpp"
#include "utils.hpp"

#include <iostream>
#include <tuple>

namespace vpd
{
int VpdTool::readKeyword(const std::string& i_fruPath,
                         const std::string& i_recordName,
                         const std::string& i_keywordName,
                         const bool i_onHardware,
                         const std::string& i_fileToSave)
{
    int l_rc = -1;
    try
    {
        types::DbusVariantType l_keywordValue;
        if (i_onHardware)
        {
            // TODO: Implement read keyword's value from hardware
        }
        else
        {
            std::string l_inventoryObjectPath(constants::baseInventoryPath +
                                              i_fruPath);

            l_keywordValue = utils::readDbusProperty(
                constants::inventoryManagerService, l_inventoryObjectPath,
                constants::ipzVpdInfPrefix + i_recordName, i_keywordName);
        }

        if (const auto l_value =
                std::get_if<types::BinaryVector>(&l_keywordValue))
        {
            const std::string& l_keywordStrValue =
                utils::getPrintableValue(*l_value);

            if (i_fileToSave.empty())
            {
                nlohmann::json l_resultInJson = nlohmann::json::object({});
                nlohmann::json l_keywordValInJson = nlohmann::json::object({});

                l_keywordValInJson.emplace(i_keywordName, l_keywordStrValue);
                l_resultInJson.emplace(i_fruPath, l_keywordValInJson);

                utils::printJson(l_resultInJson);
            }
            else
            {
                // TODO: Write result to a given file path.
            }
            l_rc = 0;
        }
        else
        {
            // TODO: Enable logging when verbose is enabled.
            // std::cout << "Invalid data type received." << std::endl;
        }
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Read keyword's value for FRU path: " << i_fruPath
                  << ", Record: " << i_recordName
                  << ", Keyword: " << i_keywordName
                  << ", failed with exception: " << l_ex.what() << std::endl;*/
    }
    return l_rc;
}

int VpdTool::writeKeyword(const std::string& i_vpdPath,
                          const std::string& i_recordName,
                          const std::string& i_keywordName,
                          const std::string& i_keywordValue,
                          const bool i_onHardware,
                          const std::string& i_filePath)
{
    int l_rc = -1;
    try
    {
        if (i_vpdPath.empty() || i_recordName.empty() || i_keywordName.empty())
        {
            throw std::runtime_error("Received input is empty.");
        }

        std::string l_keywordValue{i_keywordValue};
        types::BinaryVector l_keywordValueInBinary;

        if (!i_filePath.empty())
        {
            l_keywordValue = utils::readValueFromFile(i_filePath);
        }

        if (l_keywordValue.empty())
        {
            throw std::runtime_error("Keyword value is empty.");
        }

        l_keywordValueInBinary = utils::convertToBinary(l_keywordValue);

        std::string l_vpdPath{i_vpdPath};
        if (!i_onHardware)
        {
            l_vpdPath = constants::baseInventoryPath + i_vpdPath;
        }

        auto l_paramsToWrite = std::make_tuple(i_recordName, i_keywordName,
                                               l_keywordValueInBinary);
        l_rc = utils::writeKeyword(
            constants::vpdManagerService, constants::vpdManagerObjectPath,
            constants::vpdManagerInf, l_vpdPath, l_paramsToWrite);
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
} // namespace vpd
