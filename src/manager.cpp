#include "manager.hpp"

#include "logger.hpp"

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
#endif
    }
    catch (const std::exception& e)
    {
        logging::logMessage("VPD-Manager service failed.");
        throw;
    }
}
} // namespace vpd