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
     * For a given Object Path of a FRU, this API returns the following
     * properties of the FRU in JSON format:
     * - Present property, Pretty Name, Location Code, Sub Model
     * - SN, PN, CC, FN, DR keywords under VINI record.
     * - type and TYPE of the FRU
     *
     * @param[in] i_objectPath - DBus object path
     *
     * @return On success, returns the properties of the FRU in JSON format,
     * otherwise returns an empty JSON object.
     * Note: The caller of this API should
     * check for empty JSON object
     *
     * @throw std::runtime_error
     */
    nlohmann::json getFruPropertiesJson(const std::string& i_objectPath) const;

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
     * @brief Get the "type" and "TYPE" properties for a FRU.
     *
     * Given a FRU path, and parsed System Config JSON, this API returns the
     * "type" and "TYPE" properties for the FRU in JSON format. This API gets
     * these properties from the System Config JSON.
     *
     * @param[in] i_eepromPath - EEPROM path for a FRU.
     * @param[in] i_sysConfigJson - Parsed System Config JSON.
     *
     * @return On success, returns the "TYPE" and "type" properties in JSON
     * format. "type" property is emplaced only if the same is present in the
     * System Config JSON. On failure, returns empty JSON.
     */
    nlohmann::json getFruTypeProperty(
        const std::string& i_eepromPath,
        const nlohmann::json& i_sysCfgJsonObj) const noexcept;

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
     * For a given Object Path of a FRU, this API dumps the following properties
     * of the FRU in JSON format to console:
     * - Present property, Pretty Name, Location Code, Sub Model
     * - SN, PN, CC, FN, DR keywords under VINI record.
     * - type and TYPE of the FRU
     *
     * @param[in] i_fruPath - DBus object path.
     *
     * @return On success returns 0, otherwise returns -1.
     */
    int dumpObject(const std::string& i_fruPath) const noexcept;
};
} // namespace vpd
