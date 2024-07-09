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
} // namespace utils
} // namespace vpd
