#pragma once

#include "parser_interface.hpp"
#include "types.hpp"

namespace vpd
{

/**
 * @brief Concrete class to implement Keyword VPD parsing
 *
 * The class inherits ParserInterface class and overrides the parser
 * functionality to implement parsing logic for Keyword VPD format.
 */
class KeywordVpdParser : public ParserInterface
{
  public:
    KeywordVpdParser() = delete;
    KeywordVpdParser(const KeywordVpdParser&) = delete;
    KeywordVpdParser(KeywordVpdParser&&) = delete;
    ~KeywordVpdParser() = default;

    /**
     * @brief Constructor
     *
     * Move kwVpdVector to parser object's kwVpdVector
     */
    KeywordVpdParser(const types::BinaryVector& kwVpdVector) :
        m_keywordVpdVector(kwVpdVector)
    {
    }

    /**
     * @brief Parse the keyword VPD binary data.
     *
     * Calls the sub functions to emplace the keyword-value pairs in map and to
     * validate certain tags and checksum data.
     *
     * @error/exception DataException - VPD is not valid
     * @return map of keyword:value
     */
    std::variant<types::ParsedVPD, types::KeywordVpdMap> parse();

  private:
    /**
     * @brief Parsing keyword-value pairs and emplace into Map.
     *
     * @error/exception DataException - VPD data size is 0, check vpd
     * @return map of keyword:value
     */
    types::KeywordVpdMap parser();

    /**
     * @brief Validate checksum.
     *
     * Finding the 2's complement of sum of all the
     * keywords,values and large resource identifier string.
     *
     * @error/exception DataException - checksum invalid, check vpd
     */
    void validateChecksum();

    /**
     * @brief Get the size of the data
     *
     * @return size of the data
     */
    inline size_t getKwDataSize()
    {
        return (*(m_vpdIterator + 1) << 8 | *m_vpdIterator);
    }

    /**
     * @brief Check for given number of bytes validity
     *
     * Check if no.of elements from (begining of the vector) to (iterator +
     * numberOfBytes) is lesser than or equal to the total no.of elements in
     * the vector. This check is performed before advancement of the iterator.
     *
     * @param[in] numberOfBytes - no.of positions the iterator is going to be
     * iterated
     *
     * @error/exception DataException - Truncated vpd data, check VPD.
     */
    void checkNextBytesValidity(uint8_t numberOfBytes);

    /*Iterator pointing to checksum start byte*/
    types::BinaryVector::const_iterator m_checkSumStart;

    /*Iterator pointing to checksum end byte*/
    types::BinaryVector::const_iterator m_checkSumEnd;

    /*Iterator to vpd data*/
    types::BinaryVector::const_iterator m_vpdIterator;

    /*Vector of keyword VPD data*/
    const types::BinaryVector& m_keywordVpdVector;
};
} // namespace vpd
