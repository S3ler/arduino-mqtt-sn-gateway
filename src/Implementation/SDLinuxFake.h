#ifndef DATABASE_SD_H
#define DATABASE_SD_H

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <list>
#include <cstddef>

#define O_READ  std::ios::in
#define O_WRITE std::ios::out

//#define O_CREAT 255

//#define O_RDONLY O_READ

#define FILE_READ   1
#define FILE_WRITE  0
//#define FILE_WRITE (O_READ | O_WRITE | O_CREAT)

/**
 * Fake implementation using the same function signatures as the File classes from the Arduino SD Library.
 */
class FileLinuxFake {
private:
    char _name[13]; // our name
    std::string _full_path;
    uint32_t _length;
    bool _read;
    long _position=0;
    bool _closed = false;

public:
    ~FileLinuxFake(){ };
    FileLinuxFake(const char *fullPath, const char *name, bool rw);     // wraps an underlying SdFile
    FileLinuxFake();

    size_t write(const uint8_t *buf, size_t size);
    size_t write(const char *buf, size_t size);
    int read(void *buf, uint16_t nybte);
    int read();
    void flush();
    bool seek(uint32_t pos);
    void close();

};
class SDLinuxFake {
public:
    std::string _rootPath ;
private:
    bool _begin = false;
public:
    ~SDLinuxFake();

    void setRootPath(std::string rootPath);


    bool begin(uint8_t csPin/* = SD_CHIP_SELECT_PIN*/);

    FileLinuxFake open(const char *filename, uint8_t mode = FILE_READ);

    bool exists(const char *filepath);

    // Delete the file.
    bool remove(const char *filepath);


private:
    friend class FileLinuxFake;
    // friend boolean callback_openPath(SdFile&, const char *, boolean, void *);
};


#endif //DATABASE_SD_H
