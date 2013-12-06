// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  lc = new lock_client(lock_dst);
  if (ec->put(1, "") != extent_protocol::OK)
      printf("error init root dir\n"); // XYB: init root dir
}


yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
    return false;
}

bool
yfs_client::isdir(inum inum)
{
    return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
/*    std::string old;
    if(ec->get(ino,old) != extent_protocol::OK)
    {
    	r = IOERR;
    	return r;
    }
    old.resize(size,'\0');
    if(ec->put(ino,old) != extent_protocol::OK)
    {
    	r = IOERR;
    	return r;
    }

    return r;*/

    extent_protocol::attr a;
    if(ec->getattr(ino, a) != extent_protocol::OK){
        r = IOERR;
        return r;
    }
    if(a.size > size){
    std::string str;
    if(ec->get(ino, str) != extent_protocol::OK){
        r = IOERR;
        return r;
    }
    str = str.substr(0, size);
    if(ec->put(ino, str) != extent_protocol::OK){
        r = IOERR;
        return r;
    }
    }
    if(a.size < size){
    std::string str;
    if(ec->get(ino, str) != extent_protocol::OK){
        r = IOERR;
        return r;
    }
    str.resize(size, '\0');
    if(ec->put(ino, str) != extent_protocol::OK){
        r = IOERR;
        return r;
    }
    }
    a.size = size;
    return r;

}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out, extent_protocol::types type)
{
    int r = OK;
    lc->acquire(parent);

    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    inum file_inum;
    bool found;
    lookup(parent, name, found, file_inum);
    if(found){
        r = EXIST;
        lc->release(parent);
        return r;
    }
    if(ec->create(type, file_inum) != extent_protocol::OK){
        r = IOERR;
        lc->release(parent);
        return r;
    }
    if (ec->put(file_inum, "") != extent_protocol::OK){
        r = IOERR;
        lc->release(parent);
        return r;
    }
    std::string buf;
    if(ec->get(parent, buf) != extent_protocol::OK){
        r = IOERR;
        lc->release(parent);
        return r;
    }
    buf.append("/" + filename(file_inum) + "/" + name);
    if (ec->put(parent, buf) != extent_protocol::OK){
        r = IOERR;
        lc->release(parent);
        return r;
    }
    ino_out = file_inum;

    lc->release(parent);
    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    if(!isdir(parent)){
        r = IOERR;
        return r;
    }
    std::list<dirent> list;
    if(readdir(parent, list) != extent_protocol::OK){
        r = IOERR;
        return r;
    }
    std::string filename(name);
    std::list<dirent>::iterator it;
    for(it=list.begin(); it!=list.end(); it++){
        if(it->name == filename){
            found = true;
            ino_out = it->inum;
            return r;
        }
    }
    found = false;
    r = NOENT;

    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;
    //lc->acquire(dir);

    /*
     * your lab2 code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */

    if(!isdir(dir)){
        r = IOERR;
        //lc->release(dir);
        return r;
    }
    std::string buf;
    dirent d;
    if(ec->get(dir, buf) != extent_protocol::OK){
        r = IOERR;
        //lc->release(dir);
        return r;
    }
    int level = 0;
    int pos = 0;
    std::string ino(""), filename("");
    char ch;
    while(true){
        if(pos == buf.size()){
            if(level == 2){
                d.inum = n2i(ino);
                d.name = filename;
                list.push_back(d);
            }
            break;
        }
        ch = buf[pos];
        switch(level){
        case 0:
            if(ch != '/'){
                ino += ch;
                level = 1;
            }
            break;
        case 1:
            if(ch != '/'){
                ino += ch;
            } else {
                level = 2;
            }
            break;
        case 2:
            if(ch != '/'){
                filename += ch;
            } else {
                level = 0;
                d.inum = n2i(ino);
                d.name = filename;
                list.push_back(d);
                ino = filename = "";
            }
            break;
        }
        pos ++;
    }
    //lc->release(dir);
    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;
    lc->acquire(ino);

    /*
     * your lab2 code goes here.
     * note: read using ec->get().
     */
    std::string tmp;
    if(ec->get(ino, tmp) != extent_protocol::OK){
        r = IOERR;
        lc->release(ino);
        return r;
    }
    data = tmp.substr(off, size);
    lc->release(ino);
    return r;
}


int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;
    lc->acquire(ino);

    /*
     * your lab2 code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    extent_protocol::attr a;
    if(ec->getattr(ino, a) != extent_protocol::OK){
        r = IOERR;
        lc->release(ino);
        return r;
    }
    int ut_size;
    if(off+size < a.size){
        ut_size = a.size;
        bytes_written = size;
    } else {
        ut_size = off+size;
        if(off > a.size){
            bytes_written = off - a.size + size;
        } else {
            bytes_written = size;
        }
    }
    if(setattr(ino, ut_size) != extent_protocol::OK){
        r = IOERR;
        lc->release(ino);
        return r;
    }
    std::string buf;
    if(ec->get(ino, buf) != extent_protocol::OK){
        r = IOERR;
        lc->release(ino);
        return r;
    }
    buf.replace(off, size, data, size);
    if(ec->put(ino, buf) != extent_protocol::OK){
        r = IOERR;
        lc->release(ino);
        return r;
    }
    lc->release(ino);
    return r;

}


int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;
    lc->acquire(parent);

    /*
     * your lab2 code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    inum ino;
    bool found;
    lookup(parent, name, found, ino);
    if(!found){
        r == NOENT;
        lc->release(parent);
        return r;
    }
    if(isdir(ino)){
        r = IOERR;
        lc->release(parent);
        return r;
    }
    if (ec->remove(ino) != extent_protocol::OK){
        r = IOERR;
        lc->release(parent);
        return r;
    }
    std::string buf;
    if (ec->get(parent, buf) != extent_protocol::OK){
        r = NOENT;
        lc->release(parent);
        return r;
    }
    std::string tmp = "/" + filename(ino) + "/" + name;
    buf.erase(buf.find(tmp), tmp.length());
    if (ec->put(parent, buf) != extent_protocol::OK){
        r = IOERR;
        lc->release(parent);
        return r;
    }
    lc->release(parent);
    return r;
}

