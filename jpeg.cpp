#include "jpeg.h"



#define JPEG_FUNCTION_END(NAME) static void NAME##End () { }
#define JPEG_FUNCTION_SIZE(NAME) ((int) &NAME##End - (int) &NAME) & ~3

typedef void (*JPEG_FUNSELF)(int *, int *, int *,
    JPEG_HuffmanTable *, JPEG_HuffmanTable *,
    const unsigned char **, unsigned int *, unsigned int *, const unsigned int *);
typedef void (*JPEG_IDCT_Columns_Fun) (int *) ;
typedef void (*JPEG_IDCT_Rows_Fun) (int *, signed char *, int );
typedef void (*JPEG_ConvertBlock_Fun) (
    signed char *, signed char *, signed char *,
    int , int , int , int , int , int , int , int ,
    char , volatile JPEG_OutputType *, int , const unsigned char *);

#if JPEG_MARK_TIME


#define JPEG_GetTime() ((REG_TM3D << 16) + REG_TM2D)
#define JPEG_MarkStart(NAME) unsigned int NAME##start = JPEG_GetTime ()
#define JPEG_MarkEnd(NAME) NAME += JPEG_GetTime () - NAME##start
#define JPEG_StartTime() \
    REG_IME = 0; \
    REG_TM2D = 0; \
    REG_TM2CNT = 0 | 128; /* Every 256 cycles and start. */ \
    \
    REG_TM3D = 0; \
    REG_TM3CNT = 4 | 128; /* Increment on overflow and start. */

static unsigned int timeIDCT = 0, timeDecodeCoefficients = 0, timeConversion = 0;
static unsigned int idctCount = 0;

void *stackDeepest;

#else

#define JPEG_GetTime() 0
#define JPEG_MarkStart(NAME)
#define JPEG_MarkEnd(NAME)
#define JPEG_StartTime()
#endif /* JPEG_MARK_TIME */

#if JPEG_USE_IWRAM

#ifndef REG_DM3SAD
#define REG_DM3SAD (*(volatile unsigned int *) 0x40000D4)
#endif /* REG_DM3SAD */

#ifndef REG_DM3DAD
#define REG_DM3DAD (*(volatile unsigned int *) 0x40000D8)
#endif /* REG_DM3DAD */

#ifndef REG_DM3CNT_L
#define REG_DM3CNT_L   	(*(volatile unsigned short *) 0x40000DC)
#endif /* REG_DM3CNT_L */

#ifndef REG_DM3CNT_H
#define REG_DM3CNT_H   	(*(volatile unsigned short *) 0x40000DE)
#endif /* REG_DM3CNT_H */

extern char __bss_end;

#define JPEG_IWRAM_USED_END (&__bss_end)

#endif /* JPEG_USE_IWRAM */

const unsigned int JPEG_ToZigZag [JPEG_DCTSIZE2] =
{
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
};

const unsigned short JPEG_AANScaleFactor [JPEG_DCTSIZE2] =
{
    256, 355, 334, 301, 256, 201, 138, 70,
    355, 492, 463, 417, 355, 278, 192, 97,
    334, 463, 437, 393, 334, 262, 181, 92,
    301, 417, 393, 353, 301, 236, 162, 83,
    256, 355, 334, 301, 256, 201, 138, 70,
    201, 278, 262, 236, 201, 158, 108, 55,
    138, 192, 181, 162, 138, 108, 74, 38,
    70, 97, 92, 83, 70, 55, 38, 19,
};

const unsigned char JPEG_ComponentRange [32 * 3] =
{
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
		19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

		31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
		31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31
};

static void JPEG_IDCT_Columns (int *zz)
{
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9, tmp10, tmp11;
    int *ez = zz + 8;
    
    for ( ; zz < ez; zz ++)
    {
        tmp0 = zz [0 * JPEG_DCTSIZE];
        tmp1 = zz [2 * JPEG_DCTSIZE];
        tmp2 = zz [4 * JPEG_DCTSIZE];
        tmp3 = zz [6 * JPEG_DCTSIZE];
        
        tmp6 = tmp1 + tmp3;
        tmp7 = JPEG_FIXMUL (tmp1 - tmp3, 362) - tmp6;
        tmp1 = tmp0 - tmp2 + tmp7;
        tmp0 = tmp0 + tmp2 + tmp6;
        
        tmp3 = tmp0 - (tmp6 << 1);
        tmp2 = tmp1 - (tmp7 << 1);
        
        tmp4 = zz [1 * JPEG_DCTSIZE];
        tmp5 = zz [3 * JPEG_DCTSIZE];
        tmp6 = zz [5 * JPEG_DCTSIZE];
        tmp7 = zz [7 * JPEG_DCTSIZE];
        
        tmp10 = tmp4 - tmp7;
        
        tmp8 = tmp6 + tmp5;
        tmp9 = tmp4 + tmp7;
        tmp7 = tmp9 + tmp8;
        tmp11 = JPEG_FIXMUL (tmp9 - tmp8, 362);
        
        tmp8 = tmp6 - tmp5;
        tmp9 = JPEG_FIXMUL (tmp8 + tmp10, 473);
        
        tmp6 = JPEG_FIXMUL (-669, tmp8) + tmp9 - tmp7;
        tmp5 = tmp11 - tmp6;
        tmp4 = JPEG_FIXMUL (277, tmp10) - tmp9 + tmp5; 
        
        zz [0 * JPEG_DCTSIZE] = tmp0 + tmp7;
        zz [1 * JPEG_DCTSIZE] = tmp1 + tmp6;
        zz [2 * JPEG_DCTSIZE] = tmp2 + tmp5;
        zz [3 * JPEG_DCTSIZE] = tmp3 - tmp4;
        zz [4 * JPEG_DCTSIZE] = tmp3 + tmp4;
        zz [5 * JPEG_DCTSIZE] = tmp2 - tmp5;
        zz [6 * JPEG_DCTSIZE] = tmp1 - tmp6;
        zz [7 * JPEG_DCTSIZE] = tmp0 - tmp7;
    }
}
JPEG_FUNCTION_END (JPEG_IDCT_Columns)

static void JPEG_IDCT_Rows (int *line, signed char *chunk, int chunkStride)
{
    int tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13;
    int tmp4, tmp5, tmp6, tmp7, z5, z10, z11, z12, z13;
    int row;
    
    for (row = 0; row < JPEG_DCTSIZE; row ++, line += JPEG_DCTSIZE, chunk += chunkStride)
    {
        tmp10 = line [0] + line [4];
        tmp11 = line [0] - line [4];

        tmp13 = line [2] + line [6];
        tmp12 = JPEG_FIXMUL (line [2] - line [6], 362) - tmp13;

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;
        
        z13 = line [5] + line [3];
        z10 = line [5] - line [3];
        z11 = line [1] + line [7];
        z12 = line [1] - line [7];

        tmp7 = z11 + z13;
        tmp11 = JPEG_FIXMUL (z11 - z13, 362);

        z5 = JPEG_FIXMUL (z10 + z12, 473);
        tmp10 = JPEG_FIXMUL (277, z12) - z5;
        tmp12 = JPEG_FIXMUL (-669, z10) + z5;
        
        tmp6 = tmp12 - tmp7;
        tmp5 = tmp11 - tmp6;
        tmp4 = tmp10 + tmp5;

        /* This shifts by an extra bit to remove the need for clamping at
         * jpeg_this point.  Thus the normative samples are in the range -64 to 63.
         * This requires a later bit-shift, but that comes for free with the ARM
         * instruction set, and has an acceptable, likely imperceptible, loss
         * of quality.
         */
         
        chunk [0] = JPEG_FIXTOI (tmp0 + tmp7) >> 4;
        chunk [1] = JPEG_FIXTOI (tmp1 + tmp6) >> 4;
        chunk [2] = JPEG_FIXTOI (tmp2 + tmp5) >> 4;
        chunk [3] = JPEG_FIXTOI (tmp3 - tmp4) >> 4;
        chunk [4] = JPEG_FIXTOI (tmp3 + tmp4) >> 4;
        chunk [5] = JPEG_FIXTOI (tmp2 - tmp5) >> 4;
        chunk [6] = JPEG_FIXTOI (tmp1 - tmp6) >> 4;
        chunk [7] = JPEG_FIXTOI (tmp0 - tmp7) >> 4;
    }
}
JPEG_FUNCTION_END (JPEG_IDCT_Rows)

/* This function comes from jpeglib.  I feel all right about that since it comes from AA&N anyway. */
void JPEG_IDCT (int *zz, signed char *chunk, int chunkStride)
{
    JPEG_IDCT_Columns (zz);
    JPEG_IDCT_Rows (zz, chunk, chunkStride);
}

#define JPEG_Value(COUNT) \
    { \
        int value = 0; \
        \
        JPEG_BITS_CHECK (COUNT); \
        value = (int) JPEG_BITS_GET (COUNT); \
        \
        if (value < 1 << (COUNT - 1)) \
            value += (-1 << COUNT) + 1; \
        \
        diff = value; \
    }
    
static void JPEG_DecodeCoefficients (
    int *dcLast, int *zz, int *quant,
    JPEG_HuffmanTable *dcTable, JPEG_HuffmanTable *acTable,
    const unsigned char **dataBase, unsigned int *bitsLeftBase, unsigned int *bitsDataBase, const unsigned int *toZigZag)
{
    unsigned bits_left = *bitsLeftBase, bits_data = *bitsDataBase;
    const unsigned char *data = *dataBase;
    int index = 1, r, s, diff;

    JPEG_HuffmanTable_Decode (dcTable);
    JPEG_Value (s);

    {
        int *ez = zz + JPEG_DCTSIZE2;
        do *-- ez = 0;
        while (ez > zz);
    }
    
    *dcLast += diff;
    zz [0] = *dcLast * quant [0];

    while (1)
    {
        JPEG_HuffmanTable_Decode (acTable);
        r = s >> 4;
        s &= 15;
    
        if (s)
        {
            index += r;
            JPEG_Value (s) ;
            zz [toZigZag [index]] = diff * quant [index];
            if (index ++ == JPEG_DCTSIZE2 - 1)
                break;
        }
        else
        {
            if (r != 15)
                break;
            index += 16;
        }
    }
    
    *bitsDataBase = bits_data;
    *bitsLeftBase = bits_left;
    *dataBase = data;
}
JPEG_FUNCTION_END (JPEG_DecodeCoefficients)

static void JPEG_ConvertBlock (
    signed char *YBlock, signed char *CbBlock, signed char *CrBlock,
    int YHorzFactor, int YVertFactor, int CbHorzFactor, int CbVertFactor, int CrHorzFactor, int CrVertFactor, int horzMax, int vertMax,
    char M211, volatile JPEG_OutputType *out, int outStride, const unsigned char *ComponentRange)
{
    int px, py;
    
#if JPEG_FASTER_M211                
    if (M211)
    {
        for (py = 0; py < 2 * JPEG_DCTSIZE; py += 2)
        {
            volatile JPEG_OutputType *row = &out [outStride * py];
            volatile JPEG_OutputType *rowEnd = row + JPEG_DCTSIZE * 2;
            
            for ( ; row < rowEnd; row += 2, YBlock += 2, CbBlock ++, CrBlock ++)
            {
                int Cb = *CbBlock, Cr = *CrBlock;
                JPEG_Convert (row [0], YBlock [0], Cb, Cr);
                JPEG_Convert (row [1], YBlock [1], Cb, Cr);
                JPEG_Convert (row [240], YBlock [2 * JPEG_DCTSIZE + 0], Cb, Cr);
                JPEG_Convert (row [241], YBlock [2 * JPEG_DCTSIZE + 1], Cb, Cr);
            }
            
            YBlock += JPEG_DCTSIZE * 2;
        }
    }
#else
    if (0) { }
#endif /* JPEG_FASTER_M211 */
#if JPEG_HANDLE_ANY_FACTORS
    else for (py = 0; py < vertMax; py ++)
    {
        signed char *YScan = YBlock + (py * YVertFactor >> 8) * (horzMax * YHorzFactor >> 8);
        signed char *CbScan = CbBlock + (py * CbVertFactor >> 8) * (horzMax * CbHorzFactor >> 8);
        signed char *CrScan = CrBlock + (py * CrVertFactor >> 8) * (horzMax * CrHorzFactor >> 8);
        
        volatile JPEG_OutputType *row = &out [outStride * py];
        
        for (px = 0; px < horzMax; px ++, row ++)
        {
            int Y = YScan [px * YHorzFactor >> 8];
            int Cb = CbScan [px * CbHorzFactor >> 8];
            int Cr = CrScan [px * CrHorzFactor >> 8];
            
            JPEG_Convert (*row, Y, Cb, Cr);
        }
    }
#else
    (void) YHorzFactor, (void) YVertFactor, (void) CbHorzFactor, (void) CbVertFactor, (void) CrHorzFactor, (void) CrVertFactor;
    (void) horzMax, (void) vertMax, (void) px;
#endif /* JPEG_HANDLE_ANY_FACTORS */

    #undef Convert
}
JPEG_FUNCTION_END (JPEG_ConvertBlock)

const unsigned char *JPEG_Decoder_ReadImage (JPEG_Decoder *jpeg_this, const unsigned char *data, volatile JPEG_OutputType *out, int outWidth, int outHeight)
{
    JPEG_FrameHeader *frame = &jpeg_this->frame;
    JPEG_ScanHeader *scan = &jpeg_this->scan;
    int YHorzFactor = 0, YVertFactor = 0;
    int CbHorzFactor = 1, CbVertFactor = 1;
    int CrHorzFactor = 1, CrVertFactor = 1;
    int horzMax = 0, vertMax = 0;
    JPEG_FrameHeader_Component *frameComponents [JPEG_MAXIMUM_COMPONENTS];
    JPEG_FrameHeader_Component *item, *itemEnd = frame->componentList + frame->componentCount;
    int dcLast [JPEG_MAXIMUM_COMPONENTS];
    int c, bx, by, cx, cy;
    int horzShift = 0;
    int vertShift = 0;
    char M211 = 0;
    
    JPEG_HuffmanTable acTableList [2];
    int acTableUse [2];
    JPEG_HuffmanTable dcTableList [2];
    int dcTableUse [2];
    
    acTableUse [0] = acTableUse [1] = dcTableUse [0] = dcTableUse [1] = -1;
    
    void *ConvertBlock = (void*)&JPEG_ConvertBlock;
    void *IDCT_Columns = (void*)&JPEG_IDCT_Columns;
    void *IDCT_Rows = (void*)&JPEG_IDCT_Rows;
    void *DecodeCoefficients = (void*)&JPEG_DecodeCoefficients;
    
    //unsigned int *ToZigZag;
    //unsigned char *ComponentRange;
    void *ToZigZag;
    void *ComponentRange;
   
#if JPEG_MARK_TIME
    stackDeepest = &ComponentRange;
#endif /* JPEG_MARK_TIME */
    
#ifdef JPEG_USE_IWRAM
    void *iwramEnd = JPEG_IWRAM_USED_END;
    
    #define LoadValue(NAME,SIZE) \
        NAME = iwramEnd; \
        while (REG_DM3CNT_H & (1 << 15)); \
        REG_DM3SAD = (unsigned int) &JPEG_##NAME; \
        REG_DM3DAD = (unsigned int) iwramEnd; \
        REG_DM3CNT_L =  SIZE >> 2; \
        REG_DM3CNT_H = (1 << 10) | (1 << 15); \
        iwramEnd = (void*)( (int)iwramEnd + SIZE & ~3)
        
    #define LoadFunction(NAME) LoadValue (NAME, JPEG_FUNCTION_SIZE (JPEG_##NAME))
    #define LoadData(NAME) LoadValue (NAME, sizeof (JPEG_##NAME))

    LoadFunction (ConvertBlock);
    LoadFunction (DecodeCoefficients);
    LoadFunction (IDCT_Columns);
    LoadFunction (IDCT_Rows);

    LoadData (ToZigZag);
    LoadData (ComponentRange);

#endif /* JPEG_USE_IWRAM */

    /* Find the maximum factors and the factors for each component. */    
    for (item = frame->componentList; item < itemEnd; item ++)
    {
        for (c = 0; c < scan->componentCount; c ++)
        {
            JPEG_ScanHeader_Component *sc = &scan->componentList [c];
            
            if (sc->selector != item->selector)
                continue;
            
            if (sc->dcTable != dcTableUse [0] && sc->dcTable != dcTableUse [1])
            {
                if (dcTableUse [0] == -1)
                    dcTableUse [0] = sc->dcTable, JPEG_HuffmanTable_Read (&dcTableList [0], jpeg_this->dcTables [sc->dcTable]);
                else
                    dcTableUse [1] = sc->dcTable, JPEG_HuffmanTable_Read (&dcTableList [1], jpeg_this->dcTables [sc->dcTable]);
            }
            
            if (sc->acTable != acTableUse [0] && sc->acTable != acTableUse [1])
            {
                if (acTableUse [0] == -1)
                    acTableUse [0] = sc->acTable, JPEG_HuffmanTable_Read (&acTableList [0], jpeg_this->acTables [sc->acTable]);
                else
                    acTableUse [1] = sc->acTable, JPEG_HuffmanTable_Read (&acTableList [1], jpeg_this->acTables [sc->acTable]);
            }
            
            frameComponents [c] = item;
            break;
        }
        
        if (item->horzFactor > horzMax)
            horzMax = item->horzFactor;
        if (item->vertFactor > vertMax)
            vertMax = item->vertFactor;
            
        if (item->selector == 1)
        {
            YHorzFactor = item->horzFactor;
            YVertFactor = item->vertFactor;
        }
        else if (item->selector == 2)
        {
            CbHorzFactor = item->horzFactor;
            CbVertFactor = item->vertFactor;
        }
        else if (item->selector == 3)
        {
            CrHorzFactor = item->horzFactor;
            CrVertFactor = item->vertFactor;
        }
    }
    
    signed char *YBlock = new signed char[YHorzFactor * YVertFactor * JPEG_DCTSIZE2];
    signed char *CbBlock = new signed char[CbHorzFactor * CrVertFactor * JPEG_DCTSIZE2];
    signed char *CrBlock =  new signed char[CrHorzFactor * CbVertFactor * JPEG_DCTSIZE2];
    
    if (horzMax == 1) horzShift = 8;
    else if (horzMax == 2) horzShift = 7;
    else if (horzMax == 4) horzShift = 6;
    
    if (vertMax == 1) vertShift = 8;
    else if (vertMax == 2) vertShift = 7;
    else if (vertMax == 4) vertShift = 6;
    
    YHorzFactor <<= horzShift;
    YVertFactor <<= vertShift;
    CbHorzFactor <<= horzShift;
    CbVertFactor <<= vertShift;
    CrHorzFactor <<= horzShift;
    CrVertFactor <<= vertShift;
    
    /* Clear the Cb channel for potential grayscale. */
    {
        signed char *e = CbBlock + JPEG_DCTSIZE2;
        
        do *-- e = 0;
        while (e > CbBlock);
    }
    
    /* Clear the Cr channel for potential grayscale. */
    {
        signed char *e = CrBlock + JPEG_DCTSIZE2;
        
        do *-- e = 0;
        while (e > CrBlock);
    }

#if JPEG_FASTER_M211
    if (YHorzFactor == 256 && YVertFactor == 256 && CbHorzFactor == 128 && CbVertFactor == 128 && CrHorzFactor == 128 && CrVertFactor == 128)
        M211 = 1;
#endif /* JPEG_FASTER_M211 */
        
    for (c = 0; c < JPEG_MAXIMUM_COMPONENTS; c ++)
        dcLast [c] = 0;
    
    JPEG_BITS_START ();
    for (by = 0; by < frame->height; by += vertMax * JPEG_DCTSIZE)
    {
        for (bx = 0; bx < frame->width; bx += horzMax * JPEG_DCTSIZE)
        {
            for (c = 0; c < scan->componentCount; c ++)
            {
                JPEG_ScanHeader_Component *sc = &scan->componentList [c];
                JPEG_FrameHeader_Component *fc = frameComponents [c];
                JPEG_HuffmanTable *dcTable, *acTable;
                int *quant = jpeg_this->quantTables [fc->quantTable];
                int stride = fc->horzFactor * JPEG_DCTSIZE;
                
                dcTable = &dcTableList [sc->dcTable == dcTableUse [1] ? 1 : 0];
                acTable = &acTableList [sc->acTable == acTableUse [1] ? 1 : 0];
                
                for (cy = 0; cy < fc->vertFactor * JPEG_DCTSIZE; cy += JPEG_DCTSIZE)
                {
                    for (cx = 0; cx < fc->horzFactor * JPEG_DCTSIZE; cx += JPEG_DCTSIZE)
                    {
                        signed char *chunk = 0;
                        int start = cx + cy * stride;
                        int zz [JPEG_DCTSIZE2];
                        
                        if (fc->selector == 1)
                            chunk = YBlock;
                        else if (fc->selector == 2)
                            chunk = CbBlock;
                        else if (fc->selector == 3)
                            chunk = CrBlock;
                        
                        JPEG_MarkStart (timeDecodeCoefficients);
						JPEG_FUNSELF pp=(JPEG_FUNSELF)DecodeCoefficients;
                        pp (&dcLast [c], zz, quant, dcTable, acTable, &data, &bits_left, &bits_data, (unsigned int *)ToZigZag);
                        JPEG_MarkEnd (timeDecodeCoefficients);

                        if (chunk)
                        {
                            JPEG_MarkStart (timeIDCT);
							JPEG_IDCT_Columns_Fun pp=(JPEG_IDCT_Columns_Fun)IDCT_Columns;
                            pp (zz);
							JPEG_IDCT_Rows_Fun pp2=(JPEG_IDCT_Rows_Fun)IDCT_Rows;
                            pp2 (zz, chunk + start, stride);
                        #if JPEG_MARK_TIME
                            idctCount ++;
                        #endif /* JPEG_MARK_TIME */
                            JPEG_MarkEnd (timeIDCT);
                        }
                    }
                }
            }
            
            if (bx + horzMax * JPEG_DCTSIZE > outWidth || by + vertMax * JPEG_DCTSIZE > outHeight)
                continue;
                
            JPEG_MarkStart (timeConversion);
			JPEG_ConvertBlock_Fun pp=(JPEG_ConvertBlock_Fun)ConvertBlock;
            pp (YBlock, CbBlock, CrBlock,
                YHorzFactor, YVertFactor, CbHorzFactor, CbVertFactor, CrHorzFactor, CrVertFactor,
                horzMax * JPEG_DCTSIZE, vertMax * JPEG_DCTSIZE, M211, out + bx + by * outWidth, outWidth, (unsigned char *)ComponentRange);
            JPEG_MarkEnd (timeConversion);
        }
        
        if (jpeg_this->restartInterval)
        {
            bits_left = 0;
            while (data [0] != 0xFF || !(data [1] >= 0xD0 && data [1] <= 0xD7))
                data --;
            data += 2;
            for (c = 0; c < JPEG_MAXIMUM_COMPONENTS; c ++)
                dcLast [c] = 0;
        }
    }
    if(YBlock) delete  YBlock;
    if(CbBlock) delete CbBlock;
    if(CrBlock) delete CrBlock;
    
    return data;
}

const unsigned char *JPEG_FrameHeader_Read (JPEG_FrameHeader *jpeg_this, const unsigned char *data, JPEG_Marker marker)
{
    int index;
        
    data += 2; /* Skip the length. */
    jpeg_this->marker = marker;
    jpeg_this->encoding = (marker >= 0xFFC0 && marker <= 0xFFC7) ? 0 : 1;
    jpeg_this->differential = !(marker >= 0xFFC0 && marker <= 0xFFC3 && marker >= 0xFFC8 && marker <= 0xFFCB);
    
    jpeg_this->precision = *data ++;
    jpeg_this->height = (data [0] << 8) | data [1]; data += 2;
    jpeg_this->width = (data [0] << 8) | data [1]; data += 2;
    jpeg_this->componentCount = *data ++;
    
    for (index = 0; index < jpeg_this->componentCount; index ++)
    {
        JPEG_FrameHeader_Component *c = &jpeg_this->componentList [index];
        unsigned char pair;
        
        c->selector = *data ++;
        pair = *data ++;
        c->horzFactor = pair >> 4;
        c->vertFactor = pair & 15;
        c->quantTable = *data ++;
    }
    
    return data;
}

const unsigned char *JPEG_ScanHeader_Read (JPEG_ScanHeader *jpeg_this, const unsigned char *data)
{
    JPEG_ScanHeader_Component *c, *cEnd;
    unsigned char pair;
    
    data += 2; /* Skip the length. */
    jpeg_this->componentCount = *data ++;
    
    for (c = jpeg_this->componentList, cEnd = c + jpeg_this->componentCount; c < cEnd; c ++)
    {
        c->selector = *data ++;
        pair = *data ++;
        c->dcTable = pair >> 4;
        c->acTable = pair & 15;
    }
    
    jpeg_this->spectralStart = *data ++;
    jpeg_this->spectralEnd = *data ++;
    pair = *data ++;
    jpeg_this->successiveApproximationBitPositionHigh = pair >> 4;
    jpeg_this->successiveApproximationBitPositionLow = pair & 15;
    return data;
}

const unsigned char *JPEG_Decoder_ReadHeaders (JPEG_Decoder *jpeg_this, const unsigned char *data)
{
    unsigned int marker;
    int c;
    
    jpeg_this->restartInterval = 0;
    data += 2; /* SOI marker, hopefully. */
        
    while (1)
    {
        marker = (data [0] << 8) | data [1]; data += 2;
        
        switch ((JPEG_Marker)marker)
        {
            case JPEG_Marker_APP0:
            case JPEG_Marker_APP1:
            case JPEG_Marker_APP2:
            case JPEG_Marker_APP3:
            case JPEG_Marker_APP4:
            case JPEG_Marker_APP5:
            case JPEG_Marker_APP6:
            case JPEG_Marker_APP7:
            case JPEG_Marker_APP8:
            case JPEG_Marker_APP9:
            case JPEG_Marker_APP10:
            case JPEG_Marker_APP11:
            case JPEG_Marker_APP12:
            case JPEG_Marker_APP13:
            case JPEG_Marker_APP14:
            case JPEG_Marker_APP15:
            case JPEG_Marker_COM:
                data += (data [0] << 8) | data [1];
                break;
            
            case JPEG_Marker_DHT:
            {
                unsigned short length = (data [0] << 8) | data [1]; data += 2;
                const unsigned char *end = data + length - 2;
                
                while (data < end)
                {
                    unsigned char pair;
                    
                    pair = *data ++;
                    unsigned char type = pair >> 4;
                    unsigned char slot = pair & 15;
                    
                    if (type == 0)
                        jpeg_this->dcTables [slot] = data;
                    else
                        jpeg_this->acTables [slot] = data;
                        
                    data = JPEG_HuffmanTable_Skip (data);
                }
                
                break;
            }
            
            case JPEG_Marker_DQT:
            {
                unsigned short length = (data [0] << 8) | data [1]; data += 2;
                const unsigned char *end = data + length - 2;
                int col, row;
                int *s;
                
                while (data < end)
                {
                    int slot;
                    
                    slot = (*data ++) & 15;
                    s = jpeg_this->quantTables [slot];
                    
                    for (c = 0; c < JPEG_DCTSIZE2; c ++)
                        s [c] = JPEG_ITOFIX (*data ++);
                    for (row = 0; row < JPEG_DCTSIZE; row ++)
                        for (col = 0; col < JPEG_DCTSIZE; col ++)
                        {
                            int *item = &s [col + row * JPEG_DCTSIZE];
                            
                            *item = JPEG_FIXMUL (*item, JPEG_AANScaleFactor [JPEG_ToZigZag [row * JPEG_DCTSIZE + col]]);
                        }
                }
                
                break;
            }
        
            case JPEG_Marker_DRI:
                jpeg_this->restartInterval = (data [0] << 8) | data [1];
                data += 2;
                break;
            
            case JPEG_Marker_SOF0:
                data = JPEG_FrameHeader_Read (&jpeg_this->frame, data, (JPEG_Marker)marker);
                break;
            
            case JPEG_Marker_SOS:
                data = JPEG_ScanHeader_Read (&jpeg_this->scan, data);
                return data;
                
            default:
                break;
        }
    }
}

const unsigned char *JPEG_HuffmanTable_Skip (const unsigned char *data)
{
    int c, total = 0;
    
    for (c = 0; c < 16; c ++)
        total += *data ++;
    data += total;
    return data;
}

const unsigned char *JPEG_HuffmanTable_Read (JPEG_HuffmanTable *jpeg_this, const unsigned char *data)
{
    const unsigned char *bits;
    int huffcode [256];
    unsigned char huffsize [256];
    int total = 0;
    int c;
    
    bits = data;
    for (c = 0; c < 16; c ++)
        total += *data ++;
    jpeg_this->huffval = data;
    data += total;
    
    
    
    
    
    {
        int k = 0;
		int i = 1;
		int j = 1;
        
        do
        {
            while (j ++ <= bits [i - 1])
                huffsize [k ++] = i;
            i ++;
            j = 1;
        }
        while (i <= 16);
            
        huffsize [k] = 0;
    }
    {
        int k = 0, code = 0, si = huffsize [0];

        while (1)
        {            
            do huffcode [k ++] = code ++;
            while (huffsize [k] == si);
                
            if (huffsize [k] == 0)
                break;
            
            do code <<= 1, si ++;
            while (huffsize [k] != si);
        }
    }
    {
        int i = 0, j = 0;
        
        while (1)
        {
            if (i >= 16)
                break;
            if (bits [i] == 0)
                jpeg_this->maxcode [i] = -1;
            else
            {
                jpeg_this->valptr [i] = &jpeg_this->huffval [j - huffcode [j]];
                j += bits [i];
                jpeg_this->maxcode [i] = huffcode [j - 1];
            }
            i ++;
        }
    }
    {
        int l, i, p, c, ctr;
        
        for (c = 0; c < 256; c ++)
            jpeg_this->look_nbits [c] = 0;
            
        p = 0;
        for (l = 1; l <= 8; l ++)
        {
            for (i = 1; i <= bits [l - 1]; i ++, p ++)
            {
                int lookbits = huffcode [p] << (8 - l);
                
                for (ctr = 1 << (8 - l); ctr > 0; ctr --)
                {
                    jpeg_this->look_nbits [lookbits] = l;
                    jpeg_this->look_sym [lookbits] = jpeg_this->huffval [p];
                    lookbits ++;
                }
            }
        }
    }
    return data;
}

void JPEG_DecompressImage (const unsigned char *data, volatile JPEG_OutputType *out, int outWidth, int outHeight)
{
    JPEG_Decoder decoder;
    
    data = JPEG_Decoder_ReadHeaders (&decoder, data);
    JPEG_StartTime ();
    data = JPEG_Decoder_ReadImage (&decoder, data, out, outWidth, outHeight);

#if JPEG_MARK_TIME
    int ticks = JPEG_GetTime ();
    
    Assert (0, "%d milliseconds, %d clock cycles,\n%d cycles per pixel\n"
    "%d%% in IDCT (%d cycles per 8x8 block)\n"
    "%d%% in coefficient decoding (%d cycles per 8x8 block)\n"
    "%d%% in colour conversion and storage (%d cycles per pixel)\nsaved %d, size %d, deep %d\n", ticks / 256 * 1000 / (CyclesPerSecond / 256), ticks, ticks / (decoder.frame.width * decoder.frame.height),
    (int) ((unsigned long long) timeIDCT * 100 / ticks), timeIDCT / idctCount,
    (int) ((unsigned long long) timeDecodeCoefficients * 100 / ticks), timeDecodeCoefficients / idctCount,
    (int) ((unsigned long long) timeConversion * 100 / ticks), timeConversion / (decoder.frame.width * decoder.frame.height),
    sizeof (JPEG_HuffmanTable) * 4, sizeof (JPEG_Decoder),
    (void *) &decoder - stackDeepest + sizeof (decoder));
#endif /* JPEG_MARK_TIME */
}
