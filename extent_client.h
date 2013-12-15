// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "extent_server.h"
#include "pthread.h"
#include "lock_client_cache.h"

using namespace std;

class extent_cache {
public:
	string txt;
	extent_protocol::attr at;
	bool isdirty;
  bool isadirty;
	bool hastxt;
	bool hasattr;
	extent_cache();
	~extent_cache();
};

class extent_client {
 private:
  rpcc *cl;
  pthread_mutex_t mut;
  map<extent_protocol::extentid_t,extent_cache> extcache;

 public:
  extent_client(std::string dst);
  ~extent_client();

  extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);
  extent_protocol::status get(extent_protocol::extentid_t eid, 
			                        std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				                          extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  extent_protocol::status flush(extent_protocol::extentid_t eid);
};

class flush_call:public lock_release_user {
public:
  extent_client *ec;
  void dorelease(extent_protocol::extentid_t eid)
  {
    ec->flush(eid);
  };
};
#endif 

