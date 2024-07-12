#pragma once

#include "parser_factory.hpp"
#include "parser_interface.hpp"
#include "types.hpp"

#include <string.h>

#include <nlohmann/json.hpp>

#include <iostream>

namespace vpd
{
/**
 * @brief Class to implement a wrapper around concrete parser class.
 * The class based on VPD file passed, selects the required parser and exposes
 * API to parse the VPD and return the parsed data in required format to the
 * caller.
 */
class Parser
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] vpdFilePath - Path to the VPD file.
     * @param[in] parsedJson - Parsed JSON.
     */
    Parser(const std::string& vpdFilePath, nlohmann::json parsedJson);

    /**
     * @brief API to implement a generic parsing logic.
     *
     * This API is called to select parser based on the vpd data extracted from
     * the VPD file path passed to the constructor of the class.
     * It further parses the data based on the parser selected and returned
     * parsed map to the caller.
     */
    types::VPDMapVariant parse();

    /**
     * @brief API to get parser instance based on VPD type.
     *
     * This API detects the VPD type based on the data passed to the constructor
     * of the class and returns the respective parser instance.
     *
     * @return Parser instance.
     */
    std::shared_ptr<vpd::ParserInterface> getVpdParserInstance();

  private:
    // holds offfset to VPD if applicable.
    size_t m_vpdStartOffset = 0;

    // Path to the VPD file
    const std::string& m_vpdFilePath;

    // Path to configuration file, can be empty.
    nlohmann::json m_parsedJson;

}; // parser
} // namespace vpd
