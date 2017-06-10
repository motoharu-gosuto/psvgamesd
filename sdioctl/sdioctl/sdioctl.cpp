#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdint.h>

#include <boost/filesystem.hpp>

int lock_volume(HANDLE hVol)
{
   DWORD bytesReturned = 0;
   return DeviceIoControl(hVol, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
}

int unlock_volume(HANDLE hVol)
{
  DWORD bytesReturned = 0;
  return DeviceIoControl(hVol, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
}

int write_image(uintmax_t filesize, std::ifstream& ifs, HANDLE hVol, int32_t bytesPerSector, int32_t sectorsPerCluster)
{
   if(lock_volume(hVol) == 0)
      return GetLastError();

   int64_t totalSize = bytesPerSector * sectorsPerCluster;
   char* buffer = new char[totalSize];

   SetFilePointer(hVol, 0, 0, FILE_BEGIN);

   int64_t nClusters = filesize / totalSize;
   int64_t nSectors = filesize % totalSize;

   for(int64_t n = 0; n < nClusters; n++)
   {
      if((n % 1000) == 0)
      {
         std::cout << n << " out of " << nClusters << std::endl;
      }

      ifs.read(buffer, totalSize);

      DWORD numberOfBytesWritten = 0;
      BOOL res = WriteFile(hVol, buffer, totalSize, &numberOfBytesWritten, NULL);

      if(numberOfBytesWritten != totalSize || res == 0)
      {
         delete [] buffer;
         return GetLastError();
      }
   }

   for(int64_t n = 0; n < nSectors; n++)
   {
      ifs.read(buffer, bytesPerSector);

      DWORD numberOfBytesWritten = 0;
      BOOL res = WriteFile(hVol, buffer, bytesPerSector, &numberOfBytesWritten, NULL);

      if(numberOfBytesWritten != bytesPerSector || res == 0)
      {
         delete [] buffer;
         return GetLastError();
      }
   }

   unlock_volume(hVol);

   delete [] buffer;
   return 0;
}

int write_image(boost::filesystem::path srcImage, std::wstring destDrive, int32_t bytesPerSector, int32_t sectorsPerCluster)
{
   HANDLE hVol = CreateFile (destDrive.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

   if (hVol == INVALID_HANDLE_VALUE) 
   {
      return GetLastError();
   }

   std::ifstream ifs(srcImage.generic_string().c_str(), std::ios::in | std::ios::binary);

   uintmax_t fileSize = boost::filesystem::file_size(srcImage);

   int res = write_image(fileSize, ifs, hVol, bytesPerSector, sectorsPerCluster);
   if(res != 0)
      std::cout << "Failed to write image" << std::endl;

   ifs.close();

   CloseHandle(hVol);
}

int check_read(std::wstring destDrive, int32_t bytesPerSector)
{
   HANDLE hVol = CreateFile (destDrive.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

   if (hVol == INVALID_HANDLE_VALUE) 
   {
      return GetLastError();
   }

   char* buffer = new char[bytesPerSector];
   DWORD numberOfBytesRead = 0;
   ReadFile(hVol, buffer, bytesPerSector, &numberOfBytesRead, NULL);

   CloseHandle(hVol);
}

//"D:\\dumps\\cartridge\\SAO_Hollow_Fragment.bin", L"\\\\.\\e:"

int main(int argc, char* argv[])
{
   if(argc < 2)
   {
      std::cout << "Wrong number of arguments" << std::endl;
      return -1;
   }

   std::string dests(argv[2]);
   std::wstring destw;
   std::copy(dests.begin(), dests.end(), std::back_inserter(destw));

   write_image(argv[1], destw, 0x200, 0x08);

   //check_read(L"\\\\.\\e:", 0x200);

	return 0;
}

