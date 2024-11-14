#pragma once

#include "tool_types.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

#include <iostream>

namespace vpd
{
namespace utils
{
/**
 * @brief An API to read property from Dbus.
 *
 * API reads the property value for the specified interface and object path from
 * the given Dbus service.
 *
 * The caller of the API needs to validate the validity and correctness of the
 * type and value of data returned. The API will just fetch and return the data
 * without any data validation.
 *
 * Note: It will be caller's responsibility to check for empty value returned
 * and generate appropriate error if required.
 *
 * @param[in] i_serviceName - Name of the Dbus service.
 * @param[in] i_objectPath - Object path under the service.
 * @param[in] i_interface - Interface under which property exist.
 * @param[in] i_property - Property whose value is to be read.
 *
 * @return - Value read from Dbus.
 *
 * @throw std::runtime_error
 */
inline types::DbusVariantType readDbusProperty(const std::string& i_serviceName,
                                               const std::string& i_objectPath,
                                               const std::string& i_interface,
                                               const std::string& i_property)
{
    types::DbusVariantType l_propertyValue;

    // Mandatory fields to make a dbus call.
    if (i_serviceName.empty() || i_objectPath.empty() || i_interface.empty() ||
        i_property.empty())
    {
        // TODO: Enable logging when verbose is enabled.
        /*std::cout << "One of the parameter to make Dbus read call is empty."
                  << std::endl;*/
        throw std::runtime_error("Empty Parameter");
    }

    try
    {
        auto l_bus = sdbusplus::bus::new_default();
        auto l_method =
            l_bus.new_method_call(i_serviceName.c_str(), i_objectPath.c_str(),
                                  "org.freedesktop.DBus.Properties", "Get");
        l_method.append(i_interface, i_property);

        auto result = l_bus.call(l_method);
        result.read(l_propertyValue);
    }
    catch (const sdbusplus::exception::SdBusError& l_ex)
    {
        // TODO: Enable logging when verbose is enabled.
        // std::cout << std::string(l_ex.what()) << std::endl;
        throw std::runtime_error(std::string(l_ex.what()));
    }
    return l_propertyValue;
}

/**
 * @brief An API to print json data on stdout.
 *
 * @param[in] i_jsonData - JSON object.
 */
inline void printJson(const nlohmann::json& i_jsonData)
{
    try
    {
        std::cout << i_jsonData.dump(constants::INDENTATION) << std::endl;
    }
    catch (const nlohmann::json::type_error& l_ex)
    {
        throw std::runtime_error("Failed to dump JSON data, error: " +
                                 std::string(l_ex.what()));
    }
}

/**
 * @brief An API to convert binary value into ascii/hex representation.
 *
 * If given data contains printable characters, ASCII formated string value of
 * the input data will be returned. Otherwise if the data has any non-printable
 * value, returns the hex represented value of the given data in string format.
 *
 * @param[in] i_keywordValue - Data in binary format.
 *
 * @throw - Throws std::bad_alloc or std::terminate in case of error.
 *
 * @return - Returns the converted string value.
 */
inline std::string getPrintableValue(const types::BinaryVector& i_keywordValue)
{
    bool l_allPrintable =
        std::all_of(i_keywordValue.begin(), i_keywordValue.end(),
                    [](const auto& l_byte) { return std::isprint(l_byte); });

    std::ostringstream l_oss;
    if (l_allPrintable)
    {
        l_oss << std::string(i_keywordValue.begin(), i_keywordValue.end());
    }
    else
    {
        l_oss << "0x";
        for (const auto& l_byte : i_keywordValue)
        {
            l_oss << std::setfill('0') << std::setw(2) << std::hex
                  << static_cast<int>(l_byte);
        }
    }

    return l_oss.str();
}

/**
 * @brief API to read keyword's value from hardware.
 *
 * This API reads keyword's value by requesting DBus service(vpd-manager) who
 * hosts the 'ReadKeyword' method to read keyword's value.
 *
 * @param[in] i_eepromPath - EEPROM file path.
 * @param[in] i_paramsToReadData - Property whose value has to be read.
 *
 * @return - Value read from hardware
 *
 * @throw std::runtime_error, sdbusplus::exception::SdBusError
 */
inline types::DbusVariantType
    readKeywordFromHardware(const std::string& i_eepromPath,
                            const types::ReadVpdParams i_paramsToReadData)
{
    if (i_eepromPath.empty())
    {
        throw std::runtime_error("Empty EEPROM path");
    }

    try
    {
        types::DbusVariantType l_propertyValue;

        auto l_bus = sdbusplus::bus::new_default();

        auto l_method = l_bus.new_method_call(
            constants::vpdManagerService, constants::vpdManagerObjectPath,
            constants::vpdManagerInfName, "ReadKeyword");

        l_method.append(i_eepromPath, i_paramsToReadData);
        auto l_result = l_bus.call(l_method);

        l_result.read(l_propertyValue);

        return l_propertyValue;
    }
    catch (const sdbusplus::exception::SdBusError& l_error)
    {
        throw;
    }
}
} // namespace utils
} // namespace vpd
