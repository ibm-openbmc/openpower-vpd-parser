#include "gpio_monitor.hpp"

#include "logger.hpp"
#include "types.hpp"
#include "utility/json_utility.hpp"

namespace vpd
{
bool GpioEventHandler::getPresencePinValue()
{
    // ToDo Add implementation.
    return true;
}

void GpioEventHandler::toggleGpio()
{
    // ToDo Add implementation.
}

void GpioEventHandler::setEventHandlerForGpioPresence(
    const std::shared_ptr<boost::asio::io_context>& i_ioContext)
{
    // ToDo Add implementation.
    (void)i_ioContext;
}

void GpioMonitor::initHandlerForGpio(
    const std::shared_ptr<boost::asio::io_context>& i_ioContext)
{
    try
    {
        std::vector<types::GpioPollingParameters>
            l_pollingRequiredFrusParamsList =
                jsonUtility::getListOfPollingParamsForFrus(m_sysCfgJsonObj);

        for (const auto& l_pollingParameters : l_pollingRequiredFrusParamsList)
        {
            std::shared_ptr<GpioEventHandler> l_gpioEventHandlerObj =
                std::make_shared<GpioEventHandler>(
                    std::get<0>(l_pollingParameters),
                    std::get<1>(l_pollingParameters),
                    std::get<2>(l_pollingParameters), i_ioContext);

            m_gpioEventHandlerObjects.push_back(l_gpioEventHandlerObj);
        }
    }
    catch (std::exception& l_ex)
    {
        // TODO log PEL for exception.
        logging::logMessage(l_ex.what());
    }
}
} // namespace vpd
