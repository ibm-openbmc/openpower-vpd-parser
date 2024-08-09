#pragma once

#include <nlohmann/json.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <vector>

namespace vpd
{
/**
 * @brief class for GPIO event handling.
 *
 * Responsible for detecting events and handling them. It continuously
 * monitors the presence of the FRU. If any attachment or detachment is
 * detected, it enables or disables the FRU's output GPIO and binds or
 * unbinds the driver accordingly.
 */
class GpioEventHandler
{
  public:
    GpioEventHandler() = delete;
    ~GpioEventHandler() = default;
    GpioEventHandler(const GpioEventHandler&) = delete;
    GpioEventHandler& operator=(const GpioEventHandler&) = delete;
    GpioEventHandler(GpioEventHandler&&) = delete;
    GpioEventHandler& operator=(GpioEventHandler&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] i_inputPin - GPIO input pin
     * @param[in] i_inputValue - holds the value of input pin
     * @param[in] i_outputPin - GPIO output pin
     * @param[in] i_outputValue - value of the output pin
     * @param[in] i_inventoryPath - inventory path of the FRU.
     * @param[in] i_ioContext - pointer to the io context object
     */
    GpioEventHandler(
        const std::string& i_presencePin, const bool& i_presenceValue,
        const std::string& i_outputPin, const bool& i_outputValue,
        const std::string& i_inventoryPath,
        const std::shared_ptr<boost::asio::io_context>& i_ioContext) :
        m_presencePin(i_presencePin),
        m_presenceValue(i_presenceValue), m_outputPin(i_outputPin),
        m_outputValue(i_outputValue), m_inventoryPath(i_inventoryPath)
    {
        setEventHandlerForGpioPresence(i_ioContext);
    }

  private:
    /**
     * @brief API to read the GPIO presence pin value.
     *
     *  @returns GPIO presence pin value.
     */
    bool getPresencePinValue();

    /**
     * @brief API to toggle the output GPIO pin.
     *
     * This API toggles the output GPIO pin based on the presence
     * state of the FRU. If there is any change in the presence pin,
     * it updates the output pin and binds or unbinds the driver
     * accordingly.
     */
    void toggleGpio();

    /**
     * @brief An API to set event handler for FRUs GPIO presence.
     *
     * An API to set timer to call event handler to detect GPIO presence
     * of the FRU.
     *
     * @param[in] i_ioContext - pointer to io context object
     */
    void setEventHandlerForGpioPresence(
        const std::shared_ptr<boost::asio::io_context>& i_ioContext);

    // Indicates presence/absence of fru
    const std::string& m_presencePin;

    // Value of GPIO input pin
    const bool& m_presenceValue;

    // GPIO pin to enable if fru is present
    const std::string& m_outputPin;

    // Value to set, to enable the output pin
    const bool& m_outputValue;

    // Inventory path of the FRU
    const std::string& m_inventoryPath;
};

class GpioMonitor
{
  public:
    GpioMonitor() = delete;
    ~GpioMonitor() = default;
    GpioMonitor(const GpioMonitor&) = delete;
    GpioMonitor& operator=(const GpioMonitor&) = delete;
    GpioMonitor(GpioMonitor&&) = delete;
    GpioMonitor& operator=(GpioMonitor&&) = delete;

    /**
     * @brief constructor
     *
     * @param[in] i_sysCfgJsonObj - System config JSON Object.
     * @param[in] i_ioContext - pointer to IO context object.
     */
    GpioMonitor(const nlohmann::json i_sysCfgJsonObj,
                const std::shared_ptr<boost::asio::io_context>& i_ioContext) :
        m_sysCfgJsonObj(i_sysCfgJsonObj)
    {
        if (!m_sysCfgJsonObj.empty())
        {
            initHandlerForGpio(i_ioContext);
        }
    }

  private:
    /**
     * @brief API to instantiate GpioEventHandler for GPIO pins.
     *
     * This API will extract the GPIO information from system config JSON
     * and instantiate event handler for GPIO pins.
     *
     * @param[in] i_ioContext - Pointer to IO context object.
     */
    void initHandlerForGpio(
        const std::shared_ptr<boost::asio::io_context>& i_ioContext);

    // Array of event handlers for all the attachable FRUs.
    std::vector<std::shared_ptr<GpioEventHandler>> m_gpioObjects;

    const nlohmann::json& m_sysCfgJsonObj;
};
} // namespace vpd
