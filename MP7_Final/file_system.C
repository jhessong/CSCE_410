/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define NODES_PER_BLOCK (BLOCK_SIZE/sizeof(m_node))

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    FileSystem::disk = NULL;
    num_blocks = 0;
    m_blocks = 0;
    m_nodes = 0;
    size = 0;
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system form disk\n");
    if (disk == NULL) {
        disk = _disk;
        return true;
    }
    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {
    Console::puts("formatting disk\n");
    FileSystem::disk = _disk;
    FileSystem::size = _size;
    FileSystem::num_blocks = (FileSystem::size / BLOCK_SIZE) + 1;
    FileSystem::m_nodes = (FileSystem::num_blocks / BLOCK_LIMIT) + 1;
    FileSystem::m_blocks = ((FileSystem::m_nodes * sizeof(m_node)) / BLOCK_SIZE) + 1;

    for(int j = 0; j < (num_blocks/8); j++) {
        block_map[j] = 0;
    }
    
    int i;
    for(i = 0; i < (m_blocks/8); i++) {
        FileSystem::block_map[i] = 0xFF;
    }

    block_map[i] = 0;
    for(int j = 0; j < (m_blocks%8); j++) {
        FileSystem::block_map[i] = block_map[i] | (i << j);
    }

    char buf[512];
    memset(buf, 0, 512);
    for(int j = 0; j <num_blocks; j++) {
        disk->write(j, (unsigned char*)buf);
    }
    return true;
}

File * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file\n");
    File* file = (File*) new File();
    char buf[512];
    memset(buf, 0, 512);
    for(int i = 0; i < m_blocks; i++) {
        memset(buf, 0, 512);
        disk->read(i, (unsigned char*)buf);
        m_node* m_node_l = (m_node*)buf;
        for(int j = 0; j < NODES_PER_BLOCK; j++) {
            if(m_node_l[j].fd == _file_id) {
                file->fd = _file_id;
                file->size = m_node_l[j].size;
                file->curr_block = m_node_l[j].block[0];
                file->index = 1;
                file->pos = 0;
                for(int k = 0; k < BLOCK_LIMIT; k++) {
                    file->blocks[k] = m_node_l[j].block[k];
                }
                file->file_system = FILE_SYSTEM;
                if(file->file_system == NULL) {
                    Console::puts("File system NULL in lookup\n");
                }
                Console::puts("Found file with id ");
                Console::puti(_file_id);
                Console::puts("\n");
                return file;
            }
        }
    }
    return NULL;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file\n");
    if(LookupFile(_file_id) != NULL) {
        Console::puts("File already exists with this id\n");
        return false;
    }
    char buf[512];
    memset(buf, 0, 512);
    for(int i = 0; i < m_blocks; i++) {
        memset(buf, 0, 512);
        disk->read(i, (unsigned char*)buf);
        m_node* m_node_l = (m_node*)buf;
        for(int j = 0; j < NODES_PER_BLOCK; j++) {
            if(m_node_l[j].fd == 0) {
                m_node_l[j].fd = _file_id;
                m_node_l[j].block[0] = GetBlock();
                Console::puts("get block ");
                Console::puti(m_node_l[j].block[0]);
                Console::puts("\n");
                m_node_l[j].b_size = 1;
                disk->write(i, (unsigned char*)buf);
                return true;
            }
        }
    }
    return false;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file\n");
    char buf[512];
    memset(buf, 0, 512);
    for(int i = 0; i < m_blocks; i++) {
        memset(buf, 0, 512);
        disk->read(i, (unsigned char*)buf);
        m_node* m_node_l = (m_node*)buf;
        for(int j = 0; j < NODES_PER_BLOCK; j++) {
            if(m_node_l[j].fd == _file_id) {
                m_node_l[j].fd = 0;
                m_node_l[j].size = 0;
                m_node_l[j].b_size = 0;
                for(int k = 0; k < BLOCK_LIMIT; k++){
                    if(m_node_l[j].block[k] != 0) {
                        FreeBlock(m_node_l[j].block[k]);
                    }
                    m_node_l[j].block[k] = 0;
                }
                disk->write(i, (unsigned char*)buf);
                return true;
            }
        }
    }
    Console::puts("File not found\n");
    return false;
}

void FileSystem::EraseFile(int _file_id) {
    Console::puts("Erasing content of file\n");
    char buf[512];
    char buf2[512];
    memset(buf, 0, 512);
    memset(buf2, 0, 512);
    for(int i = 0; i < m_blocks; i++) {
        memset(buf, 0, 512);
        disk->read(i, (unsigned char*)buf);
        m_node* m_node_l = (m_node*)buf;
        for(int j = 0; j < NODES_PER_BLOCK; j++) {
            if(m_node_l[j].fd == _file_id) {
                m_node_l[j].size = 0;
                m_node_l[j].b_size = 0;
                for(int k = 0; k < BLOCK_LIMIT; k++) {
                    if(m_node_l[j].block[k] != 0) {
                        disk->write(m_node_l[j].block[k], (unsigned char*)buf2);
                        if(k != 0) {
                            FreeBlock(m_node_l[j].block[k]);
                            m_node_l[j].block[k] = 0;
                        }
                    }
                }
                disk->write(i, (unsigned char*)buf);
                return;
            }
        }
    }
}

int FileSystem::GetBlock() {
    Console::puts("Total blocks: ");
    Console::puti(num_blocks/8);
    Console::puts("\n");
    for(int i = 0; i < (num_blocks/8); i++) {
        if(block_map[i] != 0xFF) {
            for(int j = 0; j < 8; j++) {
                if(block_map[i] & (1<<j)) {
                    continue;
                }
                else {
                    block_map[i] = block_map[i] | (1<<j);
                    int b = j + i*8;
                    Console::puts("Block ");
                    Console::puti(b);
                    Console::puts(" allocated.\n");
                    return b;
                }
            }
        }
    }
    Console::puts("returning block 0\n");
    return 0;
}

void FileSystem::FreeBlock(int block_num) {
    int node = block_num / 8;
    int index = block_num % 8;
    block_map[node] = block_map[node] | (1<<index);
    block_map[node] = block_map[node] ^ (1<<index);
}

void FileSystem::UpdateSize(long size, unsigned long fd, File* file) {
    Console::puts("Updating block size\n");
    char buf[512];
    memset(buf, 0, 512);
    for(int i = 0; i < m_blocks; i++) {
        memset(buf, 0, 512);
        disk->read(i, (unsigned char*)buf);
        m_node* m_node_l = (m_node*)buf;
        for(int j = 0; j < NODES_PER_BLOCK; j++) {
            if(m_node_l[j].fd == fd) {
                m_node_l[j].size += size;
                file->size = m_node_l[j].size;
                disk->write(i, (unsigned char*)buf);
                return;
            }
        }
    }
    Console::puts("File with given fd not found\n");
    return;
}

void FileSystem::UpdateBlockData(int fd, int block) {
    Console::puts("Updating block data\n");
    char buf[512];
    memset(buf, 0, 512);
    for (int i = 0; i < m_blocks; i++) {
        memset(buf, 0, 512);
        disk->read (i, (unsigned char*)buf);
        m_node* m_node_l = (m_node*)buf;
        for (int j = 0; j < NODES_PER_BLOCK; j++) {
            if (m_node_l[j].fd == fd) {
                m_node_l[j].b_size += 1;
                m_node_l[j].block[m_node_l[j].b_size] = block;
                disk->write(i, (unsigned char *)buf);
                return;
            }
        }
    }
    Console::puts("File with given fd not found\n");
    return;
}

