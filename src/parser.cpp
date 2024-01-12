#include "parser.hpp"

#include "utils.hpp"

#include <fstream>

namespace vpd
{
Parser::Parser(const std::string& vpdFilePath, nlohmann::json parsedJson) :
    m_vpdFilePath(vpdFilePath), m_parsedJson(parsedJson)
{
    // Read VPD offset if applicable.
    if (!m_parsedJson.empty())
    {
        m_vpdStartOffset = utils::getVPDOffset(m_parsedJson, vpdFilePath);
    }
}

types::VPDMapVariant Parser::parse()
{
    std::fstream vpdFileStream;
    vpdFileStream.exceptions(std::ifstream::badbit | std::ifstream::failbit);

    // Read the VPD data into a vector.
    types::BinaryVector vpdVector;
    utils::getVpdDataInVector(vpdFileStream, m_vpdFilePath, vpdVector,
                              m_vpdStartOffset);

    // This will detect the type of parser required.
    std::shared_ptr<vpd::ParserInterface> parser =
        ParserFactory::getParser(vpdVector, m_vpdFilePath, m_vpdStartOffset);

    return parser->parse();
}

} // namespace vpd