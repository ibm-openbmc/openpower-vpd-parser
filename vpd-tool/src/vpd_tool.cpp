#include "config.h"

#include "vpd_tool.hpp"

#include "tool_constants.hpp"
#include "tool_json_utility.hpp"
#include "tool_types.hpp"
#include "tool_utils.hpp"

#include <iostream>

namespace vpd
{
int VpdTool::readKeyword(const std::string& i_vpdPath,
                         const std::string& i_recordName,
                         const std::string& i_keywordName,
                         const bool i_onHardware,
                         const std::string& i_fileToSave)
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

int VpdTool::dumpObject(const std::string& i_fruPath) const noexcept
{
    int l_rc{constants::FAILURE};
    try
    {
        const nlohmann::json l_resultJson = getFruPropertiesJson(i_fruPath);
        if (!l_resultJson.empty())
        {
            utils::printJson(l_resultJson);
            l_rc = constants::SUCCESS;
        }
    }
    catch (std::exception& l_ex)
    {
        std::cerr << "Dump Object failed for FRU: " << i_fruPath
                  << " Error: " << l_ex.what() << std::endl;
    }
    return l_rc;
}

nlohmann::json VpdTool::getFruPropertiesJson(const std::string& i_fruPath) const
{
    nlohmann::json l_resultInJson = nlohmann::json::array({});
    const nlohmann::json& l_sysCfgJsonObj =
        vpd::jsonUtility::getParsedJson(INVENTORY_JSON_SYM_LINK);
    // TODO: Implement getFruPropertiesJson
    (void)i_fruPath;
    (void)l_sysCfgJsonObj;
    return l_resultInJson;
}

} // namespace vpd
