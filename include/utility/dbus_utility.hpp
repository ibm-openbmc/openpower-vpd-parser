#pragma once

#include "logger.hpp"
#include "types.hpp"

namespace vpd
{
/**
 * @brief The namespace defines utlity methods for generic D-Bus operations.
 */
namespace dbusUtility
{

/**
 * @brief An API to get Map of service and interfaces for an object path.
 *
 * The API returns a Map of service name and interfaces for a given pair of
 * object path and interface list. It can be used to determine service name
 * which implemets a particular object path and interface.
 *
 * Note: It will be caller's responsibility to check for empty map returned and
 * generate appropriate error.
 *
 * @param [in] objectPath - Object path under the service.
 * @param [in] interfaces - Array of interface(s).
 * @return - A Map of service name to object to interface(s), if success.
 *           If failed,  empty map.
 */
inline types::MapperGetObject getObjectMap(const std::string& objectPath,
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
 * @param [in] serviceName - Name of the Dbus service.
 * @param [in] objectPath - Object path under the service.
 * @param [in] interface - Interface under which property exist.
 * @param [in] property - Property whose value is to be read.
 * @return - Value read from Dbus, if success.
 *           If failed, empty variant.
 */
inline types::DbusVariantType readDbusProperty(const std::string& serviceName,
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

/**
 * @brief An API to write property on Dbus.
 *
 * The caller of this API needs to handle exception thrown by this method to
 * identify any write failure. The API in no other way indicate write  failure
 * to the caller.
 *
 * Note: It will be caller's responsibility ho handle the exception thrown in
 * case of write failure and generate appropriate error.
 *
 * @param [in] serviceName - Name of the Dbus service.
 * @param [in] objectPath - Object path under the service.
 * @param [in] interface - Interface under which property exist.
 * @param [in] property - Property whose value is to be written.
 * @param [in] propertyValue - The value to be written.
 */
inline void writeDbusProperty(const std::string& serviceName,
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

/**
 * @brief API to publish data on PIM
 *
 * The API calls notify on PIM object to publlish VPD.
 *
 * @param[in] objectMap - Object, its interface and data.
 * @return bool - Status of call to PIM notify.
 */
inline bool callPIM(types::ObjectMap&& objectMap)
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

/**
 * @brief API to check if a D-Bus service is running or not.
 *
 * Any failure in calling the method "NameHasOwner" implies that the service is
 * not in a running state. Hence the API returns false in case of any exception
 * as well.
 *
 * @param[in] i_serviceName - D-Bus service name whose status is to be checked.
 * @return bool - True if the service is running, false otherwise.
 */
inline bool isServiceRunning(const std::string& i_serviceName)
{
    bool l_retVal = false;

    try
    {
        auto l_bus = sdbusplus::bus::new_default();
        auto l_method = l_bus.new_method_call(
            "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus", "NameHasOwner");
        l_method.append(i_serviceName);

        l_bus.call(l_method).read(l_retVal);
    }
    catch (const sdbusplus::exception::SdBusError& l_ex)
    {
        logging::logMessage(
            "Call to check service status failed with exception: " +
            std::string(l_ex.what()));
    }

    return l_retVal;
}
} // namespace dbusUtility
} // namespace vpd
