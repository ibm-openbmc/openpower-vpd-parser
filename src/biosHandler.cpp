#include "biosHandler.hpp"

#include "constants.hpp"
#include "logger.hpp"

#include <sdbusplus/bus/match.hpp>
#include <utility/dbus_utility.hpp>

namespace vpd
{
void BiosHandler::checkAndListenPldmService()
{
    // Setup a call back match on NameOwnerChanged to determine when PLDM is up.
    static std::shared_ptr<sdbusplus::bus::match_t> l_nameOwnerMatch =
        std::make_shared<sdbusplus::bus::match_t>(
            *m_asioConn,
            sdbusplus::bus::match::rules::nameOwnerChanged(
                constants::pldmServiceName),
            [this](sdbusplus::message_t& l_msg) {
        if (l_msg.is_method_error())
        {
            logging::logMessage(
                "Error in reading PLDM name owner changed signal.");
            return;
        }

        std::string l_name;
        std::string l_newOwner;
        std::string l_oldOwner;

        l_msg.read(l_name, l_oldOwner, l_newOwner);

        if (!l_newOwner.empty() &&
            (l_name.compare(constants::pldmServiceName) ==
             constants::STR_CMP_SUCCESS))
        {
            // TODO: Restore BIOS attribute from here.
            //  We don't need the match anymore
            l_nameOwnerMatch.reset();
        }
    });

    // Based on PLDM service status reset owner match registered above and
    // trigger BIOS attribute sync.
    if (dbusUtility::isServiceRunning(constants::pldmServiceName))
    {
        l_nameOwnerMatch.reset();
        // TODO: retore BIOS attribute from here.
    }
}
} // namespace vpd
