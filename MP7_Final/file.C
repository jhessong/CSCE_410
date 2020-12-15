/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File() {
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */
    Console::puts("In file constructor.\n");
    fd = -1;
    size = 0;
    curr_block = -1;
    index = 1;
    pos = 0;
    file_system = NULL;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char * _buf) {
    Console::puts("reading from file\n");
    if(curr_block == -1 || file_system == NULL) {
        Console::puts("File not initialized\n");
        return 0;
    }
    int read = 0;
    int to_read = _n;
    char buf[512];
    memset(buf, 0, 512);
    Console::puts("Reading from block ");
    Console::puti(curr_block);
    Console::puts("\n");
    file_system->disk->read(curr_block, (unsigned char*)buf);
    while(!EoF() && (to_read > 0)) {
        _buf[read] = buf[pos];
        to_read--;
        read++;
        pos++;
        if(pos >= 512) {
            index++;
            if(index > 15) {
                return read;
            }
            curr_block = blocks[index-1];
            memset(buf, 0, 512);
            file_system->disk->read(curr_block, (unsigned char*)buf);
            pos = 0;
        }
    }
    return read;
}


void File::Write(unsigned int _n, const char * _buf) {
    Console::puts("writing to file\n");
    if(curr_block == -1 || file_system == NULL) {
        if(file_system == NULL) {
            Console::puts("File system NULL\n");
        }
        Console::puts("File not initialized\n");
        return;
    }
    int write = 0;
    int to_write = _n;
    unsigned char buf[512];
    memset(buf, 0, 512);
    file_system->disk->read(curr_block, buf);
    while(to_write > 0) {
        buf[pos] = _buf[write];
        write++;
        pos++;
        to_write--;
        if(pos >= 512) {
            file_system->disk->write(curr_block, (unsigned char*)buf);
            curr_block = file_system->GetBlock();
            file_system->UpdateBlockData(fd, curr_block);
            memset(buf, 0, 512);
            file_system->disk->read(curr_block, (unsigned char*)buf);
            pos = 0;
        }
    }
    file_system->UpdateSize(write, fd, this);
    file_system->disk->write(curr_block, buf);
}

void File::Reset() {
    Console::puts("reset current position in file\n");
    pos = 0;
    curr_block = blocks[0];    
}

void File::Rewrite() {
    Console::puts("erase content of file\n");
    file_system->EraseFile(fd);
    for(int i = 1; i < 16; i++) {
        blocks[i] = 0;
    }
}


bool File::EoF() {
    //Console::puts("testing end-of-file condition\n");
    char buf[512];
    memset(buf, 0, 512);
    if((((index - 1)*512) + pos) > size) {
        return true;
    }
    return false;
}
