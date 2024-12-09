#pragma once

#include <nlohmann/json.hpp>

#include <fstream>

namespace vpd
{
namespace jsonUtility
{

/**
 * @brief API to parse respective JSON.
 *
 * Exception is thrown in case of JSON parse error.
 *
 * @param[in] i_pathToJson - Path to JSON.
 * @return Parsed JSON.
 * @throw std::runtime_error
 */
inline nlohmann::json getParsedJson(const std::string& i_pathToJson)
{
    if (i_pathToJson.empty())
    {
        throw std::runtime_error("Path to JSON is missing");
    }

    try
    {
        if (!std::filesystem::exists(i_pathToJson) ||
            std::filesystem::is_empty(i_pathToJson))
        {
            throw std::runtime_error("Incorrect File Path or empty file = " +
                                     i_pathToJson);
        }
    }
    catch (std::exception& l_ex)
    {
        throw std::runtime_error("Failed to check file path: " + i_pathToJson +
                                 " Error: " + l_ex.what());
    }

    std::ifstream l_jsonFile(i_pathToJson);
    if (!l_jsonFile)
    {
        throw std::runtime_error("Failed to access Json path = " +
                                 i_pathToJson);
    }

    try
    {
        return nlohmann::json::parse(l_jsonFile);
    }
    catch (const nlohmann::json::parse_error& l_ex)
    {
        throw std::runtime_error("Failed to parse JSON file: " + i_pathToJson +
                                 " Error: " + l_ex.what());
    }
}

/**
 * @brief Get FRU EEPROM path from system config JSON
 *
 * Given either D-bus inventory path/FRU EEPROM path/redundant EEPROM path,
 * this API returns FRU EEPROM path if present in JSON.
 *
 * @param[in] i_sysCfgJsonObj - System config JSON object
 * @param[in] i_vpdPath - Path to where VPD is stored.
 *
 * @throw std::runtime_error.
 *
 * @return On success return valid path, on failure return empty string.
 */
inline std::string getFruPathFromJson(const nlohmann::json& i_sysCfgJsonObj,
                                      const std::string& i_vpdPath)
{
    if (i_vpdPath.empty())
    {
        throw std::runtime_error("Path parameter is empty.");
    }

    if (!i_sysCfgJsonObj.contains("frus"))
    {
        throw std::runtime_error("Missing frus tag in system config JSON.");
    }

    // check if given path is FRU path
    if (i_sysCfgJsonObj["frus"].contains(i_vpdPath))
    {
        return i_vpdPath;
    }

    const nlohmann::json& l_fruList =
        i_sysCfgJsonObj["frus"].get_ref<const nlohmann::json::object_t&>();

    for (const auto& l_fru : l_fruList.items())
    {
        const auto l_fruPath = l_fru.key();

        // check if given path is redundant FRU path or inventory path
        if (i_vpdPath == i_sysCfgJsonObj["frus"][l_fruPath].at(0).value(
                             "redundantEeprom", "") ||
            (i_vpdPath == i_sysCfgJsonObj["frus"][l_fruPath].at(0).value(
                              "inventoryPath", "")))
        {
            return l_fruPath;
        }
    }
    return std::string();
}

} // namespace jsonUtility
} // namespace vpd
