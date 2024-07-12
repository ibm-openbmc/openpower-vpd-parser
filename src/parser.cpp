#include "parser.hpp"

#include <utility/generic_utility.hpp>
#include <utility/json_utility.hpp>

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
    genericUtility::getVpdDataInVector(m_vpdFilePath, m_vpdVector,
                                       m_vpdStartOffset);

    // This will detect the type of parser required.
    std::shared_ptr<vpd::ParserInterface> parser =
        ParserFactory::getParser(m_vpdVector, m_vpdFilePath, m_vpdStartOffset);

    return parser;
}

types::VPDMapVariant Parser::parse()
{
    std::shared_ptr<vpd::ParserInterface> parser = getVpdParserInstance();
    return parser->parse();
}

} // namespace vpd
