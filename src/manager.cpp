#include "config.h"

#include "manager.hpp"

#include "exceptions.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "utils.hpp"

#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/message.hpp>

#include <fstream>

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
#endif

        try
        {
            // Create VPD JSON Object
            m_jsonObj = utils::getParsedJson(INVENTORY_JSON_SYM_LINK);
        }
        catch (...)
        {
            m_jsonObj = nlohmann::json();
        }

        // Register methods under com.ibm.VPD.Manager interface
        iFace->register_method(
            "WriteKeyword",
            [this](const types::Path i_path, const types::VpdData i_data) {
            this->updateKeyword(i_path, i_data);
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
            m_worker->collectFrusFromJson();
        }
    });
}
#endif

int Manager::updateKeyword(types::Path i_path, const types::VpdData i_data)
{
    int sizeUpdated = 0;
    types::PathCollection l_fruPaths;

    const auto isDbusPath = utils::isDbusInvPath(i_path);

    std::string l_invPath = isDbusPath ? i_path : std::string();
    std::string l_primaryHwPath = !isDbusPath ? i_path : std::string();
    std::string l_redundantHwPath{};

    try
    {
        if (utils::getFRUPaths(i_path, l_fruPaths, m_jsonObj))
        {
            l_invPath = std::get<0>(l_fruPaths);
            l_primaryHwPath = std::get<1>(l_fruPaths);
            l_redundantHwPath = std::get<2>(l_fruPaths);
        }
    }
    catch (JsonException& e)
    {
        // Do nothing, proceed to update keyword on the given path.
    }

    try
    {
        // If there is a primary hardware path, update on hardware
        if (!l_primaryHwPath.empty())
        {
            sizeUpdated = utils::updateHardware(l_primaryHwPath, i_data,
                                                m_jsonObj);

            if (sizeUpdated > 0)
            {
                std::cout << "\nData updated successfully on "
                          << l_primaryHwPath << std::endl;
            }
        }

        // If there is a redundant eeprom path, then update
        if (!l_redundantHwPath.empty())
        {
            sizeUpdated = utils::updateHardware(l_redundantHwPath, i_data,
                                                m_jsonObj);

            if (sizeUpdated > 0)
            {
                std::cout << "\nData updated successfully on "
                          << l_redundantHwPath << std::endl;
            }
        }

        // If there is a Dbus path, update on Dbus
        if (!l_invPath.empty())
        {
            sizeUpdated = utils::updateDbus(l_invPath, i_data);

            if (sizeUpdated > 0)
            {
                std::cout << "\nData updated successfully on " << l_invPath
                          << std::endl;
            }
        }

        return sizeUpdated;
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        logging::logMessage(e.what());
        logging::logMessage("D-bus write failed.");
        throw;
    }
    catch (const std::exception& e)
    {
        logging::logMessage(e.what());
        logging::logMessage("D-bus write failed.");
        throw;
    }

    return -1;
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
