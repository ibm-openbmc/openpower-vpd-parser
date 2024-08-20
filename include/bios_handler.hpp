#pragma once
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>

namespace vpd
{

/**
 * @brief Interface class for BIOS handling.
 *
 * The class layout has the virtual methods required to be implemented by any
 * concrete class that intends to use the feature provided via BIOS handler
 * class.
 */
class BiosHandlerInterface
{
  public:
    /**
     * @brief API to back up or restore BIOS attributes.
     *
     * Concrete class should implement the API and read the backed up data from
     * its designated location and take a call if it should be backed up or
     * restored.
     */
    virtual void backUpOrRestoreBiosAttributes() = 0;

    /**
     * @brief Callback API to be triggered on BIOS attribute change.
     *
     * Concrete class should implement the API to extract the attribute and its
     * value from DBus message broadcasted on BIOS attribute change.
     * The definition should be overridden in concrete class to deal with BIOS
     * attributes interested in.
     *
     * @param[in] i_msg - The callback message.
     */
    virtual void biosAttributesCallback(sdbusplus::message_t& i_msg) = 0;
};

/**
 * @brief IBM specifc BIOS handler class.
 */
class IbmBiosHandler : public BiosHandlerInterface
{
  public:
    /**
     * @brief API to back up or restore BIOS attributes.
     *
     * The API will read the backed up data from the VPD keyword and based on
     * its value, either backs up or restores the data.
     */
    virtual void backUpOrRestoreBiosAttributes();

    /**
     * @brief Callback API to be triggered on BIOS attribute change.
     *
     * The API to extract the required attribute and its value from DBus message
     * broadcasted on BIOS attribute change.
     *
     * @param[in] i_msg - The callback message.
     */
    virtual void biosAttributesCallback(sdbusplus::message_t& i_msg);
};

/**
 * @brief A class to operate upon BIOS attributes.
 *
 * The class along with specific BIOS handler class(es), provides a feature
 * where specific BIOS attributes identified by the concrete specific class can
 * be listened for any change and can be backed up to a desired location or
 * restored back to the BIOS table.
 *
 * To use the feature, "BiosHandlerInterface" should be implemented by a
 * concrete class and the same should be used to instantiate BiosHandler.
 *
 * This class registers call back to listen to PLDM service as it is being used
 * for reading/writing BIOS attributes.
 *
 * The feature can be used in a factory reset scenario where backed up values
 * can be used to restore BIOS.
 *
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
        m_specificBiosHandler = std::make_shared<T>();
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

    // Reference to the connection.
    const std::shared_ptr<sdbusplus::asio::connection>& m_asioConn;

    // shared pointer to specific BIOS handler.
    std::shared_ptr<T> m_specificBiosHandler;
};
} // namespace vpd
