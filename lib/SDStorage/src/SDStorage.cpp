#include <SDStorage.h>

#ifdef SDMMC
#ifdef ESP32
#define BOOT_OPT_PIN 2
#endif
void SDStorage::init()
{
    if (!SD_MMC.begin("/sdcard", false, false, 2000000, 5U))
    {
        Serial.println("Card Mount Failed");
        return;
    }

    uint8_t cardType = SD_MMC.cardType();

    if (cardType == CARD_NONE)
    {
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC)
    {
        Serial.println("MMC");
    }
    else if (cardType == CARD_SD)
    {
        Serial.println("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        Serial.println("SDHC");
    }
    else
    {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

String SDStorage::listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{

    File root = fs.open(dirname);
    if (!root || !root.isDirectory())
    {
        return "{\"filename\":[],\"filesize\":[]}";
    }

    String fileInfo = "{\"filename\":[";
    String fileSizes = "\"filesize\":[";
    uint16_t fileIndex = 0;

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            if (levels)
            {
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            if (fileIndex > 0)
            {
                fileInfo += ",";
                fileSizes += ",";
            }
            fileInfo += "\"";
            fileInfo += String(file.path());
            fileInfo += "\"";

            fileSizes += String(file.size());
            fileIndex++;
        }
        file = root.openNextFile();
    }

    fileInfo += "],";
    fileSizes += "]}";

    return fileInfo + fileSizes;
}

void SDStorage::createDir(fs::FS &fs, const char *path)
{
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path))
    {
        Serial.println("Dir created");
    }
    else
    {
        Serial.println("mkdir failed");
    }
}

void SDStorage::removeDir(fs::FS &fs, const char *path)
{
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path))
    {
        Serial.println("Dir removed");
    }
    else
    {
        Serial.println("rmdir failed");
    }
}

String SDStorage::readFile(fs::FS &fs, const char *path)
{
    // with this function uses more memory alocation because "buffer" variable below
    String buffer = "";
    Serial.printf("Reading file: %s\n", path);
    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return "";
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print("Read from file: ");
    while (file.available())
    {
        buffer += file.readString();
    }
    file.close();
    return buffer;
}

void SDStorage::readFile(fs::FS &fs, const char *path, String *readMemReturn)
{
    // with this function it uses a String pointer to reduce memory usage
    Serial.printf("Reading file: %s\n", path);
    File file = fs.open(path, "r");
    file.setTimeout(5000);
    file.setBufferSize(1024);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print("Read from file: ");
    while (file.available())
    {
        *readMemReturn += String((char)file.read());
    }
    file.close();
}

void SDStorage::writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("File written");
    }
    else
    {
        Serial.println("Write failed");
    }
    file.close();
}

void SDStorage::appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\n", path);
    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("Message appended");
    }
    else
    {
        Serial.println("Append failed");
    }
    file.close();
}

void SDStorage::renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("File renamed");
    }
    else
    {
        Serial.println("Rename failed");
    }
}

void SDStorage::deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path))
    {
        Serial.println("File deleted");
    }
    else
    {
        Serial.println("Delete failed");
    }
}

void SDStorage::testFileIO(fs::FS &fs, const char *path)
{
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;

    if (file)
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
                toRead = 512;
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    }
    else
    {
        Serial.println("Failed to open file for reading");
    }

    file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }

    start = millis();
    for (size_t i = 0; i < 2048; i++)
    {
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

bool SDStorage::fileExists(String filename)
{
    return SD_MMC.exists(filename.c_str());
}

void SDStorage::copyFile(fs::FS &fs, const char *masterFile, const char *resultFile)
{
    Serial.printf("Copy file: %s\n", masterFile);
    Serial.printf("To file: %s\n", resultFile);
    File file = fs.open(masterFile, "r");
    file.setTimeout(5000);
    file.setBufferSize(32);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    File copy = fs.open(resultFile, "w", true);
    copy.setTimeout(5000);
    copy.setBufferSize(32);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print("Read from file: ");
    while (file.available())
    {
        char charToWrite = (char)file.read();
        copy.print(charToWrite);
    }
    file.close();
    copy.close();
}

void SDStorage::copyFileByLines(fs::FS &fs, const char *masterFile, const char *resultFile, uint32_t numOfLines)
{
    uint32_t linesFound = 0;
    Serial.printf("Copy file: %s\n", masterFile);
    Serial.printf("To file: %s\n", resultFile);
    File file = fs.open(masterFile, "r");
    file.setTimeout(5000);
    file.setBufferSize(32);

    vTaskDelay(500 / portTICK_PERIOD_MS);

    File copy = fs.open(resultFile, "w", true);
    copy.setTimeout(5000);
    copy.setBufferSize(32);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print("Read from file: ");
    while (file.available())
    {
        char charToWrite = (char)file.read();
        copy.print(charToWrite);
        if (charToWrite == '\n')
        {
            linesFound++;
        }
        if (linesFound >= numOfLines)
        {
            break;
        }
    }
    file.close();
    copy.close();
}
void SDStorage::substractFile(fs::FS &fs, const char *masterFile, const char *substractor)
{
    File master = fs.open(masterFile, "r");
    File sbFile = fs.open(substractor, "r");
    File temporaryResult = fs.open("/subTMP.csv", "w", true);

    bool subIsBlank = false;

    master.setTimeout(5000);
    sbFile.setTimeout(5000);
    temporaryResult.setTimeout(5000);
    master.setBufferSize(512);
    sbFile.setBufferSize(512);
    temporaryResult.setBufferSize(512);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    String masterLine = "";
    String sbFileLine = "";
    while (master.available())
    {
        masterLine = master.readStringUntil('\n');
        masterLine += '\n';
        if (!subIsBlank)
        {
            sbFileLine += sbFile.readStringUntil('\n');
            sbFileLine += '\n';
            if (sbFileLine.length() < 10)
            {
                subIsBlank = true;
            }
        }
        else
        {
            sbFileLine = "";
        }
        // Serial.print("master:");
        // Serial.println(masterLine);
        // Serial.print("sbustractor:");
        // Serial.println(sbFileLine);
        if (masterLine == sbFileLine) // if the line is same, then don't write into the result file
        {
            masterLine = "";
            sbFileLine = "";
        }
        else
        {
            temporaryResult.print(masterLine); // print the unmatch lines with the original data from master file
        }
    }
    temporaryResult.close();
    master.close();
    sbFile.close();
    this->deleteFile(fs, masterFile);
    this->copyFile(fs, "/subTMP.csv", masterFile);
    this->deleteFile(fs, "/subTMP.csv");
}

uint32_t SDStorage::contentCount(fs::FS &fs, const char *path)
{
    uint32_t cnt = 0;
    File file = fs.open(path, "r");
    file.setTimeout(2000);
    file.setBufferSize(32);
    if (!file)
    {
        Serial.println("Failed to open file for counting");
        return cnt;
    }
    while (file.available())
    {
        cnt++;
    }
    file.close();
    return cnt;
}

#endif
#ifdef SDSPI
void SDStorage::init()
{
    if (!SD.begin(13))
    {
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        Serial.println("No SD card attached");
        return;
    }
    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC)
    {
        Serial.println("MMC");
    }
    else if (cardType == CARD_SD)
    {
        Serial.println("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        Serial.println("SDHC");
    }
    else
    {
        Serial.println("UNKNOWN");
    }
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

// String SDStorage::listDir(fs::FS &fs, const char *dirname, uint8_t levels)
// {
//     Serial.printf("Listing directory: %s\n", dirname);

//     File root = fs.open(dirname);
//     if (!root)
//     {
//         // Serial.println("Failed to open directory");
//         return "{\"filename\":[],\"filesize\":[]}";
//     }
//     if (!root.isDirectory())
//     {
//         // Serial.println("Not a directory");
//         return "{\"filename\":[],\"filesize\":[]}";
//     }
//     uint16_t fileIndex = 0;
//     String fileInfo = "{\"filename\":[";
//     File file = root.openNextFile();
//     while (file)
//     {
//         if (file.isDirectory())
//         {
//             // Serial.print("  DIR : ");
//             // Serial.println(file.name());
//             if (levels)
//             {
//                 listDir(fs, file.name(), levels - 1);
//             }
//         }
//         else
//         {
//             // Serial.print("  FILE: ");
//             // Serial.print(file.name());
//             // Serial.print("  SIZE: ");
//             // Serial.println(file.size());
//             if (fileIndex > 0)
//             {
//                 fileInfo += ",";
//             }
//             fileInfo += "\"";
//             fileInfo += String(file.name());
//             fileInfo += "\"";
//             fileIndex++;
//         }
//         file = root.openNextFile();
//     }
//     fileInfo += "]}";
//     return fileInfo;
// }

String SDStorage::listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    File root = fs.open(dirname);
    if (!root || !root.isDirectory())
    {
        return "{\"filename\":[],\"filesize\":[]}";
    }

    String fileInfo = "{\"filename\":[";
    String fileSizes = "\"filesize\":[";
    uint16_t fileIndex = 0;

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            if (levels)
            {
                // Recursively list subdirectories, but discard the result (optional: you could merge the subdirectory results here)
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            if (fileIndex > 0)
            {
                fileInfo += ",";
                fileSizes += ",";
            }
            fileInfo += "\"";
            fileInfo += String(file.path()); // Use file.path() for the full path
            fileInfo += "\"";

            fileSizes += String(file.size());
            fileIndex++;
        }
        file = root.openNextFile();
    }

    fileInfo += "],";
    fileSizes += "]}";

    return fileInfo + fileSizes;
}

void SDStorage::createDir(fs::FS &fs, const char *path)
{
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path))
    {
        Serial.println("Dir created");
    }
    else
    {
        Serial.println("mkdir failed");
    }
}

void SDStorage::removeDir(fs::FS &fs, const char *path)
{
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path))
    {
        Serial.println("Dir removed");
    }
    else
    {
        Serial.println("rmdir failed");
    }
}

void SDStorage::readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
}

void SDStorage::writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("File written");
    }
    else
    {
        Serial.println("Write failed");
    }
    file.close();
}

void SDStorage::appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("Message appended");
    }
    else
    {
        Serial.println("Append failed");
    }
    file.close();
}

void SDStorage::renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("File renamed");
    }
    else
    {
        Serial.println("Rename failed");
    }
}

void SDStorage::deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path))
    {
        Serial.println("File deleted");
    }
    else
    {
        Serial.println("Delete failed");
    }
}

void SDStorage::testFileIO(fs::FS &fs, const char *path)
{
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if (file)
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    }
    else
    {
        Serial.println("Failed to open file for reading");
    }

    file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for (i = 0; i < 2048; i++)
    {
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

bool SDStorage::fileExists(String filename)
{
    return (bool)SD.exists(filename.c_str());
}
#endif