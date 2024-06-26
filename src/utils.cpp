#include "config.h"

#include "utils.hpp"

#include "constants.hpp"
#include "exceptions.hpp"
#include "logger.hpp"

#include <gpiod.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <regex>

namespace vpd
{
namespace utils
{
// Function pointers to process tags from config JSON.
typedef bool (*functionPtr)(const nlohmann::json& i_parsedConfigJson,
                            const std::string& i_vpdFilePath,
                            const std::string& i_baseAction,
                            const std::string& i_flagToProcess);

std::unordered_map<std::string, functionPtr> funcionMap{
    {"gpioPresence", processGpioPresenceTag},
    {"setGpio", procesSetGpioTag},
    {"systemCmd", processSystemCmdTag}};

types::MapperGetObject getObjectMap(const std::string& objectPath,
                                    std::span<const char*> interfaces)
{
    types::MapperGetObject getObjectMap;

    // interface list is optional argument, hence no check required.
    if (objectPath.empty())
    {
        logging::logMessage("Path value is empty, invalid call to GetObject");
        return getObjectMap;
    }

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                                          "/xyz/openbmc_project/object_mapper",
                                          "xyz.openbmc_project.ObjectMapper",
                                          "GetObject");
        method.append(objectPath, interfaces);
        auto result = bus.call(method);
        result.read(getObjectMap);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        logging::logMessage(e.what());
    }

    return getObjectMap;
}

types::DbusVariantType readDbusProperty(const std::string& serviceName,
                                        const std::string& objectPath,
                                        const std::string& interface,
                                        const std::string& property)
{
    types::DbusVariantType propertyValue;

    // Mandatory fields to make a read dbus call.
    if (serviceName.empty() || objectPath.empty() || interface.empty() ||
        property.empty())
    {
        logging::logMessage(
            "One of the parameter to make Dbus read call is empty.");
        return propertyValue;
    }

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method =
            bus.new_method_call(serviceName.c_str(), objectPath.c_str(),
                                "org.freedesktop.DBus.Properties", "Get");
        method.append(interface, property);

        auto result = bus.call(method);
        result.read(propertyValue);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        logging::logMessage(e.what());
    }
    return propertyValue;
}

void writeDbusProperty(const std::string& serviceName,
                       const std::string& objectPath,
                       const std::string& interface,
                       const std::string& property,
                       const types::DbusVariantType& propertyValue)
{
    // Mandatory fields to make a write dbus call.
    if (serviceName.empty() || objectPath.empty() || interface.empty() ||
        property.empty())
    {
        logging::logMessage(
            "One of the parameter to make Dbus read call is empty.");

        // caller need to handle the throw to ensure Dbus write success.
        throw std::runtime_error("Dbus write failed, Parameter empty");
    }

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method =
            bus.new_method_call(serviceName.c_str(), objectPath.c_str(),
                                "org.freedesktop.DBus.Properties", "Set");
        method.append(interface, property, propertyValue);
        bus.call(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        logging::logMessage(e.what());

        // caller needs to handle this throw to handle error in writing Dbus.
        throw std::runtime_error("Dbus write failed");
    }
}

std::string generateBadVPDFileName(const std::string& vpdFilePath)
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

void dumpBadVpd(const std::string& vpdFilePath,
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

void getKwVal(const types::IPZKwdValueMap& kwdValueMap, const std::string& kwd,
              std::string& kwdValue)
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

bool callPIM(types::ObjectMap&& objectMap)
{
    try
    {
        std::array<const char*, 1> pimInterface = {constants::pimIntf};

        auto mapperObjectMap = getObjectMap(constants::pimPath, pimInterface);

        if (!mapperObjectMap.empty())
        {
            auto bus = sdbusplus::bus::new_default();
            auto pimMsg = bus.new_method_call(
                mapperObjectMap.begin()->first.c_str(), constants::pimPath,
                constants::pimIntf, "Notify");
            pimMsg.append(std::move(objectMap));
            bus.call(pimMsg);
        }
        else
        {
            logging::logMessage("Mapper returned empty object map for PIM");
            return false;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        logging::logMessage(e.what());
        return false;
    }
    return true;
}

std::string encodeKeyword(const std::string& keyword,
                          const std::string& encoding)
{
    // Default value is keyword value
    std::string result(keyword.begin(), keyword.end());

    if (encoding == "MAC")
    {
        result.clear();
        size_t firstByte = keyword[0];
        result += toHex(firstByte >> 4);
        result += toHex(firstByte & 0x0f);
        for (size_t i = 1; i < keyword.size(); ++i)
        {
            result += ":";
            result += toHex(keyword[i] >> 4);
            result += toHex(keyword[i] & 0x0f);
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

void insertOrMerge(types::InterfaceMap& map, const std::string& interface,
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

std::string getExpandedLocationCode(const std::string& unexpandedLocationCode,
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
                    getObjectMap("/xyz/openbmc_project/inventory/system/"
                                 "chassis/motherboard",
                                 interfaceList);

                if (mapperRetValue.empty())
                {
                    throw std::runtime_error("Mapper failed to get service");
                }

                const std::string& serviceName =
                    std::get<0>(mapperRetValue.at(0));

                auto retVal =
                    readDbusProperty(serviceName,
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

                retVal =
                    readDbusProperty(serviceName,
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

void getVpdDataInVector(const std::string& vpdFilePath,
                        types::BinaryVector& vpdVector, size_t& vpdStartOffset)
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

size_t getVPDOffset(const nlohmann::json& parsedJson,
                    const std::string& vpdFilePath)
{
    size_t vpdOffset = 0;
    if (!vpdFilePath.empty())
    {
        if (parsedJson["frus"].contains(vpdFilePath))
        {
            for (const auto& item : parsedJson["frus"][vpdFilePath])
            {
                if (item.find("offset") != item.end())
                {
                    vpdOffset = item["offset"];
                    break;
                }
            }
        }
    }
    return vpdOffset;
}

nlohmann::json getParsedJson(const std::string& pathToJson)
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

bool executePostFailAction(const nlohmann::json& i_parsedConfigJson,
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

bool processSystemCmdTag(const nlohmann::json& i_parsedConfigJson,
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
        const std::string& l_bindCommand =
            i_parsedConfigJson["frus"][i_vpdFilePath].at(
                0)[i_baseAction][i_flagToProcess]["systemCmd"]["cmd"];

        logging::logMessage("Bind command = " + l_bindCommand);
        utils::executeCmd(l_bindCommand);
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

bool processGpioPresenceTag(const nlohmann::json& i_parsedConfigJson,
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

bool procesSetGpioTag(const nlohmann::json& i_parsedConfigJson,
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

bool executePreAction(const nlohmann::json& i_parsedConfigJson,
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
} // namespace utils
} // namespace vpd
