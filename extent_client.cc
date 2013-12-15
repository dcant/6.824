// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

extent_cache::extent_cache()
{
  isdirty = false;
  hastxt = false;
  hasattr = false;
}
extent_cache::~extent_cache()
{}

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
  pthread_mutex_init(&mut, NULL);
}
extent_client::~extent_client()
{
  pthread_mutex_lock(&mut);
  pthread_mutex_destroy(&mut);
  // map<extent_protocol::extentid_t,extent_cache*>::iterator it;
  // for(it = extcache.begin(); it != extcache.end(); it++)
  // {
  //   delete it->second;
  // }
}

// a demo to show how to use RPC
extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&mut);
  printf("\tGetattr: eid = %d\n",eid);
  if((extcache.count(eid) > 0) && (extcache[eid].hasattr == true))
  {
    // attr = extcache[eid]->at; whether this is OK?
    // attr.type = extcache[eid]->at.type;
    // attr.atime = extcache[eid]->at.atime;
    // attr.ctime = extcache[eid]->at.ctime;
    // attr.mtime = extcache[eid]->at.mtime;
    // attr.size = extcache[eid]->at.size;

    attr.type = extcache[eid].at.type;
    attr.atime = extcache[eid].at.atime;
    attr.ctime = extcache[eid].at.ctime;
    attr.mtime = extcache[eid].at.mtime;
    attr.size = extcache[eid].at.size;
  }else{
    ret = cl->call(extent_protocol::getattr, eid, attr);
    if(extent_protocol::OK == ret)
    {
      // if(extcache.count(eid) ==0)
      //   extcache[eid] = new extent_cache();
      // extcache[eid]->at.type = attr.type;
      // extcache[eid]->at.atime = attr.atime;
      // extcache[eid]->at.ctime = attr.ctime;
      // extcache[eid]->at.mtime = attr.mtime;
      // extcache[eid]->at.size = attr.size;
      // extcache[eid]->hasattr = true;

      if(extcache.count(eid) == 0)
      {
        extcache[eid].hastxt = false;
        extcache[eid].isdirty =false;
      }


      extcache[eid].at.type = attr.type;
      extcache[eid].at.atime = attr.atime;
      extcache[eid].at.ctime = attr.ctime;
      extcache[eid].at.mtime = attr.mtime;
      if(!extcache[eid].isadirty)
        extcache[eid].at.size = attr.size;
      extcache[eid].hasattr = true;
    }
  }
  pthread_mutex_unlock(&mut);
  return ret;
}

extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &id)
{
  extent_protocol::status ret = extent_protocol::OK;
  // Your lab3 code goes here
  pthread_mutex_lock(&mut);
  ret = cl->call(extent_protocol::create, type, id);
  pthread_mutex_unlock(&mut);
  return ret;
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&mut);
  if((extcache.count(eid) > 0) && (extcache[eid].hastxt == true))
  {
    buf = extcache[eid].txt;
    extcache[eid].at.atime = time(NULL);
    std::cout<<"\t"<<"Get: "<<buf<<endl;
  }else{
    ret = cl->call(extent_protocol::get, eid, buf);
    std::cout<<"\t"<<"Get: "<<buf<<endl;
    if(extent_protocol::OK == ret)
    {
      // if(extcache.count(eid) == 0)
      //   extcache[eid] = new extent_cache();

      if(extcache.count(eid) == 0)
      {
        extcache[eid].hasattr = false;
        extcache[eid].isdirty = false;
      }
      extcache[eid].txt = buf;
      extcache[eid].hastxt = true;
    }
  }
  printf("\tGet: eid = %d\n",eid);

  pthread_mutex_unlock(&mut);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&mut);
//  printf("\tPut: eid = %d\n",eid);
  cout<<"\tPut: eid = "<<eid<<" buf = "<<buf<<endl;
  // if(extcache.count(eid) == 0)
  //   extcache[eid] = new extent_cache();

  if(extcache.count(eid) == 0)
  {
    extcache[eid].hastxt = true;
  }

  extcache[eid].txt = buf;
  extcache[eid].isdirty = true;
  extcache[eid].isadirty = true;
  extcache[eid].at.atime = time(NULL);
  extcache[eid].at.ctime = time(NULL);
  extcache[eid].at.mtime = time(NULL);
  extcache[eid].at.size = buf.size(); 
  pthread_mutex_unlock(&mut); 
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&mut);
  int r;
  if(extcache.count(eid) > 0)
  {
//    delete extcache[eid];
    extcache.erase(eid);
  }
  ret = cl->call(extent_protocol::remove, eid, r);
  pthread_mutex_unlock(&mut);
  return ret;
}

extent_protocol::status
extent_client::flush(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  pthread_mutex_lock(&mut);
  if(0 == extcache.count(eid))
  {
//    ret = cl->call(extent_protocol::remove, eid, r);
  }else if(true == extcache[eid].isdirty){
    ret = cl->call(extent_protocol::put, eid, extcache[eid].txt, r);
//    delete extcache[eid];
    extcache.erase(eid);
  }else{
//    delete extcache[eid];
    extcache.erase(eid);
  }
  pthread_mutex_unlock(&mut);
  return ret;
}

