#include "vpd_tool.hpp"

#include "utils.hpp"

#include <iostream>

namespace vpd
{
void VpdTool::readKeyword(const std::string& i_fruPath,
                          const std::string& i_recordName,
                          const std::string& i_keyword, bool i_onHardware)
{
    types::DbusVariantType l_keywordValue;
    if (i_onHardware)
    {
        l_keywordValue = utils::readKeywordFromHardware(
            "com.ibm.VPD.Manager", "/com/ibm/VPD/Manager",
            "com.ibm.VPD.Manager", i_fruPath,
            make_tuple(i_recordName, i_keyword));
    }
    else
    {
        constexpr auto l_inventoryPath = "/xyz/openbmc_project/inventory";
        std::string l_fullObjectPath = l_inventoryPath + i_fruPath;

        l_keywordValue = utils::readDbusProperty(
            "xyz.openbmc_project.Inventory.Manager", l_fullObjectPath,
            "com.ibm.ipzvpd." + i_recordName, i_keyword);
    }

    if (auto l_value = std::get_if<types::BinaryVector>(&l_keywordValue))
    {
        const std::string& l_strValue = utils::getPrintableValue(*l_value);

        nlohmann::json l_output = nlohmann::json::object({});
        nlohmann::json l_kwVal = nlohmann::json::object({});

        l_kwVal.emplace(i_keyword, l_strValue);
        l_output.emplace(i_fruPath, l_kwVal);

        utils::printJson(l_output);
    }
}
} // namespace vpd
