#pragma once

#include "tool_utils.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace vpd
{
/**
 * @brief Class to support operations on VPD.
 *
 * The class provides API's to,
 * Read keyword value from DBus/hardware.
 * Update keyword value to DBus/hardware.
 * Dump DBus object's critical information.
 * Fix system VPD if any mismatch between DBus and hardware data.
 * Reset specific system VPD keywords to its default value.
 * Force VPD collection for hardware.
 */
class VpdTool
{
  private:
    /**
     * @brief Get specific properties of a FRU in JSON format.
     *
     * For a given object path of a FRU, this API returns the following
     * properties of the FRU in JSON format:
     * - Pretty Name, Location Code, Sub Model
     * - SN, PN, CC, FN, DR keywords under VINI record.
     *
     * @param[in] i_objectPath - DBus object path
     *
     * @return On success, returns the properties of the FRU in JSON format,
     * otherwise returns an empty JSON.
     * If FRU's "Present" property is false, this API returns an empty JSON.
     * Note: The caller of this API should handle empty JSON.
     *
     * @throw json::exception
     */
    nlohmann::json getFruProperties(const std::string& i_objectPath) const;

    /**
     * @brief Get any Inventory Property in JSON.
     *
     * API to get any property of a FRU in JSON format. Given an object path,
     * interface and property name, this API does a D-Bus read property on PIM
     * to get the value of that property and returns it in JSON format. This API
     * returns empty JSON in case of failure. The caller of the API must check
     * for empty JSON.
     *
     * @param[in] i_objectPath - DBus object path
     *
     * @param[in] i_interface - Interface name
     *
     * @param[in] i_propertyName - Property name
     *
     * @return On success, returns the property and its value in JSON format,
     * otherwise return empty JSON.
     * {"SN" : "ABCD"}
     */
    template <typename PropertyType>
    nlohmann::json getInventoryPropertyJson(
        const std::string& i_objectPath, const std::string& i_interface,
        const std::string& i_propertyName) const noexcept;

    /**
     * @brief API to get properties [SN, PN, CC, FN, DR] under VINI interface of
     * PIM in JSON format.
     *
     * API to get properties [SN, PN, CC, FN, DR] under VINI interface in JSON
     * format. Given an object path, this API does a D-Bus read property on PIM
     * to get the value of properties [SN, PN, CC, FN, DR] under VINI interface
     * and returns it in JSON format. This API returns empty JSON in case of
     * failure. The caller of the API must check for empty JSON.
     *
     * @param[in] i_objectPath - DBus object path.
     *
     * @return On success, returns JSON output with the above properties under
     * VINI interface, otherwise returns empty JSON.
     */
    nlohmann::json
        getVINIPropertiesJson(const std::string& i_objectPath) const noexcept;

    /**
     * @brief Get the "type" property for a FRU.
     *
     * Given a FRU path, and parsed System Config JSON, this API returns the
     * "type" property for the FRU in JSON format. This API gets
     * these properties from Phosphor Inventory Manager.
     *
     * @param[in] i_objectPath - DBus object path.
     *
     * @return On success, returns the "type" property in JSON
     * format, otherwise returns empty JSON. The caller of this API should
     * handle empty JSON.
     */
    nlohmann::json
        getFruTypeProperty(const std::string& i_objectPath) const noexcept;

    /**
     * @brief Check if a FRU is present in the system.
     *
     * Given a FRU's object path, this API checks if the FRU is present in the
     * system by reading the "Present" property of the FRU.
     *
     * @param[in] i_objectPath - DBus object path.
     *
     * @return true if FRU's "Present" property is true, false otherwise.
     */
    bool isFruPresent(const std::string& i_objectPath) const noexcept;

  public:
    /**
     * @brief Read keyword value.
     *
     * API to read VPD keyword's value from the given input path.
     * If the provided i_onHardware option is true, read keyword's value from
     * the hardware. Otherwise read keyword's value from DBus.
     *
     * @param[in] i_vpdPath - DBus object path or EEPROM path.
     * @param[in] i_recordName - Record name.
     * @param[in] i_keywordName - Keyword name.
     * @param[in] i_onHardware - True if i_vpdPath is EEPROM path, false
     * otherwise.
     * @param[in] i_fileToSave - File path to save keyword's value, if not given
     * result will redirect to a console.
     *
     * @return On success return 0, otherwise return -1.
     */
    int readKeyword(const std::string& i_vpdPath,
                    const std::string& i_recordName,
                    const std::string& i_keywordName, const bool i_onHardware,
                    const std::string& i_fileToSave = {});

    /**
     * @brief Dump the given inventory object in JSON format to console.
     *
     * For a given object path of a FRU, this API dumps the following properties
     * of the FRU in JSON format to console:
     * - Pretty Name, Location Code, Sub Model
     * - SN, PN, CC, FN, DR keywords under VINI record.
     * If the FRU's "Present" property is not true, the above properties are not
     * dumped to console.
     *
     * @param[in] i_fruPath - DBus object path.
     *
     * @return On success returns 0, otherwise returns -1.
     */
    int dumpObject(const std::string& i_fruPath) const noexcept;
};
} // namespace vpd
