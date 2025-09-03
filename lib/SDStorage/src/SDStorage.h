#define SDMMC // using official TMR A32 board
// #define SDSPI // using SPI SD card Module

#if !defined(SDSTORAGE_H)
#define SDSTORAGE_H

#ifdef SDMMC

#include "FS.h"
#include "SD_MMC.h"
#include <Arduino.h>

class SDStorage
{
private:
public:
    fs::FS &card = SD_MMC; // Use SD_MMC instead of SD

    void init();
    String listDir(fs::FS &fs, const char *dirname, uint8_t levels);
    void createDir(fs::FS &fs, const char *path);
    void removeDir(fs::FS &fs, const char *path);
    void readFile(fs::FS &fs, const char *path);
    void writeFile(fs::FS &fs, const char *path, const char *message);
    void appendFile(fs::FS &fs, const char *path, const char *message);
    void renameFile(fs::FS &fs, const char *path1, const char *path2);
    void deleteFile(fs::FS &fs, const char *path);
    void testFileIO(fs::FS &fs, const char *path);
    bool fileExists(String filename);
};
#endif

#ifdef SDSPI
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Arduino.h>

class SDStorage
{
private:
public:
    fs::FS &card = SD;
    void init();
    String listDir(fs::FS &fs, const char *dirname, uint8_t levels);
    void createDir(fs::FS &fs, const char *path);
    void removeDir(fs::FS &fs, const char *path);
    void readFile(fs::FS &fs, const char *path);
    void writeFile(fs::FS &fs, const char *path, const char *message);
    void appendFile(fs::FS &fs, const char *path, const char *message);
    void renameFile(fs::FS &fs, const char *path1, const char *path2);
    void deleteFile(fs::FS &fs, const char *path);
    void testFileIO(fs::FS &fs, const char *path);
    bool fileExists(String filename);
};
#endif

#endif // SDSTORAGE_H
