#include "parser.hpp"

#include <utility/dbus_utility.hpp>
#include <utility/json_utility.hpp>
#include <utility/vpd_specific_utility.hpp>

#include <fstream>

namespace vpd
{
Parser::Parser(const std::string& vpdFilePath, nlohmann::json parsedJson) :
    m_vpdFilePath(vpdFilePath), m_parsedJson(parsedJson)
{
    // Read VPD offset if applicable.
    if (!m_parsedJson.empty())
    {
        m_vpdStartOffset = jsonUtility::getVPDOffset(m_parsedJson, vpdFilePath);
    }
}

std::shared_ptr<vpd::ParserInterface> Parser::getVpdParserInstance()
{
    // Read the VPD data into a vector.
    vpdSpecificUtility::getVpdDataInVector(m_vpdFilePath, m_vpdVector,
                                           m_vpdStartOffset);

    // This will detect the type of parser required.
    std::shared_ptr<vpd::ParserInterface> l_parser =
        ParserFactory::getParser(m_vpdVector, m_vpdFilePath, m_vpdStartOffset);

    return l_parser;
}

types::VPDMapVariant Parser::parse()
{
    std::shared_ptr<vpd::ParserInterface> l_parser = getVpdParserInstance();
    return l_parser->parse();
}

int Parser::updateVpdKeyword(const types::WriteVpdParams& i_paramsToWriteData)
{
    // Update keyword's value on hardware
    int l_bytesUpdatedOnHardware = -1;
    try
    {
        std::shared_ptr<ParserInterface> l_vpdParserInstance =
            getVpdParserInstance();
        l_bytesUpdatedOnHardware =
            l_vpdParserInstance->writeKeywordOnHardware(i_paramsToWriteData);
    }
    catch (const std::exception& l_exception)
    {
        logging::logMessage(
            "Error while updating keyword's value on hardware path " +
            m_vpdFilePath + ", error: " + std::string(l_exception.what()));
        // TODO : Log PEL
        return -1;
    }

    auto [l_fruPath, l_inventoryObjPath, l_redundantFruPath] =
        jsonUtility::getAllPathsToUpdateKeyword(m_parsedJson, m_vpdFilePath);

    // If inventory D-bus object path is present, update keyword's value on DBus
    if (!l_inventoryObjPath.empty())
    {
        types::Record l_recordName;
        std::string l_interfaceName;
        std::string l_propertyName;
        types::DbusVariantType l_keywordValue;

        if (const types::IpzData* l_ipzData =
                std::get_if<types::IpzData>(&i_paramsToWriteData))
        {
            l_recordName = std::get<0>(*l_ipzData);
            l_interfaceName = constants::ipzVpdInf + l_recordName;
            l_propertyName = std::get<1>(*l_ipzData);

            try
            {
                // Read keyword's value from hardware to write the same on
                // D-bus.
                std::shared_ptr<ParserInterface> l_vpdParserInstance =
                    getVpdParserInstance();

                logging::logMessage("Performing VPD read on " + m_vpdFilePath);

                l_keywordValue = l_vpdParserInstance->readKeywordFromHardware(
                    types::ReadVpdParams(
                        std::make_tuple(l_recordName, l_propertyName)));
            }
            catch (const std::exception& l_exception)
            {
                // Unable to read keyword's value from hardware.
                logging::logMessage(
                    "Error while reading keyword's value from hadware path " +
                    m_vpdFilePath +
                    ", error: " + std::string(l_exception.what()));
                // TODO: Log PEL
                return -1;
            }
        }
        else
        {
            // Input parameter type provided isn't compatible to perform update.
            logging::logMessage(
                "Input parameter type isn't compatible to update keyword's value on DBus for object path: " +
                l_inventoryObjPath);
            return -1;
        }

        // Get D-bus name for the given keyword
        l_propertyName =
            vpdSpecificUtility::getDbusPropNameForGivenKw(l_propertyName);

        // Create D-bus object map
        types::ObjectMap l_dbusObjMap = {std::make_pair(
            l_inventoryObjPath,
            types::InterfaceMap{std::make_pair(
                l_interfaceName, types::PropertyMap{std::make_pair(
                                     l_propertyName, l_keywordValue)})})};

        // Call PIM's Notify method to perform update
        if (!dbusUtility::callPIM(std::move(l_dbusObjMap)))
        {
            // Call to PIM's Notify method failed.
            logging::logMessage("Notify PIM is failed for object path: " +
                                l_inventoryObjPath);
            return -1;
        }
    }

    // Update keyword's value on redundant hardware if present
    if (!l_redundantFruPath.empty())
    {
        if (updateVpdKeywordOnRedundantPath(l_redundantFruPath,
                                            i_paramsToWriteData) < 0)
        {
            logging::logMessage(
                "Error while updating keyword's value on redundant path " +
                l_redundantFruPath);
            return -1;
        }
    }

    // TODO: Check if revert is required when any of the writes fails.
    // TODO: Handle error logging

    // All updates are successful.
    return l_bytesUpdatedOnHardware;
}

int Parser::updateVpdKeywordOnRedundantPath(
    const std::string& i_fruPath,
    const types::WriteVpdParams& i_paramsToWriteData)
{
    try
    {
        std::shared_ptr<Parser> l_parserObj =
            std::make_shared<Parser>(i_fruPath, m_parsedJson);

        std::shared_ptr<ParserInterface> l_vpdParserInstance =
            l_parserObj->getVpdParserInstance();

        return l_vpdParserInstance->writeKeywordOnHardware(i_paramsToWriteData);
    }
    catch (const std::exception& l_exception)
    {
        logging::logMessage(
            "Error while updating keyword's value on redundant path " +
            i_fruPath + ", error: " + std::string(l_exception.what()));
        // TODO : Log PEL
        return -1;
    }
}

} // namespace vpd
