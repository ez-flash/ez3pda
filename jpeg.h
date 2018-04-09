/* A JPEG decompressor, targeted for the GameBoy Advance (although there
 * should be no machine-specific aspects if you disable JPEG_USE_IWRAM
 * and JPEG_MARK_TIME).  On the GBA it consumes, all with slight potential
 * variance:
 *
 * 2648 bytes of IWRAM, temporary
 * 6640 bytes of ROM
 * 4068 bytes of stack space, usually in IWRAM
 * 350 milliseconds for decompressing a representative image
 *
 * Unlike before when IWRAM was permanently used, it's now loaded in just
 * before decompression, allowing you to spend IWRAM on more tools called
 * constantly rather than one you call only once in awhile.  There is no
 * permanent IWRAM usage with jpeg_this library.
 * 
 * It has a low capacitance for unusual JPEGs.  They cannot be progressive,
 * use arithmetic coding, have more than 3 components in a scan, and must be
 * 8-bit.  They can be colour or grayscale, and any component scaling factors
 * are valid (unless if JPEG_HANDLE_ANY_FACTORS is reset, in which cas only
 * 2:1:1 is allowed).  The maximum component scale factors cannot be three.  In
 * general, you'll be all right, but if it doesn't like your input it will not
 * react sensibly.
 * 
 * This code is in the public domain.  JPEG is used for both its standard
 * meaning and for JFIF.
 * 
 * Revision 1: Inflicted stricter warnings, fixed C99-using code, and reduced
 *     allocation footprint (6144 bytes less).
 * Revision 2: Reduced ROM usage by 276 bytes, with the body going to 832 bytes
 *     of IWRAM.  I made it more configurable, particularly in YCbCr->RGB
 *     conversion.  Some brute force ROM usage reduction.
 * Revision 3: Removed all memset, malloc, and free dependencies.  This
 *     increases stack use drastically but also makes it completely
 *     self-sufficient.
 * Revision 4: Saved 3088 bytes of JPEG_Decoder state by exploiting an
 *     allowance of baseline JPEG decoding.  This requires 1544 more bytes of
 *     stack space, however.
 *
 * - Burton Radons (loth@users.sourceforge.net)
 */

#ifndef GBA_IMAGE_JPEG_H
#define GBA_IMAGE_JPEG_H

#define JPEG_MARK_TIME 0
    /**< You cannot set jpeg_this, because you don't have Assert. */
    
#define JPEG_HANDLE_ANY_FACTORS 1
    /**< If jpeg_this is set, any component factors are valid.  Otherwise
     * it will only handle 2:1:1 (the typical form that sacrifices colour
     * resolution).  Note that Photoshop will only generate such files if you
     * use Sa_ve for Web.  Resetting jpeg_this sa_ves 468 bytes of IWRAM.
     */
     
#define JPEG_FASTER_M211 1
    /**< If jpeg_this is set, then the most common JPEG format is not given
     * special, faster treatment.  You must set JPEG_HANDLE_ANY_FACTORS
     * in jpeg_this case, or you will not see anything.  Resetting jpeg_this saves
     * 592 bytes of IWRAM, at the cost of speed.
     */
     
//#define JPEG_USE_IWRAM 1
    /**< If jpeg_this is set, the JPEG decompressor will use IWRAM for huge
     * benefits to decompression speed (249% faster than reset).  Resetting
     * jpeg_this saves up to 2648 bytes of IWRAM, depending upon
     * JPEG_HANDLE_ANY_FACTORS and JPEG_FASTER_M211.
     */
     
#define JPEG_DCTSIZE 8
    /**< The number of samples across and down a JPEG DCT.  This cannot be
     * configured, as the inverse DCT only handles 8x8.
     */

#define JPEG_DCTSIZE2 (JPEG_DCTSIZE * JPEG_DCTSIZE)
    /**< The number of samples in a full 2-D DCT. */
    
#ifndef JPEG_MAXIMUM_COMPONENTS
#define JPEG_MAXIMUM_COMPONENTS 3
    /**< The maximum number of components that can be involved in an image.
      * Each value costs 8 bytes of stack space and 8 bytes of allocations.
      */
#endif /* JPEG_MAXIMUM_SCAN_COMPONENTS */

#ifndef JPEG_OutputType    
#define JPEG_OutputType unsigned short
    /**< This is the data type that JPEG outputs to. */
#endif /* JPEG_OutputType */
    
#ifndef JPEG_Convert
/** Convert YCbCr values (each in the nominal range -64 to 63) to RGB and store
  * in the output value (of type JPEG_OutputType).  By default jpeg_this stores to 15-bit RGB.
  */
  
#define JPEG_Convert(OUT, Y, Cb, Cr) \
    do { \
        int eY = (Y) + 63; \
        int R = (eY) + ((Cr) * 359 >> 8); \
        int G = (eY) - ((Cb) * 88 >> 8) - ((Cr) * 183 >> 8); \
        int B = (eY) + ((Cb) * 454 >> 8); \
        \
        R = ComponentRange [(R >> 2) + 31]; \
        G = ComponentRange [(G >> 2) + 31] << 5; \
        B = ComponentRange [(B >> 2) + 31] << 10; \
        (OUT) = R | G | B; \
    } while (0)
#endif /* JPEG_Convert  */

typedef struct JPEG_HuffmanTable JPEG_HuffmanTable;
//typedef enum JPEG_Marker JPEG_Marker;
typedef struct JPEG_Decoder JPEG_Decoder;
typedef struct JPEG_FrameHeader JPEG_FrameHeader;
typedef struct JPEG_FrameHeader_Component JPEG_FrameHeader_Component;
typedef struct JPEG_ScanHeader JPEG_ScanHeader;
typedef struct JPEG_ScanHeader_Component JPEG_ScanHeader_Component;

typedef int JPEG_QuantizationTable [JPEG_DCTSIZE2]; /**< Quantization table elements, in zigzag order, fixed. */

/** Compute the multiplication of two fixed-point values. */
#define JPEG_FIXMUL(A, B) ((A) * (B) >> 8)

/** Convert a fixed-point value to an integer. */
#define JPEG_FIXTOI(A) ((A) >> 8)

/** Convert an integer to a fixed-point value. */
#define JPEG_ITOFIX(A) ((A) << 8)

/** A huffman table. */
struct JPEG_HuffmanTable
{
    const unsigned char *huffval; /**< Pointer to values in the table (256 entries). */
    int maxcode [16]; /**< The maximum code for each length - 1. */
    const unsigned char *valptr [16]; /* Items are subtracted by mincode and then indexed into huffval. */
    
    unsigned char look_nbits [256]; /**< The lookahead buffer lengths. */
    unsigned char look_sym [256]; /**< The lookahead buffer values. */
};

/** The markers that can appear in a JPEG stream. */
enum JPEG_Marker
{
    JPEG_Marker_APP0 = 0xFFE0, /**< Reserved application segment 0. */
    JPEG_Marker_APP1 = 0xFFE1, /**< Reserved application segment 1. */
    JPEG_Marker_APP2 = 0xFFE2, /**< Reserved application segment 2. */
    JPEG_Marker_APP3 = 0xFFE3, /**< Reserved application segment 3. */
    JPEG_Marker_APP4 = 0xFFE4, /**< Reserved application segment 4. */
    JPEG_Marker_APP5 = 0xFFE5, /**< Reserved application segment 5. */
    JPEG_Marker_APP6 = 0xFFE6, /**< Reserved application segment 6. */
    JPEG_Marker_APP7 = 0xFFE7, /**< Reserved application segment 7. */
    JPEG_Marker_APP8 = 0xFFE8, /**< Reserved application segment 8. */
    JPEG_Marker_APP9 = 0xFFE9, /**< Reserved application segment 9. */
    JPEG_Marker_APP10 = 0xFFEA, /**< Reserved application segment 10. */
    JPEG_Marker_APP11 = 0xFFEB, /**< Reserved application segment 11. */
    JPEG_Marker_APP12 = 0xFFEC, /**< Reserved application segment 12. */
    JPEG_Marker_APP13 = 0xFFED, /**< Reserved application segment 13. */
    JPEG_Marker_APP14 = 0xFFEE, /**< Reserved application segment 14. */
    JPEG_Marker_APP15 = 0xFFEF, /**< Reserved application segment 15. */
    JPEG_Marker_COM = 0xFFFE, /**< Comment. */
    JPEG_Marker_DHT = 0xFFC4, /**< Define huffman table. */
    JPEG_Marker_DQT = 0xFFDB, /**< Define quantization table(s). */
    JPEG_Marker_DRI = 0xFFDD, /**< Define restart interval. */
    JPEG_Marker_SOF0 = 0xFFC0, /**< Start of Frame, non-differential, Huffman coding, baseline DCT. */
    JPEG_Marker_SOI = 0xFFD8, /**< Start of image. */
    JPEG_Marker_SOS = 0xFFDA /**< Start of scan. */
};

/** An image component in the frame. */
struct JPEG_FrameHeader_Component
{
    unsigned char selector; /**< Component identifier, must be unique amongst the identifiers (C). */
    unsigned char horzFactor; /**< Horizontal sampling factor. */
    unsigned char vertFactor; /**< Vertical sampling factor. */
    unsigned char quantTable; /**< Quantization table destination selector. */
};

/** The frame header state. */
struct JPEG_FrameHeader
{
    JPEG_Marker marker; /**< The marker that began jpeg_this frame header, one of JPEG_Marker_SOFn. */
    int encoding; /**< 0 for Huffman coding, 1 for arithmetic coding. */
    char differential; /**< Differential (1) or non-differential (0). */
    
    unsigned char precision; /**< Sample precision - precision in bits for the samples of the components in the frame. */
    unsigned short height; /**< Maximum number of lines in the source image, equal to the number of lines in the component with the maximum number of vertical samples.  0 indicates that the number of lines shall be defined by the DNL marker and parameters at the end of the first scan. */
    unsigned short width; /**< Number of samples per line in the source image, equal to the number of samples per line in the component with the maximum number of horizontal samples. */
    JPEG_FrameHeader_Component componentList [JPEG_MAXIMUM_COMPONENTS]; /**< Components. */
    int componentCount; /**< Number of components. */
};

/** A component involved in jpeg_this scan. */
struct JPEG_ScanHeader_Component
{
    unsigned char selector; /**< Selector index corresponding to one specified in the frame header (Csj). */
    unsigned char dcTable; /**< DC entropy coding table destination selector (Tdj). */
    unsigned char acTable; /**< AC entropy coding table destination selector (Taj). */
};

/** Scan header state. */
struct JPEG_ScanHeader
{
    JPEG_ScanHeader_Component componentList [JPEG_MAXIMUM_COMPONENTS]; /**< Components involved in jpeg_this scan. */
    int componentCount; /**< Number of components involved in jpeg_this scan. */
    unsigned char spectralStart; /**< In DCT modes of operation, the first DCT coefficient in each block in zig-zag order which shall be coded in the scan (Ss).  For sequential DCT jpeg_this is zero. */
    unsigned char spectralEnd; /**< Specify the last DCT coefficient in each block in zig-zag order which shall be coded in the scan. */
    unsigned char successiveApproximationBitPositionHigh; /**< (Ah). */
    unsigned char successiveApproximationBitPositionLow; /**< (Al). */
};

/** The complete decoder state. */
struct JPEG_Decoder
{
    const unsigned char *acTables [4]; /**< The AC huffman table slots. */
    const unsigned char *dcTables [4]; /**< The DC huffman table slots. */
    JPEG_QuantizationTable quantTables [4]; /**< The quantization table slots. */
    unsigned int restartInterval; /**< Number of MCU in the restart interval (Ri). */
    JPEG_FrameHeader frame; /**< Current frame. */
    JPEG_ScanHeader scan; /**< Current scan. */
};

/** Start reading bits. */
#define JPEG_BITS_START() \
    unsigned int bits_left = 0, bits_data = 0;
    
/** Fill the buffer if there are fewer than jpeg_this many bits left. */
#define JPEG_BITS_CHECK(COUNT) \
    do { \
        while (bits_left < 32 - 7) \
        { \
            unsigned char c = *data ++; \
            \
            if (c == 0xFF) \
                data ++; \
            bits_data = (bits_data << 8) | c; \
            bits_left += 8; \
        } \
    } while (0)
   
/** Return and consume a number of bits. */
#define JPEG_BITS_GET(COUNT) \
    ((bits_data >> (bits_left -= (COUNT))) & ((1 << (COUNT)) - 1))
    
/** Return a number of bits without consuming them. */
#define JPEG_BITS_PEEK(COUNT) \
    ((bits_data >> (bits_left - (COUNT))) & ((1 << (COUNT)) - 1))
    
/** Drop a number of bits from the stream. */
#define JPEG_BITS_DROP(COUNT) \
    (bits_left -= (COUNT))
    
/* Check for a single bit and then read it. */
#define JPEG_BITS_NEXTBIT() \
    JPEG_BITS_GET (1) 

/** Read a single unsigned char from the current bit-stream by using the provided table. */
#define JPEG_HuffmanTable_Decode(TABLE) \
    { \
        int bitcount, look, result; \
        \
        JPEG_BITS_CHECK (8); \
        look = JPEG_BITS_PEEK (8); \
        \
        if ((bitcount = (TABLE)->look_nbits [look]) != 0) \
        { \
            JPEG_BITS_DROP (bitcount); \
            result = (TABLE)->look_sym [look]; \
        } \
        else \
        { \
            int i = 7, code = look; \
            \
            JPEG_BITS_DROP (8); \
            while (code > TABLE->maxcode [i]) \
            { \
                i ++; \
                JPEG_BITS_CHECK (1);\
                code = (code << 1) | JPEG_BITS_NEXTBIT (); \
            } \
            \
            result = TABLE->valptr [i] [code]; \
        } \
        \
        s = result; \
    }

extern const unsigned int JPEG_ToZigZag [JPEG_DCTSIZE2]; /* Converts row-major indices to zig-zagged order. */
extern const unsigned char JPEG_FromZigZag [JPEG_DCTSIZE2]; /* Converts zig-zagged indices to row-major order. */
extern const unsigned short JPEG_AANScaleFactor [JPEG_DCTSIZE2]; /* AA&N scaling factors for quantisation in 24:8 fixed point. */
extern const unsigned char JPEG_ComponentRange [32 * 3]; /* A limited component clamp that keeps values in the 0..31 range if incremented by 32. */

const unsigned char *JPEG_FrameHeader_Read (JPEG_FrameHeader *jpeg_this, const unsigned char *data, JPEG_Marker marker);
const unsigned char *JPEG_HuffmanTable_Read (JPEG_HuffmanTable *table, const unsigned char *data);
const unsigned char *JPEG_HuffmanTable_Skip (const unsigned char *data);
const unsigned char *JPEG_ScanHeader_Read (JPEG_ScanHeader *jpeg_this, const unsigned char *data);

/** Read all headers up to the start of the image and return the new data pointer. */
const unsigned char *JPEG_Decoder_ReadHeaders (JPEG_Decoder *jpeg_this, const unsigned char *data);

/** Read the entire image and return the data pointer. */
const unsigned char *JPEG_Decoder_ReadImage (JPEG_Decoder *jpeg_this, const unsigned char *data, volatile JPEG_OutputType *out, int outWidth, int outHeight);

/** Perform a 2D inverse DCT computation on the input.
  *
  * @param zz The coefficients to process, JPEG_DCTSIZE2 in length.  The
  *     contents will be destroyed in the computations.
  * @param chunk The chunk to store the results in, nominally from -64 to 63,
  *     although some error is expected.
  * @param chunkStride The number of values in a row for the chunk array.
  */

void JPEG_IDCT (int *zz, signed char *chunk, int chunkStride);

/** Create a decompressor, read the headers from the provided data, and then
  * read the image into the buffer given.
  */
  
void JPEG_DecompressImage (const unsigned char *data, volatile JPEG_OutputType *out, int outWidth, int outHeight);

#endif /* GBA_IMAGE_JPEG_H */
