#pragma once
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>

namespace vpd
{

class BiosHandlerInterface
{
  public:
    virtual void backUpOrRestoreBiosAttributes() = 0;
};

class IbmBiosHandler : public BiosHandlerInterface
{
  public:
    /**
     * @brief API to back up or restore BIOS attributes.
     *
     * The API reads the required BIOS attribute and VPD keywords. Based on
     * value read, the attributes are either backed up in VPD keyword or BIOS
     * attribute is restored.
     */
    void backUpOrRestoreBiosAttributes();

    void biosAttribsCallback(sdbusplus::message_t& i_msg);
};

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
template <typename T>
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
        specificBiosHandler = std::make_shared<T>();
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

    /**
     * @brief Register listener for BIOS attribute property change.
     *
     * The VPD manager needs to listen for property change of certain BIOS
     * attributes that are backed in VPD. When the attributes change, the new
     * value is written back to the VPD keywords that backs them up.
     */
    void listenBiosAttributes();

    /**
     * @brief Callback for BIOS Attribute change.
     *
     * Checks if the BIOS attribute(s) changed are those backed up by VPD. If
     * yes, it will update the VPD with the new attribute value.
     *
     * @param[in] i_msg - The callback message.
     */
    // void biosAttribsCallback(sdbusplus::message_t& i_msg);

    // Reference to the connection.
    const std::shared_ptr<sdbusplus::asio::connection>& m_asioConn;

    // shared pointer to specific BIOS handler.
    std::shared_ptr<T> specificBiosHandler;
};
} // namespace vpd
