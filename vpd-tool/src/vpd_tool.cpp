#include "config.h"

#include "vpd_tool.hpp"

#include "tool_constants.hpp"
#include "tool_json_utility.hpp"
#include "tool_types.hpp"
#include "utils.hpp"

#include <iostream>

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

int VpdTool::dumpObject(const std::string& i_fruPath,
                        nlohmann::json& io_resultJson) const noexcept
{
    int l_rc{-1};
    try
    {
        nlohmann::json l_fruJson = nlohmann::json::object_t({});
        l_fruJson.emplace(i_fruPath, nlohmann::json::object_t({}));

        auto& l_fruObject = l_fruJson[i_fruPath];

        // if input FRU path contains base inventory path prefix, strip it.
        const std::string l_effFruPath =
            (i_fruPath.find(vpd::constants::baseInventoryPath) ==
             std::string::npos)
                ? i_fruPath
                : i_fruPath.substr(strlen(vpd::constants::baseInventoryPath));

        const auto l_presentPropertyInJson = getInventoryPropertyJson<bool>(
            l_effFruPath, constants::inventoryItemInf, "Present");
        if (!l_presentPropertyInJson.empty())
        {
            l_fruObject.insert(l_presentPropertyInJson.cbegin(),
                               l_presentPropertyInJson.cend());
            l_rc = 0;
        }

        const auto l_prettyNameInJson = getInventoryPropertyJson<std::string>(
            l_effFruPath, constants::inventoryItemInf, "PrettyName");
        if (!l_prettyNameInJson.empty())
        {
            l_fruObject.insert(l_prettyNameInJson.cbegin(),
                               l_prettyNameInJson.cend());
            l_rc = 0;
        }

        const auto l_locationCodeInJson = getInventoryPropertyJson<std::string>(
            l_effFruPath, constants::locationCodeInf, "LocationCode");
        if (!l_locationCodeInJson.empty())
        {
            l_fruObject.insert(l_locationCodeInJson.cbegin(),
                               l_locationCodeInJson.cend());
            l_rc = 0;
        }

        const auto l_subModelInJson = getInventoryPropertyJson<std::string>(
            l_effFruPath, constants::assetInf, "SubModel");

        if (!l_subModelInJson.empty() &&
            !l_subModelInJson.value("SubModel", "").empty())
        {
            l_fruObject.insert(l_subModelInJson.cbegin(),
                               l_subModelInJson.cend());
            l_rc = 0;
        }

        const auto l_viniPropertiesInJson = getVINIPropertiesJson(l_effFruPath);
        if (!l_viniPropertiesInJson.empty())
        {
            l_fruObject.insert(l_viniPropertiesInJson.cbegin(),
                               l_viniPropertiesInJson.cend());
            l_rc = 0;
        }

        if (io_resultJson.empty())
        {
            io_resultJson += l_fruJson;
        }
        else
        {
            io_resultJson.at(0).insert(l_fruJson.cbegin(), l_fruJson.cend());
        }

        // TODO: Insert "type" and "TYPE" once Inventory JSON parsing is added.
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cerr << "Dump Object failed for FRU " << i_fruPath <<
        " due to error:" << l_ex.what() << std::endl;*/
    }
    return l_rc;
}

nlohmann::json
    VpdTool::getVINIPropertiesJson(const std::string& i_fruPath) const noexcept
{
    nlohmann::json l_resultInJson = nlohmann::json::object({});

    const std::array<std::string, 5> l_viniKeyWords{"SN", "PN", "CC", "FN",
                                                    "DR"};

    auto l_readKeyWord = [i_fruPath, &l_resultInJson,
                          this](const std::string& i_keyWord) {
        nlohmann::json l_keyWordJson =
            getInventoryPropertyJson<vpd::types::BinaryVector>(
                i_fruPath, constants::kwdVpdInf, i_keyWord);
        l_resultInJson.insert(l_keyWordJson.cbegin(), l_keyWordJson.cend());
    };

    std::for_each(l_viniKeyWords.cbegin(), l_viniKeyWords.cend(),
                  l_readKeyWord);
    return l_resultInJson;
}

template <typename PropertyType>
nlohmann::json VpdTool::getInventoryPropertyJson(
    const std::string& i_objectPath, const std::string& i_interface,
    const std::string& i_propertyName) const noexcept
{
    nlohmann::json l_resultInJson = nlohmann::json::object({});
    try
    {
        types::DbusVariantType l_keyWordValue;
        const std::string l_inventoryObjectPath{constants::baseInventoryPath +
                                                i_objectPath};
        l_keyWordValue = utils::readDbusProperty(
            constants::inventoryManagerService, l_inventoryObjectPath,
            i_interface, i_propertyName);

        if (const auto l_value = std::get_if<PropertyType>(&l_keyWordValue))
        {
            if constexpr (std::is_same<PropertyType, std::string>::value)
            {
                l_resultInJson.emplace(i_propertyName, *l_value);
            }
            else if constexpr (std::is_same<PropertyType, bool>::value)
            {
                l_resultInJson.emplace(i_propertyName,
                                       *l_value ? "true" : "false");
            }
            else if constexpr (std::is_same<PropertyType,
                                            types::BinaryVector>::value)
            {
                const std::string& l_keywordStrValue =
                    vpd::utils::getPrintableValue(*l_value);

                l_resultInJson.emplace(i_propertyName, l_keywordStrValue);
            }
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
        /*std::cerr << "Read " << i_propertyName << " value for FRU path: " <<
           i_fruPath
                  << ", failed with exception: " << l_ex.what() << std::endl;*/
    }
    return l_resultInJson;
}

int VpdTool::dumpInventory() const noexcept
{
    int l_rc{-1};
    try
    {
        nlohmann::json l_resultInJson = nlohmann::json::array({});

        const nlohmann::json l_parsedJson =
            vpd::jsonUtility::getParsedJson(INVENTORY_JSON_SYM_LINK);

        if (l_parsedJson.contains("frus"))
        {
            const nlohmann::json& l_listOfFrus =
                l_parsedJson["frus"].get_ref<const nlohmann::json::object_t&>();

            for (const auto& l_frus : l_listOfFrus.items())
            {
                for (const auto& l_aFru : l_frus.value())
                {
                    if (l_aFru.contains("inventoryPath"))
                    {
                        const std::string& l_inventoryPath =
                            l_aFru.value("inventoryPath", "");

                        l_rc = dumpObject(l_inventoryPath, l_resultInJson);
                    }
                }
            }
            vpd::utils::printJson(l_resultInJson);
        }
        else
        {
            // TODO: Enable logging when verbose is enabled.
            // std::cerr << "Inventory JSON doesn't have frus" << std::endl;
        }
    }
    catch (const std::exception& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        std::cerr << "Dump Inventory failed with error :" << l_ex.what()
                  << std::endl;
    }
    return l_rc;
}

} // namespace vpd
