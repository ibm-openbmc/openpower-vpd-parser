#include "handler.hpp"

#include "logger.hpp"
#include "utils.hpp"

#include <filesystem>
#include <fstream>
#include "config.h"

namespace vpd
{

void Handler::processAndPublishVPD(
    std::string_view vpdFilePath,
    std::variant<types::ParsedVPD, types::KeywordVpdMap>& vpdMap)
{
    logging::logMessage(std::string("Processing file = ") + vpdFilePath.data());

    (void)vpdMap;
    if (vpdFilePath.empty())
    {
        logging::logMessage("Empty file path. Unable to process.");
        return;
    }

    if (!std::filesystem::exists(vpdFilePath))
    {
        logging::logMessage("File path could not be found.");
        return;
    }

    // if system VPD is being processed
    if (!vpdFilePath.compare(SYSTEM_VPD_FILE_PATH))
    {
        // if current power on state is "on".
        if ("xyz.openbmc_project.State.Chassis.PowerState.On" ==
            utils::getChassisPowerState())
        {
            logging::logMessage(
                "Chassis is in power on state, system VPD can't be collected");
            return;
        }
    }

    // TODO: Populate the VPD data on DBus. Call The API from here.
}

void Handler::getVPDMap(
    std::string_view vpdFilePath,
    std::variant<types::ParsedVPD, types::KeywordVpdMap>& vpdMap)
{
    logging::logMessage(std::string("Parsing file = ") + vpdFilePath.data());

    // TODO: once the caller is implemented, this can be removed.
    (void)vpdMap;

    if (vpdFilePath.empty())
    {
        logging::logMessage("Empty file path. Unable to process.");
        return;
    }

    uint32_t vpdStartOffset = 0;
    types::BinaryVector vpdVector;
    getVpdDataInVector(vpdFilePath.data(), vpdVector, vpdStartOffset);

    // TODO: This code can be uncommented when factory code goes in.
    /*
    ParserInterface* parser = ParserFactory::getParser(
        vpdVector, (pimPath + baseFruInventoryPath), file, vpdStartOffset);
    vpdMap = parser->parse();*/
}

void Handler::getVpdDataInVector(const std::string& vpdFilePath,
                                 types::BinaryVector& vpdVector,
                                 uint32_t& vpdStartOffset)
{
    // check if offset present.
    for (const auto& item : m_inventoryJson["frus"][vpdFilePath])
    {
        if (item.find("offset") != item.end())
        {
            vpdStartOffset = item["offset"];
        }
    }

    auto vpdSizeToRead = std::min(std::filesystem::file_size(vpdFilePath),
                                  static_cast<uintmax_t>(65504));

    std::ifstream vpdFile(vpdFilePath);
    if (vpdFile)
    {
        vpdVector.resize(vpdSizeToRead);
        vpdFile.seekg(vpdStartOffset, std::ios_base::beg);
        vpdFile.read(reinterpret_cast<char*>(&vpdVector[0]), vpdSizeToRead);

        if (!vpdFile)
        {
            logging::logMessage("Failed to read complete data. Data read = " +
                                std::to_string(vpdFile.gcount()));
        }

        vpdVector.resize(vpdFile.gcount());

        // Make sure we reset the EEPROM pointer to a "safe" location if it was
        // DIMM SPD that we just read.
        for (const auto& item : m_inventoryJson["frus"][vpdFilePath])
        {
            if (item.find("extraInterfaces") != item.end())
            {
                if (item["extraInterfaces"].find(
                        "xyz.openbmc_project.Inventory.Item.Dimm") !=
                    item["extraInterfaces"].end())
                {
                    // moves the EEPROM pointer to 2048 'th byte.
                    vpdFile.seekg(2047, std::ios::beg);

                    // Read that byte and discard - to affirm the move
                    // operation.
                    char ch;
                    vpdFile.read(&ch, sizeof(ch));
                    break;
                }
            }
        }
    }
    else
    {
        logging::logMessage("Stream failed to open VPD file. Path = " +
                            vpdFilePath + " Error no. = " + strerror(errno));
    }
}

} // namespace vpd