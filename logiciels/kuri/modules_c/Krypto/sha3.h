// //////////////////////////////////////////////////////////
// sha3.h
// Copyright (c) 2014,2015 Stephan Brumme. All rights reserved.
// see http://create.stephan-brumme.com/disclaimer.html
//

#pragma once

// #include "hash.h"
#include <string>

// define fixed size integer types
#ifdef _MSC_VER
// Windows
typedef unsigned __int8 uint8_t;
typedef unsigned __int64 uint64_t;
#else
// GCC
#    include <stdint.h>
#endif

#define DETERMINE_TAILLE_BLOC(x) (200 - 2 * ((x) / 8))
#define DETERMINE_TAILLE_HASH(x) (x / 8)

/// compute SHA3 hash
/** Usage:
    SHA3 sha3;
    std::string myHash  = sha3("Hello World");     // std::string
    std::string myHash2 = sha3("How are you", 11); // arbitrary data, 11 bytes

    // or in a streaming fashion:

    SHA3 sha3;
    while (more data available)
      sha3.add(pointer to fresh data, number of new bytes);
    std::string myHash3 = sha3.getHash();
  */
class SHA3 /* : public Hash */ {
  public:
    /// algorithm variants
    enum Bits { Bits224 = 224, Bits256 = 256, Bits384 = 384, Bits512 = 512 };

    /// same as reset()
    explicit SHA3(Bits bits = Bits256);

    /// add arbitrary number of bytes
    void add(const void *data, size_t numBytes);

    /// return latest hash as hex characters
    void getHash(unsigned char *sortie);

    /// restart
    void reset();

  private:
    /// process a full block
    void processBlock(const void *data);
    /// process everything left in the internal buffer
    void processBuffer();

    /// 1600 bits, stored as 25x64 bit, BlockSize is no more than 1152 bits (Keccak224)
    enum { StateSize = 1600 / (8 * 8), MaxBlockSize = DETERMINE_TAILLE_BLOC(224) };

    /// hash
    uint64_t m_hash[StateSize];
    /// size of processed data in bytes
    uint64_t m_numBytes;
    /// block size (less or equal to MaxBlockSize)
    size_t m_blockSize;
    /// valid bytes in m_buffer
    size_t m_bufferSize;
    /// bytes not processed yet
    uint8_t m_buffer[MaxBlockSize];
    /// variant
    Bits m_bits;
};

class SHA3_224bits : public SHA3 {
  public:
    enum {
        HashBytes = DETERMINE_TAILLE_HASH(224),
        BlockSize = DETERMINE_TAILLE_BLOC(224),
    };

    SHA3_224bits() : SHA3(Bits224)
    {
    }
};

class SHA3_256bits : public SHA3 {
  public:
    enum {
        HashBytes = DETERMINE_TAILLE_HASH(256),
        BlockSize = DETERMINE_TAILLE_BLOC(256),
    };

    SHA3_256bits() : SHA3(Bits256)
    {
    }
};

class SHA3_384bits : public SHA3 {
  public:
    enum {
        HashBytes = DETERMINE_TAILLE_HASH(384),
        BlockSize = DETERMINE_TAILLE_BLOC(384),
    };

    SHA3_384bits() : SHA3(Bits384)
    {
    }
};

class SHA3_512bits : public SHA3 {
  public:
    enum {
        HashBytes = DETERMINE_TAILLE_HASH(512),
        BlockSize = DETERMINE_TAILLE_BLOC(512),
    };

    SHA3_512bits() : SHA3(Bits512)
    {
    }
};
