#pragma once

#include <string>

namespace vpd
{
/**
 * @brief Class to support read and write VPD.
 *
 * The class provides API's to read/update keyword value on DBUS and on the
 * EEPROM file path. And also provides API's to dump DBus object critical
 * information.
 */
class VpdTool
{
  public:
    /**
     * @brief Read keyword value.
     *
     * API to read VPD keyword's value from the given input path.
     * If the provided path is EEPROM path, it reads keyword value from the
     * hardware by invoking "ReadKeyword" method on "com.ibm.VPD.Manager"
     * service. Othrwise if the provided path is DBus path, it reads keyword
     * value from DBus.
     *
     * @param[in] i_fruPath - DBus object path or EEPROM path.
     * @param[in] i_recordName - Record name.
     * @param[in] i_keyword - Keyword name.
     * @param[in] i_onHardware - True if i_fruPath is EEPROM path, false
     * otherwise.
     *
     */
    void readKeyword(const std::string& i_fruPath,
                     const std::string& i_recordName,
                     const std::string& i_keyword, bool i_onHardware);
};
} // namespace vpd
