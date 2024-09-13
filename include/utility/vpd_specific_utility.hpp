#pragma once

#include "config.h"

#include "constants.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "parser_factory.hpp"
#include "parser_interface.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <utility/common_utility.hpp>
#include <utility/dbus_utility.hpp>
#include <utility/json_utility.hpp>

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
 * @brief Read keyword value.
 *
 * API can be used to read VPD keyword from the given input path.
 *
 * To read keyword of type IPZ, input parameter for reading should be in the
 * form of (Record, Keyword). Eg: ("VINI", "SN").
 *
 * To read keyword from keyword type VPD, just keyword name has to be
 * supplied in the input parameter. Eg: ("SN").
 *
 * @param[in] i_fruPath - EEPROM path.
 * @param[in] i_paramsToReadData - Input details.
 *
 * @throw
 * sdbusplus::xyz::openbmc_project::Common::Device::Error::ReadFailure.
 *
 * @return On success returns the read value in variant of array of bytes.
 * On failure throws exception.
 */
inline types::DbusVariantType
    readKeyword(const nlohmann::json& i_sysCfgJsonObj,
                const types::Path i_fruPath,
                const types::ReadVpdParams& i_paramsToReadData)
{
    try
    {
        std::error_code ec;

        // Check if given path is filesystem path
        if (!std::filesystem::exists(i_fruPath, ec) && (ec))
        {
            throw std::runtime_error("Given file path " + i_fruPath +
                                     " not found.");
        }

        logging::logMessage("Performing VPD read on " + i_fruPath);

        std::shared_ptr<vpd::Parser> l_parserObj =
            std::make_shared<vpd::Parser>(i_fruPath, i_sysCfgJsonObj);

        std::shared_ptr<vpd::ParserInterface> l_vpdParserInstance =
            l_parserObj->getVpdParserInstance();

        return (
            l_vpdParserInstance->readKeywordFromHardware(i_paramsToReadData));
    }
    catch (const std::exception& e)
    {
        logging::logMessage(e.what() +
                            std::string(". VPD read operation failed for ") +
                            i_fruPath);
        throw types::DeviceError::ReadFailure();
    }
}

/**
 * @brief API to update keyword's value on hardware
 *
 * @param[in] i_fruPath - FRU path.
 * @param[in] i_sysCfgJsonObj - JSON object.
 * @param[in] i_paramsToWriteData - Data required to perform write.
 *
 * @return On success returns number of bytes written. On failure returns
 * -1.
 */
inline int
    updateKeywordOnHardware(const types::Path& i_fruPath,
                            const nlohmann::json& i_sysCfgJsonObj,
                            const types::WriteVpdParams& i_paramsToWriteData)
{
    try
    {
        std::shared_ptr<Parser> l_parserObj =
            std::make_shared<Parser>(i_fruPath, i_sysCfgJsonObj);

        std::shared_ptr<ParserInterface> l_vpdParserInstance =
            l_parserObj->getVpdParserInstance();

        return (
            l_vpdParserInstance->writeKeywordOnHardware(i_paramsToWriteData));
    }
    catch (const std::exception& l_error)
    {
        // TODO : Log PEL
        return -1;
    }
}

/**
 * @brief Update keyword value.
 *
 * This API is used to update keyword value on the given input path and its
 * redundant path(s) if any taken from system config JSON.
 *
 * To update IPZ type VPD, input parameter for writing should be in the form
 * of (Record, Keyword, Value). Eg: ("VINI", "SN", {0x01, 0x02, 0x03}).
 *
 * To update Keyword type VPD, input parameter for writing should be in the
 * form of (Keyword, Value). Eg: ("PE", {0x01, 0x02, 0x03}).
 *
 * @param[in] i_sysCfgJsonObj - System config JSON object.
 * @param[in] i_vpdPath - Path (inventory object path/FRU EEPROM path).
 * @param[in] i_paramsToWriteData - Input details.
 *
 * @return On success returns number of bytes written, on failure returns
 * -1.
 */
inline int updateKeyword(const nlohmann::json& i_sysCfgJsonObj,
                         const types::Path& i_vpdPath,
                         const types::WriteVpdParams& i_paramsToWriteData)
{
    if (i_vpdPath.empty())
    {
        logging::logMessage("Given VPD path is empty.");
        return -1;
    }

    types::Path l_fruPath = i_vpdPath;
    types::Path l_inventoryObjPath;
    types::Path l_redundantFruPath;

    if (!i_sysCfgJsonObj.empty())
    {
        try
        {
            // Get hardware path from system config JSON.
            const types::Path& l_tempPath =
                jsonUtility::getFruPathFromJson(i_sysCfgJsonObj, i_vpdPath);

            if (!l_tempPath.empty())
            {
                // Save the FRU path to update on hardware
                l_fruPath = l_tempPath;

                // Get inventory object path from system config JSON
                l_inventoryObjPath = jsonUtility::getInventoryObjPathFromJson(
                    i_sysCfgJsonObj, i_vpdPath);

                // Get redundant hardware path if present in system config JSON
                l_redundantFruPath =
                    jsonUtility::getRedundantEepromPathFromJson(i_sysCfgJsonObj,
                                                                i_vpdPath);
            }
        }
        catch (const std::exception& e)
        {
            return -1;
        }
    }

    // Update keyword's value on hardware
    int l_bytesUpdatedOnHardware = updateKeywordOnHardware(
        l_fruPath, i_sysCfgJsonObj, i_paramsToWriteData);

    if (l_bytesUpdatedOnHardware == -1)
    {
        return l_bytesUpdatedOnHardware;
    }

    // If inventory D-bus object path is present, perform update
    if (!l_inventoryObjPath.empty())
    {
        types::Record l_recordName;
        std::string l_interfaceName;
        std::string l_propertyName;
        types::DbusVariantType l_keywordValue;

        if (const types::IpzData* l_ipzData =
                std::get_if<types::IpzData>(&i_paramsToWriteData))
        {
            l_recordName = std::get<0>(*l_ipzData);
            l_interfaceName = constants::ipzVpdInf + l_recordName;
            l_propertyName = std::get<1>(*l_ipzData);

            try
            {
                // Read keyword's value from hardware to write the same on
                // D-bus.
                l_keywordValue = readKeyword(
                    i_sysCfgJsonObj, l_fruPath,
                    types::ReadVpdParams(
                        std::make_tuple(l_recordName, l_propertyName)));
            }
            catch (const std::exception& l_exception)
            {
                // TODO: Log PEL
                // Unable to read keyword's value from hardware.
                return -1;
            }
        }
        else
        {
            // Input parameter type provided isn't compatible to perform update.
            return -1;
        }

        // Create D-bus object map
        types::ObjectMap l_dbusObjMap = {std::make_pair(
            l_inventoryObjPath,
            types::InterfaceMap{std::make_pair(
                l_interfaceName, types::PropertyMap{std::make_pair(
                                     l_propertyName, l_keywordValue)})})};

        // Call PIM's Notify method to perform update
        if (!dbusUtility::callPIM(std::move(l_dbusObjMap)))
        {
            // Call to PIM's Notify method failed.
            return -1;
        }
    }

    // Update keyword's value on redundant hardware if present
    if (!l_redundantFruPath.empty())
    {
        int l_bytesUpdatedOnRedundantHw = updateKeywordOnHardware(
            l_redundantFruPath, i_sysCfgJsonObj, i_paramsToWriteData);

        if (l_bytesUpdatedOnRedundantHw == -1)
        {
            return l_bytesUpdatedOnRedundantHw;
        }
    }

    // TODO: Check if revert is required when any of the writes fails.
    // TODO: Handle error logging

    // All updates are successful.
    return l_bytesUpdatedOnHardware;
}
} // namespace vpdSpecificUtility
} // namespace vpd
