#include <gpioMonitor.hpp>

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
    // ToDo Add implementation.
    (void)i_ioContext;
}
} // namespace vpd
