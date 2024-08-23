#include "event_logger.hpp"

#include "logger.hpp"

#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/server.hpp>

namespace vpd
{
const std::unordered_map<types::SeverityType, std::string>
    EventLogger::m_severityMap = {
        {types::SeverityType::Notice,
         "xyz.openbmc_project.Logging.Entry.Level.Notice"},
        {types::SeverityType::Informational,
         "xyz.openbmc_project.Logging.Entry.Level.Informational"},
        {types::SeverityType::Debug,
         "xyz.openbmc_project.Logging.Entry.Level.Debug"},
        {types::SeverityType::Warning,
         "xyz.openbmc_project.Logging.Entry.Level.Warning"},
        {types::SeverityType::Critical,
         "xyz.openbmc_project.Logging.Entry.Level.Critical"},
        {types::SeverityType::Emergency,
         "xyz.openbmc_project.Logging.Entry.Level.Emergency"},
        {types::SeverityType::Alert,
         "xyz.openbmc_project.Logging.Entry.Level.Alert"},
        {types::SeverityType::Error,
         "xyz.openbmc_project.Logging.Entry.Level.Error"}};

const std::unordered_map<types::ErrorType, std::string>
    EventLogger::m_errorMsgMap = {
        {types::ErrorType::DefaultValue, "com.ibm.VPD.Error.DefaultValue"},
        {types::ErrorType::InvalidVpdMessage, "com.ibm.VPD.Error.InvalidVPD"},
        {types::ErrorType::VpdMismatch, "com.ibm.VPD.Error.Mismatch"},
        {types::ErrorType::InvalidEeprom,
         "com.ibm.VPD.Error.InvalidEepromPath"},
        {types::ErrorType::EccCheckFailed, "com.ibm.VPD.Error.EccCheckFailed"},
        {types::ErrorType::JsonFailure, "com.ibm.VPD.Error.InvalidJson"},
        {types::ErrorType::DbusFailure, "com.ibm.VPD.Error.DbusFailure"},
        {types::ErrorType::InvalidSystem,
         "com.ibm.VPD.Error.UnknownSystemType"},
        {types::ErrorType::EssentialFru,
         "com.ibm.VPD.Error.RequiredFRUMissing"},
        {types::ErrorType::GpioError, "com.ibm.VPD.Error.GPIOError"}};

const std::unordered_map<types::CalloutPriority, std::string>
    EventLogger::m_priorityMap = {{types::CalloutPriority::High, "H"},
                                  {types::CalloutPriority::Medium, "M"},
                                  {types::CalloutPriority::MediumGroupA, "A"},
                                  {types::CalloutPriority::MediumGroupB, "B"},
                                  {types::CalloutPriority::MediumGroupC, "C"},
                                  {types::CalloutPriority::Low, "L"}};

void EventLogger::createAsyncPelWithInventoryCallout(
    const types::ErrorType i_errorType, const types::SeverityType i_severity,
    const std::vector<types::InventoryCalloutData>& i_callouts,
    const std::string& i_fileName, const std::string& i_funcName,
    const std::string& i_internalRc,
    const std::optional<std::pair<std::string, std::string>> i_userData1,
    const std::optional<std::pair<std::string, std::string>> i_userData2,
    const std::optional<std::string> i_symFru,
    const std::optional<std::string> i_procedure)
{
    // TODO, implementation needs to be added.
    (void)i_errorType;
    (void)i_severity;
    (void)i_callouts;
    (void)i_fileName;
    (void)i_funcName;
    (void)i_internalRc;
    (void)i_userData1;
    (void)i_userData2;
    (void)i_symFru;
    (void)i_procedure;
}

void EventLogger::createAsyncPelWithI2cDeviceCallout(
    const types::ErrorType i_errorType, const types::SeverityType i_severity,
    const std::vector<types::DeviceCalloutData>& i_callouts,
    const std::string& i_fileName, const std::string& i_funcName,
    const std::string& i_internalRc,
    const std::optional<std::pair<std::string, std::string>> i_userData1,
    const std::optional<std::pair<std::string, std::string>> i_userData2)
{
    // TODO, implementation needs to be added.
    (void)i_errorType;
    (void)i_severity;
    (void)i_callouts;
    (void)i_fileName;
    (void)i_funcName;
    (void)i_internalRc;
    (void)i_userData1;
    (void)i_userData2;
}

void EventLogger::createAsyncPelWithI2cBusCallout(
    const types::ErrorType i_errorType, const types::SeverityType i_severity,
    const std::vector<types::I2cBusCalloutData>& i_callouts,
    const std::string& i_fileName, const std::string& i_funcName,
    const std::string& i_internalRc,
    const std::optional<std::pair<std::string, std::string>> i_userData1,
    const std::optional<std::pair<std::string, std::string>> i_userData2)
{
    // TODO, implementation needs to be added.
    (void)i_errorType;
    (void)i_severity;
    (void)i_callouts;
    (void)i_fileName;
    (void)i_funcName;
    (void)i_internalRc;
    (void)i_userData1;
    (void)i_userData2;
}
} // namespace vpd
