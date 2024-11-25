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

    if (!std::filesystem::exists(i_pathToJson) ||
        std::filesystem::is_empty(i_pathToJson))
    {
        throw std::runtime_error("Incorrect File Path or empty file = " +
                                 i_pathToJson);
    }

    std::ifstream jsonFile(i_pathToJson);
    if (!jsonFile)
    {
        throw std::runtime_error("Failed to access Json path = " +
                                 i_pathToJson);
    }

    try
    {
        return nlohmann::json::parse(jsonFile);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        throw std::runtime_error("Failed to parse JSON file");
    }
}

} // namespace jsonUtility
} // namespace vpd
