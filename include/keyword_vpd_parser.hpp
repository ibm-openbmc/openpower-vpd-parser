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
    KeywordVpd() = delete;
    KeywordVpd(const KeywordVpd&) = delete;
    KeywordVpd(KeywordVpd&&) = delete;
    ~KeywordVpd() = default;

    /**
     * @brief Constructor
     *
     * Move kwVpdVector to parser object's kwVpdVector
     */
    KeywordVpd(const types::BinaryVector& kwVpdVector) :
        keywordVpdVector(kwVpdVector)
    {
    }

    /**
     * @brief Parse the keyword VPD binary data.
     *
     * Calls the sub functions to emplace the keyword-value pairs in map and to
     * validate certain tags and checksum data.
     *
     * @return map of keyword:value
     */
    std::variant<types::ParsedData, types::KeywordVpdMap> parse();

  private:
    /*Iterator pointing to checksum start byte*/
    types::BinaryVector::const_iterator checkSumStart;

    /*Iterator pointing to checksum end byte*/
    types::BinaryVector::const_iterator checkSumEnd;

    /*Iterator to vpd data*/
    types::BinaryVector::const_iterator vpdIterator;

    /*Vector of keyword VPD data*/
    const types::BinaryVector& keywordVpdVector;

    /**
     * @brief Parsing keyword-value pairs and emplace into Map.
     *
     * @return map of keyword:value
     */
    types::KeywordVpdMap parser();

    /**
     * @brief Validate checksum.
     *
     * Finding the 2's complement of sum of all the
     * keywords,values and large resource identifier string.
     */
    void validateChecksum();

    /**
     * @brief Get the size of the data
     *
     * @return size of the data
     */
    inline size_t getKwDataSize()
    {
        return (*(vpdIterator + 1) << 8 | *vpdIterator);
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
     */
    void checkNextBytesValidity(uint8_t numberOfBytes);
};
} // namespace vpd
