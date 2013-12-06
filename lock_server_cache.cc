// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


lock_info_cache_s::lock_info_cache_s()
{
	//islocked = false;
	//waitid = false;
	//istrying = false;
	holdid = "";
  isrevoked = false;
	//pthread_mutex_init(&trymut, NULL);
}

lock_info_cache_s::~lock_info_cache_s()
{
	//pthread_mutex_lock(&trymut);
	//pthread_mutex_destroy(&trymut);
}

lock_server_cache::lock_server_cache():
	nacquire(0)
{
	pthread_mutex_init(&mut, NULL);
}

lock_server_cache::~lock_server_cache()
{
	map<lock_protocol::lockid_t, lock_info_cache_s*>::iterator it;
	pthread_mutex_lock(&mut);
	pthread_mutex_destroy(&mut);
	// for(it = locks_s.begin(); it != locks_s.end(); it++)
	// {
	// 	pthread_cond_destroy(&it->second->cond);
	// 	free(it->second);
	// }
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, 
                               int &)
{
  lock_protocol::status ret = lock_protocol::OK;
  pthread_mutex_lock(&mut);
  // handle *h = NULL;
  // rpcc *cl = NULL;
  string cli = "";

  if(0 == locks_s.count(lid))
  {
    printf("\tServerAcquire: new lock_info_cache_s\n");
    locks_s[lid] = new lock_info_cache_s();
  }

  if("" == locks_s[lid]->holdid)
  {
    ret = lock_protocol::OK;
    locks_s[lid]->isrevoked = false;
    locks_s[lid]->holdid = id;
    locks_s[lid]->waitid.erase(id);
  }else{
    ret = lock_protocol::RETRY;
    locks_s[lid]->waitid.insert(id);
    if(false == locks_s[lid]->isrevoked)
    {
      locks_s[lid]->isrevoked = true;
      cli = locks_s[lid]->holdid;
    }
  }

  pthread_mutex_unlock(&mut);

  if("" != cli)
  {
    handle h(cli);//handle h(locks_s[lid]->holdid);
    rpcc*cl = h.safebind();
    if(cl){
     int r;
     cl->call(rlock_protocol::revoke, lid, r);
     printf("\tServerAcquire: revoke\n");
    }
  }



  // if(0 == locks_s.count(lid))
  // {
  // 	printf("\tServerAcquire:new lock_info_cache_s\n");
  // 	locks_s[lid] = new lock_info_cache_s();
  // }
 //  if(false == locks_s[lid]->waitid)
 //  {
 //  	printf("\tServerAcquire\n");
 //  	locks_s[lid]->waitid = true;
	// while(true == locks_s[lid]->islocked)
	// {
	// 	pthread_mutex_unlock(&mut);
	//     handle h(locks_s[lid]->holdid);
	// 	rpcc *cl = h.safebind();
	// 	if(cl){
	// 		int r;
	// 		rlock_protocol::status re = cl->call(rlock_protocol::revoke, lid, r);
	// 	}
	// 	pthread_mutex_lock(&mut);
	// 	pthread_cond_wait(&locks_s[lid]->cond, &mut);
	// }

 //  	while(true == locks_s[lid]->islocked || locks_s[lid]->istrying)
 //  	{
 //  		if(cl && (locks_s[lid]->holdid == cli))
 //  		{
 //  			if(0 == pthread_mutex_trylock(&locks_s[lid]->trymut))	//test whether some thread has been revoking the lock
 //  			{
 //  				printf("\tServerAcquire: revoke lock\n");
 //  				locks_s[lid]->istrying = true;
 //  				pthread_mutex_unlock(&mut);
 //  				int r;
 //  				cl->call(rlock_protocol::revoke, lid, r);
 //  				pthread_mutex_lock(&mut);
 //  				pthread_mutex_unlock(&locks_s[lid]->trymut);
 //  				locks_s[lid]->istrying = false;
 //  				pthread_cond_signal(&locks_s[lid]->cond);
 //  			}else{
 //  				printf("\tServerAcquire: some thread is revoking\n");
 //  				pthread_cond_wait(&locks_s[lid]->cond, &mut);	//some thread is revoking this lock
 //  			}

 //  		}else{
 //  			if(h)
 //  				//free(h);
 //  				delete h;
 //  			cli = locks_s[lid]->holdid;
 //  			h = new handle(cli);
 //  			pthread_mutex_unlock(&mut);
 //  			cl = h->safebind();
 //  			pthread_mutex_lock(&mut);
 //  		}
 //  	}

	// locks_s[lid]->islocked = true;
	// //locks_s[lid]->waitid = false;
	// locks_s[lid]->holdid = id; 	
 //  pthread_mutex_unlock(&mut);

  return ret;
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, 
         int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	pthread_mutex_lock(&mut);

  string retryid = "";
  bool flag = false;

  if(locks_s.count(lid) > 0)
  {
    locks_s[lid]->isrevoked = false;
    locks_s[lid]->holdid = "";
    if(!locks_s[lid]->waitid.empty())
    {
      retryid = *locks_s[lid]->waitid.begin();
      if(locks_s[lid]->waitid.size() > 1)
        flag = true;
      printf("\tServerRelease: call to retry\n");
    }
  }
	// if(locks_s.count(lid) > 0)
	// {
	// 	if(locks_s[lid]->islocked && (locks_s[lid]->holdid == id))
	// 	{
	// 		locks_s[lid]->islocked = false;
	// 		locks_s[lid]->holdid = "";
	// 	}
	// }
	pthread_mutex_unlock(&mut);

  if(retryid != "")
  {
    handle h(retryid);
    rpcc *cl = h.safebind();
    if(cl){
     int r;
     cl->call(rlock_protocol::retry, lid, r);
    }
    if(flag)
    {
      handle h(retryid);
      rpcc *cl = h.safebind();
      if(cl){
       int r;
       cl->call(rlock_protocol::revoke, lid, r);
      }
    }
  }

  return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

