#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>


#include <map>
#include <set>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"


using namespace std;

class lock_info_cache_s {
public:
	//bool islocked;
	//bool waitid;
  //bool istrying;
  bool isrevoked;
	string holdid;
  set<string> waitid;
  //pthread_mutex_t trymut;
	lock_info_cache_s();
	~lock_info_cache_s();
};

class lock_server_cache {
 private:
  int nacquire;
  map<lock_protocol::lockid_t, lock_info_cache_s*> locks_s;
  pthread_mutex_t mut;

 public:
  lock_server_cache();
  ~lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
