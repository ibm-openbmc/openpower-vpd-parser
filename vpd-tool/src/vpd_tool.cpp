#include "vpd_tool.hpp"

#include "tool_constants.hpp"
#include "tool_types.hpp"
#include "tool_utils.hpp"

#include <iostream>

extern bool VerboseMode;

namespace vpd
{
int VpdTool::readKeyword(const std::string& i_vpdPath,
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
                                              i_vpdPath);

            l_keywordValue = utils::readDbusProperty(
                constants::inventoryManagerService, l_inventoryObjectPath,
                constants::ipzVpdInfPrefix + i_recordName, i_keywordName);
        }

        if (const auto l_value =
                std::get_if<types::BinaryVector>(&l_keywordValue);
            l_value && !l_value->empty())
        {
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
            l_rc = 0;
        }
        else if (VerboseMode)
        {
            std::cout << "Invalid data type or empty data received."
                      << std::endl;
        }
    }
    catch (const std::exception& l_ex)
    {
        if (VerboseMode)
        {
            std::cerr << "Read keyword's value for path: " << i_vpdPath
                      << ", Record: " << i_recordName
                      << ", Keyword: " << i_keywordName
                      << " is failed, exception: " << l_ex.what() << std::endl;
        }
    }
    return l_rc;
}
} // namespace vpd
