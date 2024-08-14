#pragma once

#include "types.hpp"

#include <nlohmann/json.hpp>

#include <iostream>

namespace vpd
{
namespace utils
{
/**
 * @brief An API to read property from Dbus.
 *
 * The caller of the API needs to validate the validatity and correctness of the
 * type and value of data returned. The API will just fetch and retun the data
 * without any data validation.
 *
 * Note: It will be caller's responsibility to check for empty value returned
 * and generate appropriate error if required.
 *
 * @param[in] i_serviceName - Name of the Dbus service.
 * @param[in] i_objectPath - Object path under the service.
 * @param[in] i_interface - Interface under which property exist.
 * @param[in] i_property - Property whose value is to be read.
 * @return - Value read from Dbus on success.
 *           empty variant for any failure.
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
        std::cout << "One of the parameter to make Dbus read call is empty."
                  << std::endl;
        return l_propertyValue;
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
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cout << e.what() << std::endl;
    }
    return l_propertyValue;
}

/**
 * @brief An API to read keyword from hardware through DBus service.
 *
 * @param[in] i_serviceName - Name of the Dbus service hosting 'ReadKeyword'
 * method.
 * @param[in] i_objectPath - Object path under the service.
 * @param[in] i_interface - Interface under which 'ReadKeyword' method exist.
 * @param[in] i_eepromPath - EEPROM file path.
 * @param[in] i_paramsToReadData - Property whose value is to be read.
 * @return - Value read from Dbus on success.
 *          empty variant for any failure's.
 */
inline types::DbusVariantType readKeywordFromHardware(
    const std::string& i_serviceName, const std::string& i_objectPath,
    const std::string& i_interface, const std::string& i_eepromPath,
    const types::ReadVpdParams i_paramsToReadData)
{
    types::DbusVariantType l_propertyValue;

    // Mandatory fields to make a dbus call.
    if (i_serviceName.empty() || i_objectPath.empty() || i_interface.empty() ||
        i_eepromPath.empty())
    {
        std::cout
            << "One of the parameter to make Dbus ReadKeyword call is empty."
            << std::endl;
        return l_propertyValue;
    }

    try
    {
        auto l_bus = sdbusplus::bus::new_default();
        auto l_method =
            l_bus.new_method_call(i_serviceName.c_str(), i_objectPath.c_str(),
                                  i_interface.c_str(), "ReadKeyword");
        l_method.append(i_eepromPath, i_paramsToReadData);
        auto l_result = l_bus.call(l_method);
        l_result.read(l_propertyValue);
    }
    catch (const sdbusplus::exception::SdBusError& l_error)
    {
        std::cout << l_error.what() << std::endl;
    }
    return l_propertyValue;
}

/**
 * @brief An API to print json data on stdout.
 *
 * @param[in] i_output - JSON object.
 */
void printJson(const nlohmann::json& i_output)
{
    std::cout << i_output.dump(4) << std::endl;
}

/**
 * @brief An API to convert hex value to string.
 *
 * If given data contains printable charecters, ASCII formated string value of
 * the input data will be returned. Otherwise if the data has any non-printable
 * value, returns the hex represented string value of the given data.
 *
 * @param[in] i_keywordValue - Date needs to be converted as string.
 * @return - Returns the converted string value.
 */
std::string getPrintableValue(const types::BinaryVector& i_keywordValue)
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
} // namespace utils
} // namespace vpd
