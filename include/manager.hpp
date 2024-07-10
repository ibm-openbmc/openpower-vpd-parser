#pragma once

#include "constants.hpp"
#include "types.hpp"
#include "worker.hpp"

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
            const std::shared_ptr<sdbusplus::asio::connection>& asioConnection);

    /**
     * @brief Destructor.
     */
    ~Manager() = default;

    /**
     * @brief Update keyword value.
     *
     * API can be used to update VPD keyword.
     *
     * To update a keyword on cache, input path should be D-bus object path.
     * Eg: ("/xyz/openbmc_project/inventory/system/chassis/motherboard").
     *
     * To update a keyword on hardware, input path should be EEPROM path.
     * Eg:
     * ("/sys/bus/i2c/drivers/at24/9-0052/eeprom").
     *
     * To update a keyword on both cache and hardware, input path should be
     * D-bus object path. Eg:
     * ("/xyz/openbmc_project/inventory/system/chassis/motherboard").
     *
     * To update IPZ type VPD, input data should be in the form of (Record,
     * Keyword, Value). Eg: ("VINI", "SN", {0x01, 0x02, 0x03}).
     *
     * To update Keyword type VPD, input data should be in the form of (Keyword,
     * Value). Eg: ("PE", {0x01, 0x02, 0x03}).
     *
     * The target to update can either be cache/hardware/both, whose values are
     * 0/1/2 respectively.
     *
     * @param[in] i_path - D-bus object path/EEPROM path.
     * @param[in] i_data - Data to be updated.
     * @param[in] i_target - Target location to update (0/1/2).
     *
     * @return On failure this API throws an error, on success it returns
     * nothing.
     */
    void updateKeyword(const types::Path i_path, const types::VpdData i_data,
                       const uint8_t i_target);

    /**
     * @brief Read keyword value.
     *
     * API can be used to read VPD keyword.
     *
     * To read a keyword from cache, input path should be D-bus object path.
     * Eg: ("/xyz/openbmc_project/inventory/system/chassis/motherboard").
     *
     * To read a keyword from hardware, input path should be EEPROM path.
     * Eg: ("/sys/bus/i2c/drivers/at24/9-0052/eeprom").
     *
     * To read keyword from IPZ type VPD, input data should be in the form of
     * (Record, Keyword). Eg: ("VINI", "SN").
     *
     * To read keyword from keyword type VPD, just keyword name has to be
     * supplied as input. Eg: ("SN").
     *
     * The target to read can either be cache/hardware, whose values are 0/1
     * respectively.
     *
     * @param[in] i_path - D-bus object path/EEPROM path.
     * @param[in] i_data - Data to be read.
     * @param[in] i_target - Target location to read from (0/1).
     *
     * @return Read value in array of bytes.
     */
    types::BinaryVector readKeyword(const types::Path i_path,
                                    const types::VpdData i_data,
                                    const uint8_t i_target);

    /**
     * @brief Collect single FRU VPD
     * API can be used to perform VPD collection for the given FRU.
     * The FRU should have concurrentlyMaintainable flag set to true in VPD JSON
     * to execute this action.
     *
     * @param[in] i_dbusObjPath - D-bus object path
     */
    void collectSingleFruVpd(
        const sdbusplus::message::object_path& i_dbusObjPath);

    /**
     * @brief Delete single FRU VPD
     * API can be used to perform VPD deletion for the given FRU.
     *
     * @param[in] i_dbusObjPath - D-bus object path
     */
    void deleteSingleFruVpd(
        const sdbusplus::message::object_path& i_dbusObjPath);

    /**
     * @brief Get expanded location code.
     *
     * API to get expanded location code from the unexpanded location code.
     *
     * @param[in] i_unexpandedLocationCode - Unexpanded location code.
     * @param[in] i_nodeNumber - Denotes the node in case of a multi-node
     * configuration, defaulted to zero incase of single node system.
     *
     * @throw xyz.openbmc_project.Common.Error.InvalidArgument for
     * invalid argument.
     *
     * @return Location code of the FRU.
     */
    std::string getExpandedLocationCode(
        const std::string& i_unexpandedLocationCode,
        [[maybe_unused]] const uint16_t i_nodeNumber = 0);

    /**
     * @brief Get D-Bus object path of FRUs from expanded location code.
     *
     * An API to get list of FRU D-Bus object paths for a given expanded
     * location code.
     *
     * @param[in] i_expandedLocationCode - Expanded location code.
     *
     * @throw xyz.openbmc_project.Common.Error.InvalidArgument for
     * invalid argument.
     *
     * @return List of FRUs D-Bus object paths for the given location code.
     */
    types::ListOfPaths getFrusByExpandedLocationCode(
        const std::string& i_expandedLocationCode);

    /**
     * @brief Get D-Bus object path of FRUs from unexpanded location code.
     *
     * An API to get list of FRU D-Bus object paths for a given unexpanded
     * location code.
     *
     * @param[in] i_unexpandedLocationCode - Unexpanded location code.
     * @param[in] i_nodeNumber - Denotes the node in case of a multi-node
     * configuration, defaulted to zero incase of single node system.
     *
     * @throw xyz.openbmc_project.Common.Error.InvalidArgument for
     * invalid argument.
     *
     * @return List of FRUs D-Bus object paths for the given location code.
     */
    types::ListOfPaths getFrusByUnexpandedLocationCode(
        const std::string& i_unexpandedLocationCode,
        [[maybe_unused]] const uint16_t i_nodeNumber = 0);

    /**
     * @brief Get Hardware path
     * API can be used to get EEPROM path for the given inventory path.
     *
     * @param[in] i_dbusObjPath - D-bus object path
     *
     * @return Corresponding EEPROM path.
     */
    std::string getHwPath(const sdbusplus::message::object_path& i_dbusObjPath);

    /**
     * @brief  Perform VPD recollection
     * This api will trigger parser to perform VPD recollection for FRUs that
     * can be replaced at standby.
     */
    void performVPDRecollection();

    /**
     * @brief Get unexpanded location code.
     *
     * An API to get unexpanded location code and node number from expanded
     * location code.
     *
     * @param[in] i_expandedLocationCode - Expanded location code.
     *
     * @throw xyz.openbmc_project.Common.Error.InvalidArgument for
     * invalid argument.
     *
     * @return Location code in unexpanded format and its node number.
     */
    std::tuple<std::string, uint16_t>
        getUnexpandedLocationCode(const std::string& i_expandedLocationCode);

  private:
#ifdef IBM_SYSTEM
    /**
     * @brief API to set timer to detect system VPD over D-Bus.
     *
     * System VPD is required before bus name for VPD-Manager is claimed. Once
     * system VPD is published, VPD for other FRUs should be collected. This API
     * detects id system VPD is already published on D-Bus and based on that
     * triggers VPD collection for rest of the FRUs.
     *
     * Note: Throws exception in case of any failure. Needs to be handled by the
     * caller.
     */
    void SetTimerToDetectSVPDOnDbus();

    /**
     * @brief Set timer to detect and set VPD collection status for the system.
     *
     * Collection of FRU VPD is triggered in a separate thread. Resulting in
     * multiple threads at  a given time. The API creates a timer which on
     * regular interval will check if all the threads were collected back and
     * sets the status of the VPD collection for the system accordingly.
     *
     * @throw std::runtime_error
     */
    void SetTimerToDetectVpdCollectionStatus();
#endif

    /**
     * @brief An api to check validity of unexpanded location code.
     *
     * @param[in] i_locationCode - Unexpanded location code.
     *
     * @return True/False based on validity check.
     */
    bool isValidUnexpandedLocationCode(const std::string& i_locationCode);

    // Shared pointer to asio context object.
    const std::shared_ptr<boost::asio::io_context>& m_ioContext;

    // Shared pointer to Dbus interface class.
    const std::shared_ptr<sdbusplus::asio::dbus_interface>& m_interface;

    // Shared pointer to bus connection.
    const std::shared_ptr<sdbusplus::asio::connection>& m_asioConnection;

    // Shared pointer to worker class
    std::shared_ptr<Worker> m_worker;
};

} // namespace vpd
