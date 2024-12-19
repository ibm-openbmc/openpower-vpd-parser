#pragma once

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
     * - Present property, Pretty Name, Location Code, Sub Model
     * - SN, PN, CC, FN, DR keywords under VINI record.
     *
     * @param[in] i_fruPath - DBus object path
     *
     * @return On success, returns the properties of the FRU in JSON format,
     * otherwise throws a std::runtime_error exception.
     * Note: The caller of this API should handle this exception.
     *
     * @throw std::runtime_error
     */
    nlohmann::json getFruProperties(const std::string& i_fruPath) const;

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
     * - Present property, Pretty Name, Location Code, Sub Model
     * - SN, PN, CC, FN, DR keywords under VINI record.
     *
     * @param[in] i_fruPath - DBus object path.
     *
     * @return On success returns 0, otherwise returns -1.
     */
    int dumpObject(const std::string& i_fruPath) const noexcept;
};
} // namespace vpd
