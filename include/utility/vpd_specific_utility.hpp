#pragma once

#include "config.h"

#include "constants.hpp"
#include "logger.hpp"
#include "types.hpp"

#include <utility/common_utility.hpp>
#include <utility/dbus_utility.hpp>

#include <filesystem>
#include <fstream>
#include <regex>

namespace vpd
{
namespace vpdSpecificUtility
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
inline std::string generateBadVPDFileName(const std::string& vpdFilePath)
{
    std::string badVpdFileName = BAD_VPD_DIR;
    if (vpdFilePath.find("i2c") != std::string::npos)
    {
        badVpdFileName += "i2c-";
        std::regex i2cPattern("(at24/)([0-9]+-[0-9]+)\\/");
        std::smatch match;
        if (std::regex_search(vpdFilePath, match, i2cPattern))
        {
            badVpdFileName += match.str(2);
        }
    }
    else if (vpdFilePath.find("spi") != std::string::npos)
    {
        std::regex spiPattern("((spi)[0-9]+)(.0)");
        std::smatch match;
        if (std::regex_search(vpdFilePath, match, spiPattern))
        {
            badVpdFileName += match.str(1);
        }
    }
    return badVpdFileName;
}

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
inline void dumpBadVpd(const std::string& vpdFilePath,
                       const types::BinaryVector& vpdVector)
{
    std::filesystem::create_directory(BAD_VPD_DIR);
    auto badVpdPath = generateBadVPDFileName(vpdFilePath);

    if (std::filesystem::exists(badVpdPath))
    {
        std::error_code ec;
        std::filesystem::remove(badVpdPath, ec);
        if (ec) // error code
        {
            std::string error = "Error removing the existing broken vpd in ";
            error += badVpdPath;
            error += ". Error code : ";
            error += ec.value();
            error += ". Error message : ";
            error += ec.message();
            throw std::runtime_error(error);
        }
    }

    std::ofstream badVpdFileStream(badVpdPath, std::ofstream::binary);
    if (badVpdFileStream.is_open())
    {
        throw std::runtime_error(
            "Failed to open bad vpd file path in /tmp/bad-vpd. "
            "Unable to dump the broken/bad vpd file.");
    }

    badVpdFileStream.write(reinterpret_cast<const char*>(vpdVector.data()),
                           vpdVector.size());
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
inline void getKwVal(const types::IPZKwdValueMap& kwdValueMap,
                     const std::string& kwd, std::string& kwdValue)
{
    if (kwd.empty())
    {
        logging::logMessage("Invalid parameters");
        throw std::runtime_error("Invalid parameters");
    }

    auto itrToKwd = kwdValueMap.find(kwd);
    if (itrToKwd != kwdValueMap.end())
    {
        kwdValue = itrToKwd->second;
        return;
    }

    throw std::runtime_error("Keyword not found");
}

/**
 * @brief An API to process encoding of a keyword.
 *
 * @param[in] keyword - Keyword to be processed.
 * @param[in] encoding - Type of encoding.
 * @return Value after being processed for encoded type.
 */
inline std::string encodeKeyword(const std::string& keyword,
                                 const std::string& encoding)
{
    // Default value is keyword value
    std::string result(keyword.begin(), keyword.end());

    if (encoding == "MAC")
    {
        result.clear();
        size_t firstByte = keyword[0];
        result += commonUtility::toHex(firstByte >> 4);
        result += commonUtility::toHex(firstByte & 0x0f);
        for (size_t i = 1; i < keyword.size(); ++i)
        {
            result += ":";
            result += commonUtility::toHex(keyword[i] >> 4);
            result += commonUtility::toHex(keyword[i] & 0x0f);
        }
    }
    else if (encoding == "DATE")
    {
        // Date, represent as
        // <year>-<month>-<day> <hour>:<min>
        result.clear();
        static constexpr uint8_t skipPrefix = 3;

        auto strItr = keyword.begin();
        advance(strItr, skipPrefix);
        for_each(strItr, keyword.end(), [&result](size_t c) { result += c; });

        result.insert(constants::BD_YEAR_END, 1, '-');
        result.insert(constants::BD_MONTH_END, 1, '-');
        result.insert(constants::BD_DAY_END, 1, ' ');
        result.insert(constants::BD_HOUR_END, 1, ':');
    }

    return result;
}

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
inline void insertOrMerge(types::InterfaceMap& map,
                          const std::string& interface,
                          types::PropertyMap&& propertyMap)
{
    if (map.find(interface) != map.end())
    {
        auto& prop = map.at(interface);
        prop.insert(propertyMap.begin(), propertyMap.end());
    }
    else
    {
        map.emplace(interface, propertyMap);
    }
}

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
inline std::string
    getExpandedLocationCode(const std::string& unexpandedLocationCode,
                            const types::VPDMapVariant& parsedVpdMap)
{
    auto expanded{unexpandedLocationCode};
    if (auto ipzVpdMap = std::get_if<types::IPZVpdMap>(&parsedVpdMap))
    {
        try
        {
            // Expanded location code is formaed by combining two keywords
            // depending on type in unexpanded one. Second one is always "SE".
            std::string kwd1, kwd2{"SE"};

            // interface to search for required keywords;
            std::string kwdInterface;

            // record which holds the required keywords.
            std::string recordName;

            auto pos = unexpandedLocationCode.find("fcs");
            if (pos != std::string::npos)
            {
                kwd1 = "FC";
                kwdInterface = "com.ibm.ipzvpd.VCEN";
                recordName = "VCEN";
            }
            else
            {
                pos = unexpandedLocationCode.find("mts");
                if (pos != std::string::npos)
                {
                    kwd1 = "TM";
                    kwdInterface = "com.ibm.ipzvpd.VSYS";
                    recordName = "VSYS";
                }
                else
                {
                    throw std::runtime_error(
                        "Error detecting type of unexpanded location code.");
                }
            }

            std::string firstKwdValue, secondKwdValue;
            auto itrToVCEN = (*ipzVpdMap).find(recordName);
            if (itrToVCEN != (*ipzVpdMap).end())
            {
                // The exceptions will be cautght at end.
                getKwVal(itrToVCEN->second, kwd1, firstKwdValue);
                getKwVal(itrToVCEN->second, kwd2, secondKwdValue);
            }
            else
            {
                std::array<const char*, 1> interfaceList = {
                    kwdInterface.c_str()};

                types::MapperGetObject mapperRetValue =
                    dbusUtility::getObjectMap(
                        "/xyz/openbmc_project/inventory/system/"
                        "chassis/motherboard",
                        interfaceList);

                if (mapperRetValue.empty())
                {
                    throw std::runtime_error("Mapper failed to get service");
                }

                const std::string& serviceName =
                    std::get<0>(mapperRetValue.at(0));

                auto retVal = dbusUtility::readDbusProperty(
                    serviceName,
                    "/xyz/openbmc_project/inventory/system/"
                    "chassis/motherboard",
                    kwdInterface, kwd1);

                if (auto kwdVal = std::get_if<types::BinaryVector>(&retVal))
                {
                    firstKwdValue.assign(
                        reinterpret_cast<const char*>(kwdVal->data()),
                        kwdVal->size());
                }
                else
                {
                    throw std::runtime_error("Failed to read value of " + kwd1 +
                                             " from Bus");
                }

                retVal = dbusUtility::readDbusProperty(
                    serviceName,
                    "/xyz/openbmc_project/inventory/system/"
                    "chassis/motherboard",
                    kwdInterface, kwd2);

                if (auto kwdVal = std::get_if<types::BinaryVector>(&retVal))
                {
                    secondKwdValue.assign(
                        reinterpret_cast<const char*>(kwdVal->data()),
                        kwdVal->size());
                }
                else
                {
                    throw std::runtime_error("Failed to read value of " + kwd2 +
                                             " from Bus");
                }
            }

            if (unexpandedLocationCode.find("fcs") != std::string::npos)
            {
                // TODO: See if ND0 can be placed in the JSON
                expanded.replace(pos, 3,
                                 firstKwdValue.substr(0, 4) + ".ND0." +
                                     secondKwdValue);
            }
            else
            {
                replace(firstKwdValue.begin(), firstKwdValue.end(), '-', '.');
                expanded.replace(pos, 3, firstKwdValue + "." + secondKwdValue);
            }
        }
        catch (const std::exception& ex)
        {
            logging::logMessage(
                "Failed to expand location code with exception: " +
                std::string(ex.what()));
        }
    }
    return expanded;
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
inline void getVpdDataInVector(const std::string& vpdFilePath,
                               types::BinaryVector& vpdVector,
                               size_t& vpdStartOffset)
{
    try
    {
        std::fstream vpdFileStream;
        vpdFileStream.exceptions(std::ifstream::badbit |
                                 std::ifstream::failbit);
        vpdFileStream.open(vpdFilePath, std::ios::in | std::ios::binary);
        auto vpdSizeToRead = std::min(std::filesystem::file_size(vpdFilePath),
                                      static_cast<uintmax_t>(65504));
        vpdVector.resize(vpdSizeToRead);

        vpdFileStream.seekg(vpdStartOffset, std::ios_base::beg);
        vpdFileStream.read(reinterpret_cast<char*>(&vpdVector[0]),
                           vpdSizeToRead);

        vpdVector.resize(vpdFileStream.gcount());
        vpdFileStream.clear(std::ios_base::eofbit);
    }
    catch (const std::ifstream::failure& fail)
    {
        std::cerr << "Exception in file handling [" << vpdFilePath
                  << "] error : " << fail.what();
        throw;
    }
}

/**
 * @brief An API to get D-bus representation of given VPD keyword.
 *
 * @param[in] i_keywordName - VPD keyword name.
 *
 * @return D-bus representation of given keyword.
 */
inline std::string getDbusPropNameForGivenKw(const std::string& i_keywordName)
{
    // Check for "#" prefixed VPD keyword.
    if ((i_keywordName.size() == vpd::constants::TWO_BYTES) &&
        (i_keywordName.at(0) == constants::POUND_KW))
    {
        // D-bus doesn't support "#". Replace "#" with "PD_" for those "#"
        // prefixed keywords.
        return (std::string(constants::POUND_KW_PREFIX) +
                i_keywordName.substr(1));
    }

    // Return the keyword name back, if D-bus representation is same as the VPD
    // keyword name.
    return i_keywordName;
}
} // namespace vpdSpecificUtility
} // namespace vpd
