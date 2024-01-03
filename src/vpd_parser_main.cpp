#include "logger.hpp"
#include "parser.hpp"
#include "parser_interface.hpp"
#include "types.hpp"
#include "utils.hpp"

#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <parser_factory.hpp>

/**
 * @brief This file implements a generic parser APP.
 *
 * It recieves path of the VPD file(mandatory) and path to a config
 * file(optional) as arguments. It will parse the data and return parsed data in
 * a required format.
 *
 * Steps to get parsed VPD.
 * - Pass VPD file path and config file (if applicable).
 * - Read VPD file to vector.
 * - Pass that to parser_factory to get the parser and call parse API on that
 * parser object to get the Parsed VPD map.
 * - If VPD format is other than the existing formats. Follow the steps
 * - a) Add logic in parser_factory.cpp, vpdTypeCheck API to detect the format.
 * - b) Implement a custom parser class.
 * - c) Override parse API in the newly added parser class.
 * - d) Add type of parsed data returned by parse API into types.hpp,
 * "VPDMapVariant".
 *
 */

int main(int argc, char** argv)
{
    try
    {
        std::string vpdFilePath{};
        CLI::App app{"VPD-parser-app - APP to parse VPD. "};

        app.add_option("-f, --file", vpdFilePath, "VPD file path")->required();

        std::string configFilePath{};
#ifdef PARSER_USE_JSON
        app.add_option("-c,--config", configFilePath, "Path to JSON config");
        vpd::logging::logMessage("Config file path recieved" + configFilePath);
#endif

        CLI11_PARSE(app, argc, argv);

        vpd::logging::logMessage("VPD file path recieved" + vpdFilePath);

        // VPD file path is a mandatory parameter to execute any parser.
        if (vpdFilePath.empty())
        {
            throw std::runtime_error("Empty VPD file path");
        }

        // VPD file path should exist in the system for parser to work on it.
        if (!std::filesystem::exists(vpdFilePath))
        {
            throw std::runtime_error("VPD file path does not exist");
        }

        nlohmann::json json;
        if (!configFilePath.empty())
        {
            json = vpd::utils::getParsedJson(configFilePath);
        }

        // Instantiate generic parser and call parse to get parsed data based on
        // VPD type.
        std::shared_ptr<vpd::Parser> vpdParser =
            std::make_shared<vpd::Parser>(vpdFilePath, json);
        vpd::types::VPDMapVariant parsedVpdDataMap = vpdParser->parse();

        // Custom logic to be implemented based on the type of variant,
        //  eg: for IPZ VPD format
        if (auto ipzVpdMap =
                std::get_if<vpd::types::IPZVpdMap>(&parsedVpdDataMap))
        {
            // get rid of unused variable warning/error
            (void)ipzVpdMap;
            // implement code that needs to handle parsed IPZ VPD.
        }
    }
    catch (const std::exception& ex)
    {
        vpd::logging::logMessage(ex.what());
        return -1;
    }

    return 0;
}