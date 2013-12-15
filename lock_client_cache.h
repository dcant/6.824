// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"

#define NONE 0
#define FREE 1
#define LOCKED 2
#define ACQUIRING 3
#define RELEASING 4

using namespace std;


// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};

class lock_info_cache_c {
public:
  int state;
  pthread_t boss;
  bool retry;
  bool isrevoked;
  int revokenum;
//  pthread_mutex_t acq_lock;                  //protect state when acquiring lock
  pthread_cond_t cond;
  lock_info_cache_c();
  ~lock_info_cache_c();
};


class lock_client_cache : public lock_client {
 private:
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;

  map<lock_protocol::lockid_t, lock_info_cache_c *> locks_c;
  pthread_mutex_t mut;

 public:
  static int last_port;
  lock_client_cache(std::string xdst, class lock_release_user *l = 0);
  virtual ~lock_client_cache();
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, 
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, 
                                       int &);
};


#endif
