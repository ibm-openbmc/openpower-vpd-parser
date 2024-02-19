#include "config.h"

#include "utils.hpp"

#include "constants.hpp"
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

void getVpdDataInVector(std::fstream& vpdFileStream,
                        const std::string& vpdFilePath,
                        types::BinaryVector& vpdVector, size_t& vpdStartOffset)
{
    try
    {
        vpdFileStream.open(vpdFilePath,
                           std::ios::in | std::ios::out | std::ios::binary);
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
        std::cerr << "Stream file size = " << vpdFileStream.gcount()
                  << std::endl;
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

std::optional<bool> isPresent(const nlohmann::json& i_pasredConfigJson,
                              const std::string& i_vpdFilePath)

{
    if ((i_pasredConfigJson["frus"][i_vpdFilePath].at(0)).contains("presence"))
    {
        if (((i_pasredConfigJson["frus"][i_vpdFilePath].at(0)["presence"])
                 .contains("pin")) &&
            ((i_pasredConfigJson["frus"][i_vpdFilePath].at(0)["presence"])
                 .contains("value")))
        {
            // get the pin name
            const std::string& l_presencePinName =
                i_pasredConfigJson["frus"][i_vpdFilePath].at(
                    0)["presence"]["pin"];

            // get the pin value
            uint8_t l_presencePinValue =
                i_pasredConfigJson["frus"][i_vpdFilePath].at(
                    0)["presence"]["value"];

            try
            {
                gpiod::line l_presenceLine =
                    gpiod::find_line(l_presencePinName);

                if (!l_presenceLine)
                {
                    logging::logMessage("Presence line not found for " +
                                        l_presencePinName);

                    /*throw GpioException(
                        "Couldn't find the presence line for the "
                        "GPIO. Skipping this GPIO action.");*/
                }

                l_presenceLine.request({"Read the presence line",
                                        gpiod::line_request::DIRECTION_INPUT,
                                        0});

                uint8_t l_gpioData = l_presenceLine.get_value();

                return (l_gpioData == l_presencePinValue);
            }
            catch (const std::exception& ex)
            {
                std::string errMsg = ex.what();
                errMsg += " GPIO : " + l_presencePinName;

                if ((i_pasredConfigJson["frus"][i_vpdFilePath].at(
                         0)["presence"])
                        .contains("gpioI2CAddress"))
                {
                    errMsg += " i2cBusAddress: " +
                              std::string(
                                  i_pasredConfigJson["frus"][i_vpdFilePath].at(
                                      0)["presence"]["gpioI2CAddress"]);
                }

                // throw GpioException(errMsg);
            }
        }
        else
        {
            logging::logMessage(
                "Config JSON missing required information to detect presence for file " +
                i_vpdFilePath);

            return false;
        }
    }
    return std::optional<bool>{};
}

void executePostFailAction(const nlohmann::json& i_pasredConfigJson,
                           const std::string& i_vpdFilePath)
{
    if (!(i_pasredConfigJson["frus"][i_vpdFilePath].at(0).contains(
            "postActionFail")))
    {
        logging::logMessage(
            "Post fail action flag missing in config JSON. Abort processing");
        return;
    }

    uint8_t l_pinValue = 0;
    std::string l_pinName;

    for (const auto& postAction :
         (i_pasredConfigJson["frus"][i_vpdFilePath].at(0))["postActionFail"]
             .items())
    {
        if (postAction.key() == "pin")
        {
            l_pinName = postAction.value();
        }
        else if (postAction.key() == "value")
        {
            // Get the value to set
            l_pinValue = postAction.value();
        }
    }

    logging::logMessage("Setting GPIO: " + l_pinName + " to " +
                        std::string((int)l_pinValue));

    try
    {
        gpiod::line l_outputLine = gpiod::find_line(l_pinName);

        if (!l_outputLine)
        {
            /* throw GpioException(
                 "Couldn't find output line for the GPIO. Skipping "
                 "this GPIO action.");*/
        }
        outputLine.request(
            {"Disable line", ::gpiod::line_request::DIRECTION_OUTPUT, 0},
            l_pinValue);
    }
    catch (const std::exception& ex)
    {
        std::string errMsg = ex.what();

        if ((i_pasredConfigJson["frus"][i_vpdFilePath]
                 .at(0)["postActionFail"]
                 .contains("gpioI2CAddress")))
        {
            errMsg += " GPIO: " + l_pinName + " i2cBusAddress: " +
                      std::string(i_pasredConfigJson["frus"][i_vpdFilePath].at(
                          0)["postActionFail"]["gpioI2CAddress"]);
        }

        // throw GpioException(errMsg);
    }

    return;
}

bool executePreAction(const nlohmann::json& i_pasredConfigJson,
                      const std::string& i_vpdFilePath)
{
    auto present = isPresent(i_pasredConfigJson, i_vpdFilePath);

    if (present && !present.value())
    {
        return false;
    }

    if ((i_pasredConfigJson["frus"][i_vpdFilePath].at(0)).contains("preAction"))
    {
        if (((i_pasredConfigJson["frus"][i_vpdFilePath].at(0)["preAction"])
                 .contains("pin")) &&
            ((i_pasredConfigJson["frus"][i_vpdFilePath].at(0)["preAction"])
                 .contains("value")))
        {
            const std::string& l_pinName =
                i_pasredConfigJson["frus"][i_vpdFilePath].at(
                    0)["preAction"]["pin"];

            // Get the value to set
            uint8_t l_pinValue = i_pasredConfigJson["frus"][i_vpdFilePath].at(
                0)["preAction"]["value"];

            logging::logMessage("Setting GPIO: " + l_pinName + " to " +
                                std::string((int)l_pinValue));
            try
            {
                gpiod::line l_outputLine = gpiod::find_line(l_pinName);

                if (!l_outputLine)
                {
                    logging::logMessage(
                        "Couldn't find the line for output pin - " + l_pinName);

                    /*throw GpioException(
                        "Couldn't find output line for the GPIO. "
                        "Skipping this GPIO action.");*/
                }

                outputLine.request({"FRU pre-action",
                                    ::gpiod::line_request::DIRECTION_OUTPUT, 0},
                                   l_pinValue);
            }
            catch (const std::exception& ex)
            {
                std::string errMsg = ex.what();
                errMsg += " GPIO : " + l_pinName;

                if ((i_pasredConfigJson["frus"][i_vpdFilePath].at(
                         0)["preAction"])
                        .contains("gpioI2CAddress"))
                {
                    errMsg += " i2cBusAddress: " +
                              std::string(
                                  i_pasredConfigJson["frus"][i_vpdFilePath].at(
                                      0)["preAction"]["gpioI2CAddress"]);
                }

                // Take failure postAction
                executePostFailAction(i_pasredConfigJson, i_vpdFilePath);
                // throw GpioException(errMsg);
            }
        }
        else
        {
            logging::logMessage(
                "Config JSON missing required information for path = " +
                i_vpdFilePath + "Executing post fail action");

            return false;
        }
    }
    else
    {
        logging::logMessage("Pre action not defined for FRU " + i_vpdFilePath);
    }
    return true;
}
} // namespace utils
} // namespace vpd
