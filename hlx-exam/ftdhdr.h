#ifndef __FTDPKT_H__
#define __FTDPKT_H__

#include <stdint.h>

// prevent structure alignment by compiler
#pragma pack(push)
#pragma pack(1)

struct FTDHeader {
    uint8_t FTDType;
    uint8_t FTDExtLength;
    uint16_t FTDDataLength;
};

struct FTDExtHeader {
    uint8_t FTDTag;
    uint8_t FTDTagLength;
    //uint8_t *FTDTagData;
};

struct FTDCHeader {
    uint8_t Version;
    uint32_t TID;
    uint8_t Chain;
    uint16_t SequenceSeries;
    uint32_t SequenceNumber;
    uint16_t FieldCount;
    uint16_t FTDCContentLength;
};

struct FTDField {
    uint32_t FieldId;
    uint16_t FieldLength;
    uint8_t *DataItem;
};

#pragma pack(pop)

enum FTDType_t {
    FTDTypeNone = 0x00,
    FTDTypeFTDC = 0x01,
    FTDTypeCompressed = 0x02,
};

enum FTDTag_t {
    FTDTagNone = 0x00,
    FTDTagDatetime = 0x01,
    FTDTagCompressedMethod = 0x02,
    FTDTagSessionState = 0x03,
    FTDTagKeepAlive = 0x04,
    FTDTagTradedate = 0x05,
    FTDTagTarget = 0x06,
};

struct FTD {

    struct FTDHeader header;

    // optional ExtHeader and data
    struct FTDExtHeader *extHeader;
    uint8_t *ftdTagData;

    // optional FTDC header and fields data
    struct FTDCHeader *ftdcHeader;

    std::vector<struct FTDField> fields;
};


#endif //__FTDPKT_H__
