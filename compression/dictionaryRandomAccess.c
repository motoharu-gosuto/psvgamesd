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

int64_t get_file_size(FILE* inpFp)
{
   int res = _fseeki64(inpFp, 0, SEEK_END);
   int64_t pos = _ftelli64(inpFp);
   res = _fseeki64(inpFp, 0, SEEK_SET);  
   return pos;
}

int64_t roundDown(int64_t n, int64_t m) 
{
   return n >= 0 ? (n / m) * m : ((n - m + 1) / m) * m;
}

/** round n up to nearest multiple of m */
int64_t roundUp(int64_t n, int64_t m) 
{
   return n >= 0 ? ((n + m - 1) / m) * m : (n / m) * m;
}

int get_sizeof_header()
{
   return 0;
}

int test_compress_internal(FILE* outFp, FILE* inpFp, int block_bytes, const char* dicData, int64_t dicSize, SceOff* offsetsTable, int offsetsCapacity, char* compressedData, char* rawData)
{
   LZ4_stream_t lz4Stream_body;
   LZ4_stream_t* lz4Stream = &lz4Stream_body;

   SceOff* offsetsEnd = offsetsTable;

   LZ4_resetStream(lz4Stream);

   *offsetsEnd++ = get_sizeof_header();

   // Write compressed data blocks.  
   while (1)
   {
      // read raw data
      int rawDataSize = fread(rawData, sizeof(char), block_bytes, inpFp);
      if (rawDataSize == 0)
         break;

      //load zero size dictionary (this kinda resets the stream?)
      if (dicData == 0)
         LZ4_loadDict(lz4Stream, NULL, 0);
      else
         LZ4_loadDict(lz4Stream, dicData, dicSize);

      // compress raw data
      int compressedDataSize = LZ4_compress_fast_continue(lz4Stream, rawData, compressedData, rawDataSize, LZ4_COMPRESSBOUND(block_bytes), 1);
      if (compressedDataSize <= 0)
         return -1;

      // write compressed data
      fwrite(compressedData, sizeof(char), compressedDataSize, outFp);

      // update offset table
      *offsetsEnd = *(offsetsEnd - 1) + compressedDataSize;
      offsetsEnd++;

      if (offsetsEnd - offsetsTable > offsetsCapacity)
         return -1;
   }

   // Write the tailing jump table
   *offsetsEnd++ = (offsetsEnd - offsetsTable);

   fwrite(offsetsTable, sizeof(SceOff), (offsetsEnd - offsetsTable), outFp);

   return 0;
}

int test_compress(FILE* outFp, FILE* inpFp, int block_bytes, const char* dicData, int64_t dicSize, int offsetsCapacity)
{
   char* compressedData = (char*)malloc(LZ4_COMPRESSBOUND(block_bytes));
   if (compressedData == 0)
      return -1;

   memset(compressedData, 0, LZ4_COMPRESSBOUND(block_bytes));

   char* rawData = (char*)malloc(block_bytes);
   if (rawData == 0)
   {
      free(compressedData);
      return -1;
   }

   memset(rawData, 0, block_bytes);

   SceOff* offsetsTable = (SceOff*)malloc(offsetsCapacity * sizeof(SceOff));
   if (offsetsTable == 0)
   {
      free(compressedData);
      free(rawData);
      return -1;
   }
   
   memset(offsetsTable, 0, offsetsCapacity * sizeof(SceOff));

   int res = test_compress_internal(outFp, inpFp, block_bytes, dicData, dicSize, offsetsTable, offsetsCapacity, compressedData, rawData);

   free(compressedData);
   free(rawData);
   free(offsetsTable);

   return res;
}

int test_decompress_internal(FILE* outFp, FILE* inpFp, SceOff dataOffset, int dataLength, int block_bytes, const char* dicData, int64_t dicSize, SceOff* offsetsTable, char* compressedData, char* decompressedData)
{
   LZ4_streamDecode_t lz4StreamDecode_body;
   LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

   // The blocks [currentBlock, endBlock) contain the data we want
   SceOff startBlock = dataOffset / block_bytes;
   SceOff endBlock = ((dataOffset + dataLength - 1) / block_bytes) + 1;

   // Seek to the first block to read
   _fseeki64(inpFp, offsetsTable[startBlock], SEEK_SET);
   SceOff offset = dataOffset % block_bytes;

   // Start decoding
   int length = dataLength;
   for (SceOff i = startBlock; i < endBlock; ++i)
   {
      // The difference in offsets is the size of the block
      int compressedDataSize = offsetsTable[i + 1] - offsetsTable[i];

      // read compressed data
      int readDataSize = fread(compressedData, sizeof(char), compressedDataSize, inpFp);
      if (readDataSize != compressedDataSize)
         return -1;

      //set zero size dictionary (this kinda resets the stream?)
      if (dicData == 0)
         LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);
      else
         LZ4_setStreamDecode(lz4StreamDecode, dicData, dicSize);

      //decompress data
      int decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, compressedData, decompressedData, compressedDataSize, block_bytes);
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

int test_decompress(FILE* outFp, FILE* inpFp, SceOff dataOffset, int dataLength, int block_bytes, const char* dicData, int64_t dicSize)
{
   if (dataLength == 0)
      return -1;

   char* compressedData = (char*)malloc(LZ4_COMPRESSBOUND(block_bytes));
   if (compressedData == 0)
      return -1;

   memset(compressedData, 0, LZ4_COMPRESSBOUND(block_bytes));

   char* decompressedData = (char*)malloc(block_bytes);
   if (decompressedData == 0)
   {
      free(compressedData);
      return -1;
   }

   memset(decompressedData, 0, block_bytes);

   SceOff endBlock = ((dataOffset + dataLength - 1) / block_bytes) + 1;

   // read number of offsets from tail
   SceOff numOffsets = 0;
   _fseeki64(inpFp, -((SceOff)sizeof(SceOff)), SEEK_END); //seek to number of offsets
   fread(&numOffsets, sizeof(SceOff), 1, inpFp);

   // validate offset arg
   if (numOffsets <= endBlock)
   {
      free(compressedData);
      free(decompressedData);
      return -1;
   }

   // allocate offsets table
   SceOff* offsetsTable = (SceOff*)malloc(numOffsets * sizeof(SceOff));
   if (offsetsTable == 0)
   {
      free(compressedData);
      free(decompressedData);
      return -1;
   }

   memset(offsetsTable, 0, numOffsets * sizeof(SceOff));

   //read offset table from tail
   _fseeki64(inpFp, -((SceOff)sizeof(SceOff)* (numOffsets + 1)), SEEK_END);
   fread(offsetsTable, sizeof(SceOff), numOffsets, inpFp);

   int res = test_decompress_internal(outFp, inpFp, dataOffset, dataLength, block_bytes, dicData, dicSize, offsetsTable, compressedData, decompressedData);

   free(compressedData);
   free(decompressedData);
   free(offsetsTable);

   return res;
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

int compress(const char* inpFilename, const char* lz4Filename, int block_bytes, const char* dicData, int64_t dicSize)
{
   FILE* inpFp = fopen(inpFilename, "rb");
   FILE* outFp = fopen(lz4Filename, "wb");

   int64_t fsize = get_file_size(inpFp);
   int64_t offsetsCapacity = fsize / block_bytes;

   //when filesize is smaller than block_bytes we guarantee that size is at least one
   //this guarantees that value will be rounded which will give extra safe space and space for one extra item in the end
   offsetsCapacity++; 
   int64_t offsetsCapacityRound = roundUp(offsetsCapacity, 10);

   printf("compress : %s -> %s\n", inpFilename, lz4Filename, block_bytes, offsetsCapacityRound);
   test_compress(outFp, inpFp, block_bytes, dicData, dicSize, offsetsCapacityRound);
   printf("compress : done\n");

   fclose(outFp);
   fclose(inpFp);

   return 0;
}

int decompress(const char* lz4Filename, const char* decFilename, SceOff offset, int length, int block_bytes, const char* dicData, int64_t dicSize)
{
   FILE* inpFp = fopen(lz4Filename, "rb");
   FILE* outFp = fopen(decFilename, "wb");

   printf("decompress : %s -> %s\n", lz4Filename, decFilename);
   test_decompress(outFp, inpFp, offset, length, block_bytes, dicData, dicSize);
   printf("decompress : done\n");

   fclose(outFp);
   fclose(inpFp);

   return 0;
}

int verify(const char* inpFilename, const char* decFilename, int offset, int length)
{
   FILE* inpFp = fopen(inpFilename, "rb");
   FILE* decFp = fopen(decFilename, "rb");
   _fseeki64(inpFp, offset, SEEK_SET);

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

int64_t load_dict(const char* dicFilename, char** dicDataArg)
{
   FILE* inpFp = fopen(dicFilename, "rb");

   size_t dicSize = get_file_size(inpFp);

   char* dicData = (char*)malloc(dicSize);
   if (dicData == 0)
   {
      fclose(inpFp);
      return 0;
   }

   memset(dicData, 0, dicSize);
   fread(dicData, sizeof(char), dicSize, inpFp);

   fclose(inpFp);

   *dicDataArg = dicData;
   return dicSize;
}

//memory allocation should be dynamic - no MAX_BLOCKS
//header should store information about type of compression. lz4 or smth else
//offset table should move to top

int main(int argc, char* argv[])
{
   const char* inpFilename = "test.bin";
   const char* lz4Filename = "test.bin.lz4";
   const char* decFilename = "test.bin.lz4.dec";
   const char* dicFilename = "dict.dic";

   char* dicData = 0;
   int64_t dicSize = load_dict(dicFilename, &dicData);

   //int offset = 10;
   //int length = 20900;

   int offset = 0;
   int length = 0x20910;

   int BLOCK_BYTES = 1024 * 64;
   //int BLOCK_BYTES = 1024 * 2048;
   //int BLOCK_BYTES = 1024 * 10;

   compress(inpFilename, lz4Filename, BLOCK_BYTES, NULL, NULL);
   decompress(lz4Filename, decFilename, offset, length, BLOCK_BYTES, NULL, NULL);
   verify(inpFilename, decFilename, offset, length);

   free(dicData);

   return 0;
}
