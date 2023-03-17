#pragma once

#include <sdbusplus/asio/object_server.hpp>

namespace vpd
{
/**
 * @brief Class to manage VPD processing.
 *
 * The class is responsible to implement methods to manage VPD on the system.
 * It also implements methods to be exposed over D-Bus required to access/edit
 * VPD data.
 */
class Manager
{
  public:
    /**
     * List of deleted methods.
     */
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;

    /**
     * @brief Constructor.
     *
     * @param[in] ioCon - IO context.
     * @param[in] iFace - interface to implement.
     * @param[in] connection - Dbus Connection.
     */
    Manager(const std::shared_ptr<boost::asio::io_context>& ioCon,
            const std::shared_ptr<sdbusplus::asio::dbus_interface>& iFace,
            const std::shared_ptr<sdbusplus::asio::connection>& conn);

    /**
     * @brief Destructor.
     */
    ~Manager() = default;

  private:
    /**
     * @brief An API to select device tree and JSON.
     *
     * This API based on certain parameters choose corresponding device tree and
     * JSON based on the system.
     *
     * Note: API throws multiple exceptions in case of error(s). Caller need to
     * implement exception handling accordingly.
     */
    void selectDeviceTreeAndJson();

    /**
     * @brief API to read "fitconfig" envoronment variable.
     *
     * The API is required to detect if correct devTRee w.r.t system has been
     * set or not.
     *
     * @return Value of fitconfig.
     */
    std::string readFitConfigValue();

    /**
     * @brief Set environment variable and then reboot the system.
     *
     * @param[in] key   -env key to set new value
     * @param[in] value -value to set.
     */
    void setEnvAndReboot(const string& key, const string& value);

    /**
     * @brief API to check if motherboard path is on D-Bus.
     *
     * @param[in] mboardPath - D-Bus object path of Motherboard FRU.
     * @return bool - Depending on path found by object mapper on D-Bus.
     */
    bool motherboardPathOnDBus(std::string_view mboardPath);

    /**
     * @brief API to get system specific JSON name.
     *
     * This API reads the IM and HW keyword value from VPA map and returns JSON
     * name for that specific system.
     *
     * @param[out] systemJson - Absolute path of the system specific JSON.
     */
    void getSystemJson(std::string& systemJson);

    /**
     * @brief An API to get parsed VPD map.
     *
     * The api can be called to get map of parsed VPD data in the format as
     * defined by the type of returned map.
     *
     * Note: Empty map is returned in case of any error. Caller responsibility
     * to handle accordingly.
     *
     * @param[in] vpdFilePath - EEPROM path of FRU whose data needs to be
     * parsed.
     * @param[out] vpdMap - Parsed VPD map.
     */
    void Manager::getVPDMap(std::string_view vpdFilePath,
                            VPDMapVariant& vpdMap);

    /**
     * @brief API to read value of HW keyword.
     *
     * @param[in] vpdMap - Parsed VPD map.
     * @return Value of HW keyword.
     */
    std::string getHW(const types::ParsedVPD& vpdMap);

    /**
     * @brief API to read value of IM keyword.
     *
     * @param[in] vpdMap - Parsed VPD map.
     * @return Value of IM keyword.
     */
    std::string getIM(const types::ParsedVPD& vpdMap);

    // Shared pointer to asio context object.
    const std::shared_ptr<boost::asio::io_context>& m_ioContext;

    // Shared pointer to Dbus interface class.
    const std::shared_ptr<sdbusplus::asio::dbus_interface>& m_interface;

    // Shared pointer to bus connection.
    const std::shared_ptr<sdbusplus::asio::connection>& m_conn;

    // Parsed JSON file
    nlohmann::json m_parsedJson{};
};

} // namespace vpd