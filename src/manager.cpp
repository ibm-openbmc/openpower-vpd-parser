#include "config.h"

#include "manager.hpp"

#include "logger.hpp"

#include <boost/asio/steady_timer.hpp>

namespace vpd
{
Manager::Manager(const std::shared_ptr<boost::asio::io_context>& ioCon,
                 const std::shared_ptr<sdbusplus::asio::dbus_interface>& iFace,
                 const std::shared_ptr<sdbusplus::asio::connection>& conn) :
    m_ioContext(ioCon),
    m_interface(iFace), m_conn(conn)
{
    try
    {
#ifdef IBM_SYSTEM
        m_worker = std::make_shared<Worker>();

        // Set up minimal things that is needed before bus name is claimed.
        m_worker->performInitialSetup();

        // set timer to detect system VPD. This is required so that bus claim
        // and event loop is not blocked while all FRU VPD is being collected.
        setTimerForSystemVPDDetection(ioCon);
#endif
    }
    catch (const std::exception& e)
    {
        logging::logMessage("VPD-Manager service failed.");
        throw;
    }
}

#ifdef IBM_SYSTEM
void Manager::setTimerForSystemVPDDetection()
{
    static boost::asio::steady_timer timer(*ioCon);

    // timer for 2 seconds
    auto asyncCancelled = timeout.expires_after(std::chrono::seconds(2)));

    (asyncCancelled == 0) ? std::cout << "Timer started" << std::endl
                          : std::cout << "Timer re-started" << std::endl;

    timer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            throw std::runtime_error("Timer to detect system VPD was aborted");
        }

        if (ec)
        {
            logging::logMessage("Timer wait failed to trigger VPD collection");
            throw std::runtime_error(
                "Timer to defect System VPD collection failed");
        }

        if (m_worker->isSystemVPDOnDBus())
        {
            // clear the timer
            timer.cancel();
            m_worker->processAllFRU();
        }
    });
}
#endif
} // namespace vpd