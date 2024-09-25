#pragma once

#include "exceptions.hpp"
#include "logger.hpp"

#include <gpiod.hpp>
#include <nlohmann/json.hpp>
#include <utility/common_utility.hpp>

#include <fstream>
#include <type_traits>
#include <unordered_map>

namespace vpd
{
namespace jsonUtility
{

// forward declaration of API for function map.
bool processSystemCmdTag(const nlohmann::json& i_parsedConfigJson,
                         const std::string& i_vpdFilePath,
                         const std::string& i_baseAction,
                         const std::string& i_flagToProcess);

// forward declaration of API for function map.
bool processGpioPresenceTag(const nlohmann::json& i_parsedConfigJson,
                            const std::string& i_vpdFilePath,
                            const std::string& i_baseAction,
                            const std::string& i_flagToProcess);

// forward declaration of API for function map.
bool procesSetGpioTag(const nlohmann::json& i_parsedConfigJson,
                      const std::string& i_vpdFilePath,
                      const std::string& i_baseAction,
                      const std::string& i_flagToProcess);

// Function pointers to process tags from config JSON.
typedef bool (*functionPtr)(const nlohmann::json& i_parsedConfigJson,
                            const std::string& i_vpdFilePath,
                            const std::string& i_baseAction,
                            const std::string& i_flagToProcess);

inline std::unordered_map<std::string, functionPtr> funcionMap{
    {"gpioPresence", processGpioPresenceTag},
    {"setGpio", procesSetGpioTag},
    {"systemCmd", processSystemCmdTag}};

/**
 * @brief API to read VPD offset from JSON file.
 *
 * @param[in] i_sysCfgJsonObj - Parsed system config JSON object.
 * @param[in] i_vpdFilePath - VPD file path.
 * @return VPD offset if found in JSON, 0 otherwise.
 */
inline size_t getVPDOffset(const nlohmann::json& i_sysCfgJsonObj,
                           const std::string& i_vpdFilePath)
{
    if (i_vpdFilePath.empty() || (i_sysCfgJsonObj.empty()) ||
        (!i_sysCfgJsonObj.contains("frus")))
    {
        return 0;
    }

    if (i_sysCfgJsonObj["frus"].contains(i_vpdFilePath))
    {
        return i_sysCfgJsonObj["frus"][i_vpdFilePath].at(0).value("offset", 0);
    }

    const nlohmann::json& l_fruList =
        i_sysCfgJsonObj["frus"].get_ref<const nlohmann::json::object_t&>();

    for (const auto& l_fru : l_fruList.items())
    {
        const auto l_fruPath = l_fru.key();

        // check if given path is redundant FRU path
        if (i_vpdFilePath == i_sysCfgJsonObj["frus"][l_fruPath].at(0).value(
                                 "redundantEeprom", ""))
        {
            // Return the offset of redundant EEPROM taken from JSON.
            return i_sysCfgJsonObj["frus"][l_fruPath].at(0).value("offset", 0);
        }
    }

    return 0;
}

/**
 * @brief API to parse respective JSON.
 *
 * Exception is thrown in case of JSON parse error.
 *
 * @param[in] pathToJson - Path to JSON.
 * @return Parsed JSON.
 */
inline nlohmann::json getParsedJson(const std::string& pathToJson)
{
    if (pathToJson.empty())
    {
        throw std::runtime_error("Path to JSON is missing");
    }

    if (!std::filesystem::exists(pathToJson) ||
        std::filesystem::is_empty(pathToJson))
    {
        throw std::runtime_error("Incorrect File Path or empty file");
    }

    std::ifstream jsonFile(pathToJson);
    if (!jsonFile)
    {
        throw std::runtime_error("Failed to access Json path = " + pathToJson);
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

/**
 * @brief Process "PostFailAction" defined in config JSON.
 *
 * In case there is some error in the processing of "preAction" execution and a
 * set of procedure needs to be done as a part of post fail action. This base
 * action can be defined in the config JSON for that FRU and it will be handled
 * under this API.
 *
 * @param[in] i_parsedConfigJson - config JSON
 * @param[in] i_vpdFilePath - EEPROM file path
 * @param[in] i_flagToProcess - To identify which flag(s) needs to be processed
 * under PostFailAction tag of config JSON.
 * @return - success or failure
 */
inline bool executePostFailAction(const nlohmann::json& i_parsedConfigJson,
                                  const std::string& i_vpdFilePath,
                                  const std::string& i_flagToProcess)
{
    if (i_parsedConfigJson.empty() || i_vpdFilePath.empty() ||
        i_flagToProcess.empty())
    {
        logging::logMessage(
            "Invalid parameters. Abort processing for post fail action");

        return false;
    }

    if (!(i_parsedConfigJson["frus"][i_vpdFilePath].at(0).contains(
            "PostFailAction")))
    {
        logging::logMessage(
            "PostFailAction flag missing in config JSON. Abort processing");

        return false;
    }

    if (!(i_parsedConfigJson["frus"][i_vpdFilePath].at(0))["PostFailAction"]
             .contains(i_flagToProcess))
    {
        logging::logMessage(
            "Config JSON missing flag " + i_flagToProcess +
            " to execute post fail action for path = " + i_vpdFilePath);

        return false;
    }

    for (const auto& l_tags : (i_parsedConfigJson["frus"][i_vpdFilePath].at(
             0))["PostFailAction"][i_flagToProcess]
                                  .items())
    {
        auto itrToFunction = funcionMap.find(l_tags.key());
        if (itrToFunction != funcionMap.end())
        {
            if (!itrToFunction->second(i_parsedConfigJson, i_vpdFilePath,
                                       "PostFailAction", i_flagToProcess))
            {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Process "systemCmd" tag for a given FRU.
 *
 * The API will process "systemCmd" tag if it is defined in the config
 * JSON for the given FRU.
 *
 * @param[in] i_parsedConfigJson - config JSON
 * @param[in] i_vpdFilePath - EEPROM file path
 * @param[in] i_baseAction - Base action for which this tag has been called.
 * @param[in] i_flagToProcess - Flag nested under the base action for which this
 * tag has been called.
 * @return Execution status.
 */
inline bool processSystemCmdTag(const nlohmann::json& i_parsedConfigJson,
                                const std::string& i_vpdFilePath,
                                const std::string& i_baseAction,
                                const std::string& i_flagToProcess)
{
    if (i_vpdFilePath.empty() || i_parsedConfigJson.empty() ||
        i_baseAction.empty() || i_flagToProcess.empty())
    {
        logging::logMessage(
            "Invalid parameter. Abort processing of processSystemCmd.");
        return false;
    }

    if (!((i_parsedConfigJson["frus"][i_vpdFilePath].at(
               0)[i_baseAction][i_flagToProcess]["systemCmd"])
              .contains("cmd")))
    {
        logging::logMessage(
            "Config JSON missing required information to execute system command for EEPROM " +
            i_vpdFilePath);

        return false;
    }

    try
    {
        const std::string& l_systemCommand =
            i_parsedConfigJson["frus"][i_vpdFilePath].at(
                0)[i_baseAction][i_flagToProcess]["systemCmd"]["cmd"];

        logging::logMessage("Command = " + l_systemCommand);
        commonUtility::executeCmd(l_systemCommand);
        return true;
    }
    catch (const std::exception& e)
    {
        std::string l_errMsg = "Process system tag failed for exception: ";
        l_errMsg += e.what();

        logging::logMessage(l_errMsg);
        return false;
    }
}

/**
 * @brief Checks for presence of a given FRU using GPIO line.
 *
 * This API returns the presence information of the FRU corresponding to the
 * given VPD file path by setting the presence pin.
 *
 * @param[in] i_parsedConfigJson - config JSON
 * @param[in] i_vpdFilePath - EEPROM file path
 * @param[in] i_baseAction - Base action for which this tag has been called.
 * @param[in] i_flagToProcess - Flag nested under the base action for which this
 * tag has been called.
 * @return Execution status.
 */
inline bool processGpioPresenceTag(const nlohmann::json& i_parsedConfigJson,
                                   const std::string& i_vpdFilePath,
                                   const std::string& i_baseAction,
                                   const std::string& i_flagToProcess)
{
    if (i_vpdFilePath.empty() || i_parsedConfigJson.empty() ||
        i_baseAction.empty() || i_flagToProcess.empty())
    {
        logging::logMessage(
            "Invalid parameter. Abort processing of processGpioPresence tag");
        return false;
    }

    if (!(((i_parsedConfigJson["frus"][i_vpdFilePath].at(
                0)[i_baseAction][i_flagToProcess]["gpioPresence"])
               .contains("pin")) &&
          ((i_parsedConfigJson["frus"][i_vpdFilePath].at(
                0)[i_baseAction][i_flagToProcess]["gpioPresence"])
               .contains("value"))))
    {
        logging::logMessage(
            "Config JSON missing required information to detect presence for EEPROM " +
            i_vpdFilePath);

        return false;
    }

    // get the pin name
    const std::string& l_presencePinName =
        i_parsedConfigJson["frus"][i_vpdFilePath].at(
            0)[i_baseAction][i_flagToProcess]["gpioPresence"]["pin"];

    // get the pin value
    uint8_t l_presencePinValue = i_parsedConfigJson["frus"][i_vpdFilePath].at(
        0)[i_baseAction][i_flagToProcess]["gpioPresence"]["value"];

    try
    {
        gpiod::line l_presenceLine = gpiod::find_line(l_presencePinName);

        if (!l_presenceLine)
        {
            throw GpioException("Couldn't find the GPIO line.");
        }

        l_presenceLine.request({"Read the presence line",
                                gpiod::line_request::DIRECTION_INPUT, 0});

        return (l_presencePinValue == l_presenceLine.get_value());
    }
    catch (const std::exception& ex)
    {
        std::string l_errMsg = "Exception on GPIO line: ";
        l_errMsg += l_presencePinName;
        l_errMsg += " Reason: ";
        l_errMsg += ex.what();
        l_errMsg += " File: " + i_vpdFilePath + " Pel Logged";

        //  TODO:log PEL
        logging::logMessage(l_errMsg);
        return false;
    }
}

/**
 * @brief Process "setGpio" tag for a given FRU.
 *
 * This API enables the GPIO line.
 *
 * @param[in] i_parsedConfigJson - config JSON
 * @param[in] i_vpdFilePath - EEPROM file path
 * @param[in] i_baseAction - Base action for which this tag has been called.
 * @param[in] i_flagToProcess - Flag nested under the base action for which this
 * tag has been called.
 * @return Execution status.
 */
inline bool procesSetGpioTag(const nlohmann::json& i_parsedConfigJson,
                             const std::string& i_vpdFilePath,
                             const std::string& i_baseAction,
                             const std::string& i_flagToProcess)
{
    if (i_vpdFilePath.empty() || i_parsedConfigJson.empty() ||
        i_baseAction.empty() || i_flagToProcess.empty())
    {
        logging::logMessage(
            "Invalid parameter. Abort processing of procesSetGpio.");
        return false;
    }

    if (!(((i_parsedConfigJson["frus"][i_vpdFilePath].at(
                0)[i_baseAction][i_flagToProcess]["setGpio"])
               .contains("pin")) &&
          ((i_parsedConfigJson["frus"][i_vpdFilePath].at(
                0)[i_baseAction][i_flagToProcess]["setGpio"])
               .contains("value"))))
    {
        logging::logMessage(
            "Config JSON missing required information to set gpio line for EEPROM " +
            i_vpdFilePath);

        return false;
    }

    const std::string& l_pinName = i_parsedConfigJson["frus"][i_vpdFilePath].at(
        0)[i_baseAction][i_flagToProcess]["setGpio"]["pin"];

    // Get the value to set
    uint8_t l_pinValue = i_parsedConfigJson["frus"][i_vpdFilePath].at(
        0)[i_baseAction][i_flagToProcess]["setGpio"]["value"];

    logging::logMessage("Setting GPIO: " + l_pinName + " to " +
                        std::to_string(l_pinValue));
    try
    {
        gpiod::line l_outputLine = gpiod::find_line(l_pinName);

        if (!l_outputLine)
        {
            throw GpioException("Couldn't find GPIO line.");
        }

        l_outputLine.request(
            {"FRU Action", ::gpiod::line_request::DIRECTION_OUTPUT, 0},
            l_pinValue);
        return true;
    }
    catch (const std::exception& ex)
    {
        std::string l_errMsg = "Exception on GPIO line: ";
        l_errMsg += l_pinName;
        l_errMsg += " Reason: ";
        l_errMsg += ex.what();
        l_errMsg += " File: " + i_vpdFilePath + " Pel Logged";

        // Take failure postAction
        if (!executePostFailAction(i_parsedConfigJson, i_vpdFilePath,
                                   i_flagToProcess))
        {
            logging::logMessage("executePostFailAction failed from exception.");
        }

        //  TODO:log PEL
        logging::logMessage(l_errMsg);
        return false;
    }
}
/**
 * @brief Process "PostAction" defined in config JSON.
 *
 * If any FRU(s) requires any special pre-handling, then this base action can be
 * defined for that FRU in the config JSON, processing of which will be handled
 * in this API.
 *
 * @throw std::runtime_error
 *
 * @param[in] i_parsedConfigJson - config JSON
 * @param[in] i_vpdFilePath - EEPROM file path
 * @param[in] i_flagToProcess - To identify which flag(s) needs to be processed
 * under PostAction tag of config JSON.
 * @return - false in case of paramter or Json error, true for successful
 * processing. Throws exception in case of tag failure.
 */
inline void executePostAction(const nlohmann::json& i_parsedConfigJson,
                              const std::string& i_vpdFilePath,
                              const std::string& i_flagToProcess)
{
    if (i_flagToProcess.empty() || i_vpdFilePath.empty() ||
        i_parsedConfigJson.empty())
    {
        logging::logMessage("Invalid parameter");
        return false;
    }

    if (!(i_parsedConfigJson["frus"][i_vpdFilePath].at(0))["postAction"]
             .contains(i_flagToProcess))
    {
        logging::logMessage(
            "Config JSON missing flag " + i_flagToProcess +
            " to execute Post-action for path = " + i_vpdFilePath);

        return false;
    }

    const nlohmann::json& l_tagsJson =
        (i_parsedConfigJson["frus"][i_vpdFilePath].at(
            0))["postAction"][i_flagToProcess];

    for (const auto& l_tag : l_tagsJson.items())
    {
        auto itrToFunction = funcionMap.find(l_tag.key());
        if (itrToFunction != funcionMap.end())
        {
            if (!itrToFunction->second(i_parsedConfigJson, i_vpdFilePath,
                                       "postAction", i_flagToProcess))
            {
                // In case any of the tag fails to execute. Throws the tag to
                // detect which tag failed
                throw std::runtime_error(l_tag.key());
            }
        }
    }

    return true;
}

/**
 * @brief Process "PreAction" defined in config JSON.
 *
 * If any FRU(s) requires any special pre-handling, then this base action can be
 * defined for that FRU in the config JSON, processing of which will be handled
 * in this API.
 *
 * @param[in] i_parsedConfigJson - config JSON
 * @param[in] i_vpdFilePath - EEPROM file path
 * @param[in] i_flagToProcess - To identify which flag(s) needs to be processed
 * under PreAction tag of config JSON.
 * @return - success or failure
 */
inline bool executePreAction(const nlohmann::json& i_parsedConfigJson,
                             const std::string& i_vpdFilePath,
                             const std::string& i_flagToProcess)
{
    if (i_flagToProcess.empty() || i_vpdFilePath.empty() ||
        i_parsedConfigJson.empty())
    {
        logging::logMessage("Invalid parameter");
        return false;
    }

    if (!(i_parsedConfigJson["frus"][i_vpdFilePath].at(0))["preAction"]
             .contains(i_flagToProcess))
    {
        logging::logMessage(
            "Config JSON missing flag " + i_flagToProcess +
            " to execute Pre-action for path = " + i_vpdFilePath);

        return false;
    }

    const nlohmann::json& l_tagsJson =
        (i_parsedConfigJson["frus"][i_vpdFilePath].at(
            0))["preAction"][i_flagToProcess];

    for (const auto& l_tag : l_tagsJson.items())
    {
        auto itrToFunction = funcionMap.find(l_tag.key());
        if (itrToFunction != funcionMap.end())
        {
            if (!itrToFunction->second(i_parsedConfigJson, i_vpdFilePath,
                                       "preAction", i_flagToProcess))
            {
                // In case any of the tag fails to execute. Mark preAction
                // as failed for that flag.
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Get redundant FRU path from system config JSON
 *
 * Given either D-bus inventory path/FRU path/redundant FRU path, this
 * API returns the redundant FRU path taken from "redundantEeprom" tag from
 * system config JSON.
 *
 * @param[in] i_sysCfgJsonObj - System config JSON object.
 * @param[in] i_vpdPath - Path to where VPD is stored.
 *
 * @throw std::runtime_error.
 * @return On success return valid path, on failure return empty string.
 */
inline std::string
    getRedundantEepromPathFromJson(const nlohmann::json& i_sysCfgJsonObj,
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
        return i_sysCfgJsonObj["frus"][i_vpdPath].at(0).value("redundantEeprom",
                                                              "");
    }

    const nlohmann::json& l_fruList =
        i_sysCfgJsonObj["frus"].get_ref<const nlohmann::json::object_t&>();

    for (const auto& l_fru : l_fruList.items())
    {
        const std::string& l_fruPath = l_fru.key();
        const std::string& l_redundantFruPath =
            i_sysCfgJsonObj["frus"][l_fruPath].at(0).value("redundantEeprom",
                                                           "");

        // check if given path is inventory path or redundant FRU path
        if ((i_sysCfgJsonObj["frus"][l_fruPath].at(0).value("inventoryPath",
                                                            "") == i_vpdPath) ||
            (l_redundantFruPath == i_vpdPath))
        {
            return l_redundantFruPath;
        }
    }
    return std::string();
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

/**
 * @brief Get inventory object path from system config JSON.
 *
 * Given either D-bus inventory path/FRU EEPROM path/redundant EEPROM path,
 * this API returns D-bus inventory path if present in JSON.
 *
 * @param[in] i_sysCfgJsonObj - System config JSON object
 * @param[in] i_vpdPath - Path to where VPD is stored.
 *
 * @throw std::runtime_error.
 *
 * @return On success a valid path is returned, on failure an empty string is
 * returned or an exception is thrown.
 */
inline std::string
    getInventoryObjPathFromJson(const nlohmann::json& i_sysCfgJsonObj,
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
        return i_sysCfgJsonObj["frus"][i_vpdPath].at(0).value("inventoryPath",
                                                              "");
    }

    const nlohmann::json& l_fruList =
        i_sysCfgJsonObj["frus"].get_ref<const nlohmann::json::object_t&>();

    for (const auto& l_fru : l_fruList.items())
    {
        const auto l_fruPath = l_fru.key();
        const auto l_invObjPath =
            i_sysCfgJsonObj["frus"][l_fruPath].at(0).value("inventoryPath", "");

        // check if given path is redundant FRU path or inventory path
        if (i_vpdPath == i_sysCfgJsonObj["frus"][l_fruPath].at(0).value(
                             "redundantEeprom", "") ||
            (i_vpdPath == l_invObjPath))
        {
            return l_invObjPath;
        }
    }
    return std::string();
}

/**
 * @brief An API to check backup and restore VPD is required.
 *
 * The API checks if there is provision for backup and restore mentioned in the
 * system config JSON, by looking "backupRestoreConfigPath" tag.
 * Checks if the path mentioned is a hardware path, by checking if the file path
 * exists and size of contents in the path.
 *
 * @param[in] i_sysCfgJsonObj - System config JSON object.
 *
 * @return true if backup and restore is required, false otherwise.
 */
inline bool isBackupAndRestoreRequired(const nlohmann::json& i_sysCfgJsonObj)
{
    try
    {
        const std::string& l_backupAndRestoreCfgFilePath =
            i_sysCfgJsonObj.value("backupRestoreConfigPath", "");
        if (!l_backupAndRestoreCfgFilePath.empty() &&
            std::filesystem::exists(l_backupAndRestoreCfgFilePath) &&
            !std::filesystem::is_empty(l_backupAndRestoreCfgFilePath))
        {
            return true;
        }
    }
    catch (std::exception& ex)
    {
        logging::logMessage(ex.what());
    }
    return false;
}

/**
 * @brief An API to check if FRU qualifies for polling.
 *
 * This API checks for "pollingRequired" tag in system config JSON.
 *
 * @param[in] i_sysCfgJsonObj -  System config JSON object.
 * @param[in] i_vpdFilePath - VPD file path.
 *
 * @return true if polling is required, false otherwise.
 */
inline bool isPollingRequired(const nlohmann::json& i_sysCfgJsonObj,
                              const std::string& i_vpdFilePath)
{
    if (i_sysCfgJsonObj.empty() || i_vpdFilePath.empty())
    {
        logging::logMessage("Invalid parameters");
        return false;
    }

    if (!i_sysCfgJsonObj.contains("frus"))
    {
        logging::logMessage("Missing frus section in system config JSON");
        return false;
    }

    if (i_sysCfgJsonObj["frus"][i_vpdFilePath].at(0).contains(
            "pollingRequired"))
    {
        return true;
    }

    return false;
}

/**
 * @brief An API to return list of FRUs that needs GPIO polling.
 *
 * An API that checks for the FRUs that requires GPIO polling and returns
 * a list of FRUs that needs polling. Returns an empty list if there are
 * no FRUs that requires polling.
 *
 * @throw std::runtime_error
 *
 * @param[in] i_sysCfgJsonObj - System config JSON object.
 *
 * @return list of FRUs parameters that needs polling.
 */
inline std::vector<std::string>
    getListOfGpioPollingFrus(const nlohmann::json& i_sysCfgJsonObj)
{
    if (i_sysCfgJsonObj.empty())
    {
        throw std::runtime_error("Invalid Parameters");
    }

    if (!i_sysCfgJsonObj.contains("frus"))
    {
        throw std::runtime_error("Missing frus section in system config JSON");
    }

    std::vector<std::string> l_gpioPollingRequiredFrusList;

    for (const auto& l_fru : i_sysCfgJsonObj["frus"].items())
    {
        const auto l_fruPath = l_fru.key();

        try
        {
            if (isPollingRequired(i_sysCfgJsonObj, l_fruPath))
            {
                if (i_sysCfgJsonObj["frus"][l_fruPath]
                        .at(0)["pollingRequired"]
                        .contains("hotPlugging") &&
                    i_sysCfgJsonObj["frus"][l_fruPath]
                        .at(0)["pollingRequired"]["hotPlugging"]
                        .contains("gpioPresence"))
                {
                    l_gpioPollingRequiredFrusList.push_back(l_fruPath);
                }
            }
        }
        catch (const std::exception& l_ex)
        {
            logging::logMessage(l_ex.what());
        }
    }

    return l_gpioPollingRequiredFrusList;
}

/**
 * @brief Get all related path(s) to update keyword value.
 *
 * Given FRU EEPROM path/Inventory path needs keyword's value update, this API
 * returns tuple of FRU EEPROM path, inventory path and redundant EEPROM path if
 * exists in the system config JSON.
 *
 * Note: If the inventory object path or redundant EEPROM path(s) are not found
 * in the system config JSON, corresponding fields will have empty value in the
 * returning tuple.
 *
 * @param[in] i_sysCfgJsonObj - System config JSON object.
 * @param[in,out] io_vpdPath - Inventory object path or FRU EEPROM path.
 *
 * @return On success returns tuple of EEPROM path, inventory path & redundant
 * path, on failure returns tuple with given input path alone.
 */
inline std::tuple<std::string, std::string, std::string>
    getAllPathsToUpdateKeyword(const nlohmann::json& i_sysCfgJsonObj,
                               std::string io_vpdPath)
{
    types::Path l_inventoryObjPath;
    types::Path l_redundantFruPath;
    try
    {
        if (!i_sysCfgJsonObj.empty())
        {
            // Get hardware path from system config JSON.
            const types::Path l_fruPath =
                jsonUtility::getFruPathFromJson(i_sysCfgJsonObj, io_vpdPath);

            if (!l_fruPath.empty())
            {
                io_vpdPath = l_fruPath;

                // Get inventory object path from system config JSON
                l_inventoryObjPath = jsonUtility::getInventoryObjPathFromJson(
                    i_sysCfgJsonObj, l_fruPath);

                // Get redundant hardware path if present in system config JSON
                l_redundantFruPath =
                    jsonUtility::getRedundantEepromPathFromJson(i_sysCfgJsonObj,
                                                                l_fruPath);
            }
        }
    }
    catch (const std::exception& l_exception)
    {
        logging::logMessage(
            "Failed to get all paths to update keyword value, error " +
            std::string(l_exception.what()));
    }
    return std::make_tuple(io_vpdPath, l_inventoryObjPath, l_redundantFruPath);
}

/**
 * @brief API to check if an action is required or not for given VPD path.
 *
 * JSON can contain pre-action, post-action etc defined for a VPD file path. The
 * API will check that.
 *
 * @param[in] i_sysCfgJsonObj - System config JSON object.
 * @param[in] i_vpdFruPath - EEPROM path.
 * @param[in] i_baseAction - Base action to be checked.
 * @param[in] i_flowFlag - Denotes the flow from where the request is being
 * generated.
 * @return - True if found, false otherwise.
 */
inline bool isActionRequired(const nlohmann::json& const i_sysCfgJsonObj,
                             std::string& i_vpdFruPath,
                             const std::string& i_baseAction,
                             const std::string& i_flowFlag)
{
    if (i_vpdFruPath.empty() || i_baseAction.empty() || i_flowFlag.empty())
    {
        logging::logMessage("Invalid parameters recieved.");
        return false;
    }

    if ((i_sysCfgJsonObj["frus"][i_vpdFruPath].at(0)).contains(i_baseAction))
    {
        if ((i_sysCfgJsonObj["frus"][i_vpdFruPath].at(0))["postAction"]
                .contains(i_flowFlag))
        {
            return true;
        }

        logging::logMessage("Flow flag not found in JSON for path: " +
                            i_vpdFruPath);
        return false;
    }

    logging::logMessage("Base action not found in JSON for path: " +
                        i_vpdFruPath);
    return false;
}

inline std::vector<std::string>
    getCcinList(const nlohmann::json& const i_sysCfgJsonObj,
                const std::string& i_vpdFruPath,
                std::optional<std::string> jsonKeyLevel1 = std::nullopt,
                std::optional<std::string> jsonKeyLevel2 = std::nullopt)
{
    std::vector<std::string> ccinList;
    if (i_sysCfgJsonObj.empty())
    {
        logging::logMessage("Empty Json object.");
        return ccinList;
    }

    if (jsonKeyLevel1.has_value())
    {
        if (jsonKeyLevel2.has_value())
        {
            for (const auto& item :
                 i_sysCfgJsonObj["frus"][i_vpdFruPath].at(0)[])
            {
                if (item.find("ccinList") != item.end())
                {}
            }
            i_sysCfgJsonObj["frus"][i_vpdFruPath]
        }
    }
}
} // namespace jsonUtility
} // namespace vpd
