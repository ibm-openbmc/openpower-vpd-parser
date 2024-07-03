#pragma once

#include "types.hpp"

#include <errno.h>

#include <nlohmann/json.hpp>

#include <array>
#include <iostream>
#include <source_location>
#include <span>
#include <string_view>
#include <vector>

namespace vpd
{
/**
 * @brief The namespace defines utlity methods required for processing of VPD.
 */
namespace utils
{
/**
 * @brief API to generate file name for bad VPD.
 *
 * For i2c eeproms - the pattern of the vpd-name will be
 * i2c-<bus-number>-<eeprom-address>.
 * For spi eeproms - the pattern of the vpd-name will be spi-<spi-number>.
 *
 * @param[in] vpdFilePath - file path of the vpd.
 * @return Generated file name.
 */
std::string generateBadVPDFileName(const std::string& vpdFilePath);

/**
 * @brief API which dumps the broken/bad vpd in a directory.
 * When the vpd is bad, this API places  the bad vpd file inside
 * "/tmp/bad-vpd" in BMC, in order to collect bad VPD data as a part of user
 * initiated BMC dump.
 *
 * Note: Throws exception in case of any failure.
 *
 * @param[in] vpdFilePath - vpd file path
 * @param[in] vpdVector - vpd vector
 */
void dumpBadVpd(const std::string& vpdFilePath,
                const types::BinaryVector& vpdVector);

/**
 * @brief API to return null at the end of variadic template args.
 *
 * @return empty string.
 */
inline std::string getCommand()
{
    return "";
}

/**
 * @brief API to arrange create command.
 *
 * @param[in] arguments to create the command
 * @return cmd - command string
 */
template <typename T, typename... Types>
inline std::string getCommand(T arg1, Types... args)
{
    std::string cmd = " " + arg1 + getCommand(args...);

    return cmd;
}

/**
 * @brief API to create shell command and execute.
 *
 * Note: Throws exception on any failure. Caller needs to handle.
 *
 * @param[in] arguments for command
 * @returns output of that command
 */
template <typename T, typename... Types>
inline std::vector<std::string> executeCmd(T&& path, Types... args)
{
    std::vector<std::string> cmdOutput;
    std::array<char, 128> buffer;

    std::string cmd = path + getCommand(args...);

    std::unique_ptr<FILE, decltype(&pclose)> cmdPipe(popen(cmd.c_str(), "r"),
                                                     pclose);
    if (!cmdPipe)
    {
        std::cerr << "popen failed with error" << strerror(errno) << std::endl;
        throw std::runtime_error("popen failed!");
    }
    while (fgets(buffer.data(), buffer.size(), cmdPipe.get()) != nullptr)
    {
        cmdOutput.emplace_back(buffer.data());
    }

    return cmdOutput;
}

/**
 * @brief An API to read value of a keyword.
 *
 * Note: Throws exception. Caller needs to handle.
 *
 * @param[in] kwdValueMap - A map having Kwd value pair.
 * @param[in] kwd - keyword name.
 * @param[out] kwdValue - Value of the keyword read from map.
 */
void getKwVal(const types::IPZKwdValueMap& kwdValueMap, const std::string& kwd,
              std::string& kwdValue);

/**
 * @brief An API to process encoding of a keyword.
 *
 * @param[in] keyword - Keyword to be processed.
 * @param[in] encoding - Type of encoding.
 * @return Value after being processed for encoded type.
 */
std::string encodeKeyword(const std::string& keyword,
                          const std::string& encoding);

/**
 * @brief Helper function to insert or merge in map.
 *
 * This method checks in an interface if the given interface exists. If the
 * interface key already exists, property map is inserted corresponding to it.
 * If the key does'nt exist then given interface and property map pair is newly
 * created. If the property present in propertymap already exist in the
 * InterfaceMap, then the new property value is ignored.
 *
 * @param[in,out] map - Interface map.
 * @param[in] interface - Interface to be processed.
 * @param[in] propertyMap - new property map that needs to be emplaced.
 */
void insertOrMerge(types::InterfaceMap& map, const std::string& interface,
                   types::PropertyMap&& propertyMap);

/**
 * @brief API to expand unpanded location code.
 *
 * Note: The API handles all the exception internally, in case of any error
 * unexpanded location code will be returned as it is.
 *
 * @param[in] unexpandedLocationCode - Unexpanded location code.
 * @param[in] parsedVpdMap - Parsed VPD map.
 * @return Expanded location code. In case of any error, unexpanded is returned
 * as it is.
 */
std::string getExpandedLocationCode(const std::string& unexpandedLocationCode,
                                    const types::VPDMapVariant& parsedVpdMap);

/** @brief Return the hex representation of the incoming byte.
 *
 * @param [in] aByte - The input byte
 * @returns The hex representation of the byte as a character.
 */
constexpr auto toHex(size_t aByte)
{
    constexpr auto map = "0123456789abcdef";
    return map[aByte];
}

/**
 * @brief An API to get VPD in a vector.
 *
 * The vector is required by the respective parser to fill the VPD map.
 * Note: API throws exception in case of failure. Caller needs to handle.
 *
 * @param[in] vpdFilePath - EEPROM path of the FRU.
 * @param[out] vpdVector - VPD in vector form.
 * @param[in] vpdStartOffset - Offset of VPD data in EEPROM.
 */
void getVpdDataInVector(const std::string& vpdFilePath,
                        types::BinaryVector& vpdVector, size_t& vpdStartOffset);

/**
 * @brief API to read VPD offset from JSON file.
 *
 * @param[in] parsedJson - Parsed JSON file for the system.
 * @param[in] vpdFilePath - path to the VPD file.
 * @return Offset of VPD in VPD file.
 */
size_t getVPDOffset(const nlohmann::json& parsedJson,
                    const std::string& vpdFilePath);

/**
 * @brief API to parse respective JSON.
 *
 * Exception is thrown in case of JSON parse error.
 *
 * @param[in] pathToJson - Path to JSON.
 * @return Parsed JSON.
 */
nlohmann::json getParsedJson(const std::string& pathToJson);

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
bool processSystemCmdTag(const nlohmann::json& i_parsedConfigJson,
                         const std::string& i_vpdFilePath,
                         const std::string& i_baseAction,
                         const std::string& i_flagToProcess);

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
bool processGpioPresenceTag(const nlohmann::json& i_parsedConfigJson,
                            const std::string& i_vpdFilePath,
                            const std::string& i_baseAction,
                            const std::string& i_flagToProcess);

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
bool procesSetGpioTag(const nlohmann::json& i_parsedConfigJson,
                      const std::string& i_vpdFilePath,
                      const std::string& i_baseAction,
                      const std::string& i_flagToProcess);

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
bool executePostFailAction(const nlohmann::json& i_parsedConfigJson,
                           const std::string& i_vpdFilePath,
                           const std::string& i_flagToProcess);

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
bool executePreAction(const nlohmann::json& i_parsedConfigJson,
                      const std::string& i_vpdFilePath,
                      const std::string& i_flagToProcess);

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
std::string
    getRedundantEepromPathFromJson(const nlohmann::json& i_sysCfgJsonObj,
                                   const std::string& i_vpdPath);

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
std::string getFruPathFromJson(const nlohmann::json& i_sysCfgJsonObj,
                               const std::string& i_vpdPath);

} // namespace utils
} // namespace vpd
