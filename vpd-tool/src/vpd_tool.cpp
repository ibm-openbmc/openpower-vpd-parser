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

nlohmann::json
    VpdTool::getFruPropertiesJson(const std::string& i_objectPath) const
{
    nlohmann::json l_resultInJson = nlohmann::json::array({});
    const nlohmann::json& l_sysCfgJsonObj =
        vpd::jsonUtility::getParsedJson(INVENTORY_JSON_SYM_LINK);

    // validate the system config JSON.
    if (!l_sysCfgJsonObj.contains("frus"))
    {
        throw std::runtime_error("System Config JSON is not valid");
    }

    std::string l_eepromPath{};
    // get the EEPROM path for this object path
    l_eepromPath = jsonUtility::getFruPathFromJson(l_sysCfgJsonObj,
                                                   i_objectPath);

    nlohmann::json l_fruJson = nlohmann::json::object_t({});
    l_fruJson.emplace(i_objectPath, nlohmann::json::object_t({}));

    auto& l_fruObject = l_fruJson[i_objectPath];

    const auto l_presentPropertyInJson = getInventoryPropertyJson<bool>(
        i_objectPath, constants::inventoryItemInf, "Present");
    if (!l_presentPropertyInJson.empty())
    {
        l_fruObject.insert(l_presentPropertyInJson.cbegin(),
                           l_presentPropertyInJson.cend());
    }

    const auto l_prettyNameInJson = getInventoryPropertyJson<std::string>(
        i_objectPath, constants::inventoryItemInf, "PrettyName");
    if (!l_prettyNameInJson.empty())
    {
        l_fruObject.insert(l_prettyNameInJson.cbegin(),
                           l_prettyNameInJson.cend());
    }

    const auto l_locationCodeInJson = getInventoryPropertyJson<std::string>(
        i_objectPath, constants::locationCodeInf, "LocationCode");
    if (!l_locationCodeInJson.empty())
    {
        l_fruObject.insert(l_locationCodeInJson.cbegin(),
                           l_locationCodeInJson.cend());
    }

    const auto l_subModelInJson = getInventoryPropertyJson<std::string>(
        i_objectPath, constants::assetInf, "SubModel");

    if (!l_subModelInJson.empty() &&
        !l_subModelInJson.value("SubModel", "").empty())
    {
        l_fruObject.insert(l_subModelInJson.cbegin(), l_subModelInJson.cend());
    }

    const auto l_viniPropertiesInJson = getVINIPropertiesJson(i_objectPath);
    if (!l_viniPropertiesInJson.empty())
    {
        l_fruObject.insert(l_viniPropertiesInJson.cbegin(),
                           l_viniPropertiesInJson.cend());
    }

    const auto l_typePropertyJson = getFruTypeProperty(l_eepromPath,
                                                       l_sysCfgJsonObj);
    if (!l_typePropertyJson.empty())
    {
        l_fruObject.insert(l_typePropertyJson.cbegin(),
                           l_typePropertyJson.cend());
    }

    if (l_resultInJson.empty())
    {
        l_resultInJson += l_fruJson;
    }
    else
    {
        l_resultInJson.at(0).insert(l_fruJson.cbegin(), l_fruJson.cend());
    }
    return l_resultInJson;
}

nlohmann::json VpdTool::getVINIPropertiesJson(
    const std::string& i_objectPath) const noexcept
{
    nlohmann::json l_resultInJson = nlohmann::json::object({});

    const std::array<std::string, 5> l_viniKeyWords{"SN", "PN", "CC", "FN",
                                                    "DR"};

    auto l_readKeyWord = [i_objectPath, &l_resultInJson,
                          this](const std::string& i_keyWord) {
        nlohmann::json l_keyWordJson =
            getInventoryPropertyJson<vpd::types::BinaryVector>(
                i_objectPath, constants::kwdVpdInf, i_keyWord);
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

        l_keyWordValue =
            utils::readDbusProperty(constants::inventoryManagerService,
                                    i_objectPath, i_interface, i_propertyName);

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
           i_objectPath
                  << ", failed with exception: " << l_ex.what() << std::endl;*/
    }
    return l_resultInJson;
}

nlohmann::json VpdTool::getFruTypeProperty(
    const std::string& i_eepromPath,
    const nlohmann::json& i_sysCfgJsonObj) const noexcept
{
    nlohmann::json l_resultInJson = nlohmann::json::object({});

    if (!i_sysCfgJsonObj.contains("frus") ||
        !i_sysCfgJsonObj["frus"].contains(i_eepromPath))
    {
        return l_resultInJson;
    }

    const auto& l_fruObj = i_sysCfgJsonObj["frus"][i_eepromPath].at(0);

    std::string l_fruType{"FRU"};

    // check if our FRU has a "type" tag
    if (l_fruObj.contains("type"))
    {
        l_fruType = l_fruObj.at("type");
    }

    l_resultInJson.emplace("TYPE", l_fruType);

    if (l_fruObj.contains("inventoryPath"))
    {
        const std::string l_invPath = l_fruObj.at("inventoryPath");

        if (l_invPath.find("powersupply") != std::string::npos)
        {
            l_resultInJson.emplace("type", constants::powerSupplyInterface);
        }
        else if (l_invPath.find("fan") != std::string::npos)
        {
            l_resultInJson.emplace("type", constants::fanInterface);
        }
        else
        {
            // get the "type" property from "extraInterfaces"
            if (l_fruObj.contains("extraInterfaces"))
            {
                const auto& l_extraInfObj = l_fruObj["extraInterfaces"];

                for (const auto& l_exProperty : l_extraInfObj.items())
                {
                    if (l_exProperty.key().find("Item") != std::string::npos &&
                        l_exProperty.value().is_null())
                    {
                        l_resultInJson.emplace("type", l_exProperty.key());
                    }
                }
            }
        }
    }
    return l_resultInJson;
}

} // namespace vpd
