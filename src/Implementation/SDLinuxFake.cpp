#include <string.h>
#include <sys/stat.h>
#include "SDLinuxFake.h"



bool SDLinuxFake::begin(uint8_t csPin/* = SD_CHIP_SELECT_PIN*/) {
    _begin = true;
    std::system(("mkdir --parents " + _rootPath).c_str());
    return true;
}


void SDLinuxFake::setRootPath(std::string rootPath) {
    _rootPath = rootPath;
}


SDLinuxFake::~SDLinuxFake() {
}


FileLinuxFake SDLinuxFake::open(const char *filename, uint8_t mode) {
    std::string fullPath = _rootPath+ "/" + filename;
    std::system(("touch " + fullPath).c_str());

    return FileLinuxFake(fullPath.c_str(), filename, (bool)mode);
}


FileLinuxFake::FileLinuxFake() {

}


FileLinuxFake::FileLinuxFake(const char *fullPath, const char *name, bool rw) {
    _full_path = fullPath;
    strcpy(_name, name);
    _read = rw;
}

bool SDLinuxFake::exists(const char *filepath) {
    std::string full_path = _rootPath +  filepath;
    struct stat buffer;
    return (stat(full_path.c_str(), &buffer) == 0);
}

bool SDLinuxFake::remove(const char *filepath) {
    return !std::remove(filepath);
}



void FileLinuxFake::close() {
    _closed = true;
}


size_t FileLinuxFake::write(const uint8_t *buf, size_t size) {
    if (_closed) {
        return 0;
    }
    if (_read) {
        return 0;
    }
    // open file
    std::fstream _file;
    _file.open(_full_path);
    _file.seekg(_position);
    for (uint16_t i = 0; i < size; i++) {
        _file << *buf++;
    }
    _position += size;
    _file.close();
    return size;
}

size_t FileLinuxFake::write(const char *buf, size_t size) {
    if (_closed) {
        return 0;
    }
    if (_read) {
        return 0;
    }
    // open file
    std::fstream _file;
    _file.open(_full_path);
    _file.seekg(_position);
    for (uint16_t i = 0; i < size; i++) {
        _file << *buf++;
    }
    _position += size;
    _file.close();
    return size;
}

void FileLinuxFake::flush() {

}

int FileLinuxFake::read(void *buf, uint16_t nbyte) {
    if(!_read){
        return 0;
    }
    std::fstream _file;
    _file.open(_full_path);
    _file.seekg(_position);
    _file.read((char *) buf, nbyte);
    if (_file.rdstate() == std::ios_base::eofbit) {
        long read_bytes = _file.gcount();
        _position += read_bytes;
        _file.close();
        return read_bytes;
    } else if (_file.rdstate() == std::ios_base::failbit) {
        return 0;
    } else if (_file.rdstate() == std::ios_base::badbit) {
        return 0;
    }
    long read_bytes = _file.gcount();
    _position += read_bytes;
    _file.close();
    return read_bytes;
}

bool FileLinuxFake::seek(uint32_t pos) {
    _position = pos;
    return true;
}

int FileLinuxFake::read() {
    if(!_read){
        return 0;
    }
    std::fstream _file;
    _file.open(_full_path);
    _file.seekg(_position,std::fstream::beg);
    int c = _file.get();
    if (_file.rdstate() == std::ios_base::eofbit) {
        long read_bytes = _file.gcount();
        _position += read_bytes;
        _file.close();
        return c;
    } else if (_file.rdstate() == std::ios_base::failbit) {
        return 0;
    } else if (_file.rdstate() == std::ios_base::badbit) {
        return 0;
    }
    long read_bytes = _file.gcount();
    _position += read_bytes;
    _file.close();
    return c;
}











































