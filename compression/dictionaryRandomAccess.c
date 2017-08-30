//this code is based on example from lz4
//https://github.com/lz4/lz4/blob/dev/examples/dictionaryRandomAccess.c

//read about random access
//https://github.com/lz4/lz4/blob/master/examples/dictionaryRandomAccess.md
//https://github.com/lz4/lz4/issues/187
//https://github.com/lz4/lz4/pull/263

//read about dictionary here
//https://github.com/facebook/zstd


#include "lz4.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef int64_t SceInt64;
typedef SceInt64 SceOff;

#define MIN(x, y) (x) < (y) ? (x) : (y)

enum 
{
    BLOCK_BYTES = 1024*10,  /* 1 KiB of uncompressed data in a block */
    MAX_BLOCKS = 1024 /* For simplicity of implementation */
};

int get_sizeof_header()
{
   return 0;
}

int test_compress(FILE* outFp, FILE* inpFp)
{
   LZ4_stream_t lz4Stream_body;
   LZ4_stream_t* lz4Stream = &lz4Stream_body;

   char compressedData[LZ4_COMPRESSBOUND(BLOCK_BYTES)];
   memset(compressedData, 0, LZ4_COMPRESSBOUND(BLOCK_BYTES));

   char rawData[BLOCK_BYTES];
   memset(rawData, 0, BLOCK_BYTES);

   SceOff offsets[MAX_BLOCKS];
   memset(offsets, 0, MAX_BLOCKS * sizeof(SceOff));

   SceOff* offsetsEnd = offsets;

   LZ4_resetStream(lz4Stream);

   *offsetsEnd++ = get_sizeof_header();

   // Write compressed data blocks.  
   while (1)
   {
      // read raw data
      int rawDataSize = fread(rawData, sizeof(char), BLOCK_BYTES, inpFp);
      if (rawDataSize == 0)
         break;

      //load zero size dictionary (this kinda resets the stream?)
      LZ4_loadDict(lz4Stream, NULL, 0);

      // compress raw data
      int compressedDataSize = LZ4_compress_fast_continue(lz4Stream, rawData, compressedData, rawDataSize, sizeof(compressedData), 1);
      if (compressedDataSize <= 0)
         return -1;

      // write compressed data
      fwrite(compressedData, sizeof(char), compressedDataSize, outFp);

      // update offset table
      *offsetsEnd = *(offsetsEnd - 1) + compressedDataSize;
      offsetsEnd++;

      if (offsetsEnd - offsets > MAX_BLOCKS) 
         return -1;
   }

   // Write the tailing jump table
   *offsetsEnd++ = (offsetsEnd - offsets);

   fwrite(offsets, sizeof(SceOff), (offsetsEnd - offsets), outFp);
   
   return 0;
}

int test_decompress(FILE* outFp, FILE* inpFp, SceOff dataOffset, int dataLength)
{
   LZ4_streamDecode_t lz4StreamDecode_body;
   LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

   // The blocks [currentBlock, endBlock) contain the data we want
   SceOff startBlock = dataOffset / BLOCK_BYTES;
   SceOff endBlock = ((dataOffset + dataLength - 1) / BLOCK_BYTES) + 1;

   if (dataLength == 0)
      return -1;

   // read number of offsets from tail
   SceOff numOffsets = 0;
   fseek(inpFp, -(sizeof(SceOff)), SEEK_END); //seek to number of offsets
   fread(&numOffsets, sizeof(SceOff), 1, inpFp);

   if (numOffsets <= endBlock) 
      return -1;

   //read offset table from tail
   SceOff offsets[MAX_BLOCKS];
   memset(offsets, 0, MAX_BLOCKS * sizeof(SceOff));
   fseek(inpFp, -(sizeof(SceOff)* (numOffsets + 1)), SEEK_END);
   fread(offsets, sizeof(SceOff), numOffsets, inpFp);

   // Seek to the first block to read
   fseek(inpFp, offsets[startBlock], SEEK_SET);
   SceOff offset = dataOffset % BLOCK_BYTES;

   char compressedData[LZ4_COMPRESSBOUND(BLOCK_BYTES)];
   memset(compressedData, 0, LZ4_COMPRESSBOUND(BLOCK_BYTES));

   char decompressedData[BLOCK_BYTES];
   memset(decompressedData, 0, BLOCK_BYTES);

   // Start decoding
   int length = dataLength;
   for (SceOff i = startBlock; i < endBlock; ++i)
   {
      // The difference in offsets is the size of the block
      int compressedDataSize = offsets[i + 1] - offsets[i];

      // read compressed data
      int readDataSize = fread(compressedData, sizeof(char), compressedDataSize, inpFp);
      if (readDataSize != compressedDataSize)
         return -1;

      //set zero size dictionary (this kinda resets the stream?)
      LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);

      //decompress data
      int decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, compressedData, decompressedData, compressedDataSize, BLOCK_BYTES);
      if (decBytes <= 0)
         return -1;

      //write chunk of the data

      //this statement is important when length is equal to multiple blocks
      //takes full block starting from offset or chunk of data if we are within 1 block
      int blockLength = MIN(length, (decBytes - offset));
      fwrite(decompressedData + offset, sizeof(char), blockLength, outFp);

      //offset is important only for first block
      offset = 0;
      length -= blockLength;
   }

   return 0;
}

int compare(FILE* fp0, FILE* fp1, int length)
{
   int result = 0;

   while(0 == result) 
   {
      char b0[4096];
      char b1[4096];

      const size_t r0 = fread(b0, sizeof(char), MIN(length, (int)sizeof(b0)), fp0);
      const size_t r1 = fread(b1, sizeof(char), MIN(length, (int)sizeof(b1)), fp1);

      result = (int) r0 - (int) r1;

      if(0 == r0 || 0 == r1) 
      {
         break;
      }
      if(0 == result) 
      {
         result = memcmp(b0, b1, r0);
      }
      length -= r0;
   }

   return result;
}

//-----------------

int compress(const char* inpFilename, const char* lz4Filename)
{
   FILE* inpFp = fopen(inpFilename, "rb");
   FILE* outFp = fopen(lz4Filename, "wb");

   printf("compress : %s -> %s\n", inpFilename, lz4Filename);
   test_compress(outFp, inpFp);
   printf("compress : done\n");

   fclose(outFp);
   fclose(inpFp);

   return 0;
}

int decompress(const char* lz4Filename, const char* decFilename, SceOff offset, int length)
{
   FILE* inpFp = fopen(lz4Filename, "rb");
   FILE* outFp = fopen(decFilename, "wb");

   printf("decompress : %s -> %s\n", lz4Filename, decFilename);
   test_decompress(outFp, inpFp, offset, length);
   printf("decompress : done\n");

   fclose(outFp);
   fclose(inpFp);

   return 0;
}

int verify(const char* inpFilename, const char* decFilename, int offset, int length)
{
   FILE* inpFp = fopen(inpFilename, "rb");
   FILE* decFp = fopen(decFilename, "rb");
   fseek(inpFp, offset, SEEK_SET);

   printf("verify : %s <-> %s\n", inpFilename, decFilename);
   const int cmp = compare(inpFp, decFp, length);
   if (0 == cmp) 
   {
      printf("verify : OK\n");
   }
   else 
   {
      printf("verify : NG\n");
   }

   fclose(decFp);
   fclose(inpFp);

   return 0;
}

//TODO:
//ALL OFFSETS MUST BE 64 bit
//memory allocation should be dynamic - no MAX_BLOCKS
//header should store information about type of compression. lz4 or smth else

int main(int argc, char* argv[])
{
   const char* inpFilename = "test.bin";
   const char* lz4Filename = "test.bin.lz4";
   const char* decFilename = "test.bin.lz4.dec";

   //int offset = 10;
   //int length = 20900;

   int offset = 0;
   int length = 0x20910;

   compress(inpFilename, lz4Filename);
   decompress(lz4Filename, decFilename, offset, length);
   verify(inpFilename, decFilename, offset, length);

   return 0;
}
