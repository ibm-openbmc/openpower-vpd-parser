#pragma once
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>

namespace vpd
{
/**
 * @brief A class to operate upon and back up some of the BIOS attributes.
 *
 * The class initiates call backs to listen to PLDM service and changes in some
 * of the BIOS attributes.
 * On a factory reset, BIOS attributes are initialized by PLDM. this class waits
 * until PLDM has grabbed a bus name before attempting any syncs.
 *
 * It also initiates API to back up those data in VPD keywords.
 */
class BiosHandler
{
  public:
    // deleted APIs
    BiosHandler() = delete;
    BiosHandler(const BiosHandler&) = delete;
    BiosHandler& operator=(const BiosHandler&) = delete;
    BiosHandler& operator=(BiosHandler&&) = delete;
    ~BiosHandler() = default;

    /**
     * @brief Constructor.
     *
     * @param[in] connection - Asio connection object.
     */
    BiosHandler(
        const std::shared_ptr<sdbusplus::asio::connection>& i_connection) :
        m_asioConn(i_connection)
    {
        checkAndListenPldmService();
    }

  private:
    /**
     * @brief API to check if PLDM service is running and run BIOS sync.
     *
     * This API checks if the PLDM service is running and if yes it will start
     * an immediate sync of BIOS attributes. If the service is not running, it
     * registers a listener to be notified when the service starts so that a
     * restore can be performed.
     */
    void checkAndListenPldmService();

    // Reference to the connection.
    const std::shared_ptr<sdbusplus::asio::connection>& m_asioConn;
};
} // namespace vpd
