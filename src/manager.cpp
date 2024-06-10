#include "config.h"

#include "manager.hpp"

#include "logger.hpp"

#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/message.hpp>

namespace vpd
{
Manager::Manager(
    const std::shared_ptr<boost::asio::io_context>& ioCon,
    const std::shared_ptr<sdbusplus::asio::dbus_interface>& iFace,
    const std::shared_ptr<sdbusplus::asio::connection>& asioConnection) :
    m_ioContext(ioCon),
    m_interface(iFace), m_asioConnection(asioConnection)
{
    try
    {
#ifdef IBM_SYSTEM
        m_worker = std::make_shared<Worker>(INVENTORY_JSON_DEFAULT);

        // Set up minimal things that is needed before bus name is claimed.
        m_worker->performInitialSetup();

        // set async timer to detect if system VPD is published on D-Bus.
        SetTimerToDetectSVPDOnDbus();

        // set async timer to detect if VPD collection is done.
        SetTimerToDetectVpdCollectionStatus();
#endif

        // Register methods under com.ibm.VPD.Manager interface
        iFace->register_method("WriteKeyword",
                               [this](const types::Path i_path,
                                      const types::VpdData i_data,
                                      const uint8_t i_target) {
            this->updateKeyword(i_path, i_data, i_target);
        });

        iFace->register_method(
            "ReadKeyword",
            [this](const types::Path i_path, const types::VpdData i_data,
                   const uint8_t i_target) -> types::BinaryVector {
            return this->readKeyword(i_path, i_data, i_target);
        });

        iFace->register_method(
            "CollectFRUVPD",
            [this](const sdbusplus::message::object_path& i_dbusObjPath) {
            this->collectSingleFruVpd(i_dbusObjPath);
        });

        iFace->register_method(
            "deleteFRUVPD",
            [this](const sdbusplus::message::object_path& i_dbusObjPath) {
            this->deleteSingleFruVpd(i_dbusObjPath);
        });

        iFace->register_method(
            "GetExpandedLocationCode",
            [this](const sdbusplus::message::object_path& i_dbusObjPath)
                -> std::string {
            return this->getExpandedLocationCode(i_dbusObjPath);
        });

        iFace->register_method(
            "GetHardwarePath",
            [this](const sdbusplus::message::object_path& i_dbusObjPath)
                -> std::string { return this->getHwPath(i_dbusObjPath); });

        iFace->register_method("PerformVPDRecollection",
                               [this]() { this->performVPDRecollection(); });

        // Indicates FRU VPD collection for the system has not started.
        iFace->register_property(
            "CollectionStatus", std::string("NotStarted"),
            sdbusplus::asio::PropertyPermission::readWrite);
    }
    catch (const std::exception& e)
    {
        logging::logMessage("VPD-Manager service failed. " +
                            std::string(e.what()));
        throw;
    }
}

#ifdef IBM_SYSTEM
void Manager::SetTimerToDetectSVPDOnDbus()
{
    static boost::asio::steady_timer timer(*m_ioContext);

    // timer for 2 seconds
    auto asyncCancelled = timer.expires_after(std::chrono::seconds(2));

    (asyncCancelled == 0) ? std::cout << "Timer started" << std::endl
                          : std::cout << "Timer re-started" << std::endl;

    timer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            throw std::runtime_error(
                "Timer to detect system VPD collection status was aborted");
        }

        if (ec)
        {
            throw std::runtime_error(
                "Timer to detect System VPD collection failed");
        }

        if (m_worker->isSystemVPDOnDBus())
        {
            // cancel the timer
            timer.cancel();

            // Triggering FRU VPD collection. Setting status to "In Progress".
            m_interface->set_property("CollectionStatus",
                                      std::string("InProgress"));
            m_worker->collectFrusFromJson();
        }
    });
}

void Manager::SetTimerToDetectVpdCollectionStatus()
{
    static constexpr auto MAX_RETRY = 5;

    static boost::asio::steady_timer l_timer(*m_ioContext);
    static uint8_t l_timerRetry = 0;

    auto l_asyncCancelled = l_timer.expires_after(std::chrono::seconds(3));

    (l_asyncCancelled == 0)
        ? std::cout << "Collection Timer started" << std::endl
        : std::cout << "Collection Timer re-started" << std::endl;

    l_timer.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            throw std::runtime_error(
                "Timer to detect thread collection status was aborted");
        }

        if (ec)
        {
            throw std::runtime_error(
                "Timer to detect thread collection failed");
        }

        if (m_worker->isAllFruCollectionDone())
        {
            // cancel the timer
            l_timer.cancel();
            m_interface->set_property("CollectionStatus",
                                      std::string("Completed"));
        }
        else
        {
            if (l_timerRetry == MAX_RETRY)
            {
                l_timer.cancel();
                logging::logMessage("Taking too long. Some logic needed here?");
            }
            else
            {
                l_timerRetry++;
                logging::logMessage(
                    "Waiting... FRU VPD collection is in progress");
                SetTimerToDetectVpdCollectionStatus();
            }
        }
    });
}
#endif

void Manager::updateKeyword(const types::Path i_path,
                            const types::VpdData i_data, const uint8_t i_target)
{
    // Dummy code to supress unused variable warning.
    std::cout << "\nFRU path " << i_path;
    std::cout << "\nData " << i_data.index();
    std::cout << "\nTarget = " << static_cast<int>(i_target);

    // On success return nothing. On failure throw error.
}

types::BinaryVector Manager::readKeyword(const types::Path i_path,
                                         const types::VpdData i_data,
                                         const uint8_t i_target)
{
    // Dummy code to supress unused variable warning. To be removed.
    std::cout << "\nFRU path " << i_path;
    std::cout << "\nData " << i_data.index();
    std::cout << "\nTarget = " << static_cast<int>(i_target);

    // On success return the value read. On failure throw error.

    return types::BinaryVector();
}

void Manager::collectSingleFruVpd(
    const sdbusplus::message::object_path& i_dbusObjPath)
{
    // Dummy code to supress unused variable warning. To be removed.
    logging::logMessage(std::string(i_dbusObjPath));
}

void Manager::deleteSingleFruVpd(
    const sdbusplus::message::object_path& i_dbusObjPath)
{
    // Dummy code to supress unused variable warning. To be removed.
    logging::logMessage(std::string(i_dbusObjPath));
}

std::string Manager::getExpandedLocationCode(
    const sdbusplus::message::object_path& i_dbusObjPath)
{
    // Dummy code to supress unused variable warning. To be removed.
    logging::logMessage(std::string(i_dbusObjPath));

    return std::string{};
}

std::string
    Manager::getHwPath(const sdbusplus::message::object_path& i_dbusObjPath)
{
    // Dummy code to supress unused variable warning. To be removed.
    logging::logMessage(std::string(i_dbusObjPath));

    return std::string{};
}

void Manager::performVPDRecollection() {}

} // namespace vpd
