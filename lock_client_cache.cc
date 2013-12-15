// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


int lock_client_cache::last_port = 0;

lock_info_cache_c::lock_info_cache_c()
{
  state = NONE;
  retry = false;
  revokenum = 0;
  isrevoked = false;
  boss = -1;
//  pthread_mutex_init(&acq_lock,NULL);
  pthread_cond_init(&cond,NULL);
}

lock_info_cache_c::~lock_info_cache_c()
{
//  pthread_mutex_lock(&acq_lock);
//  pthread_mutex_destroy(&acq_lock);
  pthread_cond_destroy(&cond);
}


lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  pthread_mutex_init(&mut, NULL);
}

lock_client_cache::~lock_client_cache()
{
  map<lock_protocol::lockid_t, lock_info_cache_c *>::iterator it;
  for(it = locks_c.begin(); it != locks_c.end(); it++)
  {
    delete it->second;
  }
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  int ret = lock_protocol::OK;
  int rp = lock_protocol::OK;
  pthread_mutex_lock(&mut);
  bool flag = false;
  int r;
  if(0 == locks_c.count(lid))               //no information about lid, so create one
  {
    printf("\tClientAcquire:new lock_info_cache_c\n");
    locks_c[lid] = new lock_info_cache_c();
  }

  do{
    switch(locks_c[lid]->state)
    {
      case NONE:
        locks_c[lid]->state = ACQUIRING;
        locks_c[lid]->retry = false;
        pthread_mutex_unlock(&mut);
        rp = cl->call(lock_protocol::acquire, lid, id, r);
        pthread_mutex_lock(&mut);
        if(lock_protocol::OK == rp)
        {
          locks_c[lid]->state = LOCKED;
          locks_c[lid]->boss = pthread_self();
          flag = true;
        }
        break;
      case FREE:
        locks_c[lid]->state = LOCKED;
        locks_c[lid]->boss = pthread_self();
        flag = true;
        break;
      case LOCKED:
        pthread_cond_wait(&locks_c[lid]->cond, &mut);
        break;
      case ACQUIRING:
        if(true == locks_c[lid]->retry)
        {
          locks_c[lid]->state = ACQUIRING;
          locks_c[lid]->retry = false;
          pthread_mutex_unlock(&mut);
          rp = cl->call(lock_protocol::acquire, lid, id, r);
          pthread_mutex_lock(&mut);
          if(lock_protocol::OK == rp)
          {
            locks_c[lid]->state = LOCKED;
            locks_c[lid]->boss = pthread_self();
            flag = true;
          }
        }else{
          pthread_cond_wait(&locks_c[lid]->cond, &mut);
        }
        break;
      case RELEASING:
        locks_c[lid]->state = ACQUIRING;
        locks_c[lid]->retry = false;
        pthread_mutex_unlock(&mut);
        rp = cl->call(lock_protocol::acquire, lid, id, r);
        pthread_mutex_lock(&mut);
        if(lock_protocol::OK == rp)
        {
          locks_c[lid]->state = LOCKED;
          locks_c[lid]->boss = pthread_self();
          flag = true;
        }
    }
  }while(!flag);

  // switch(locks_c[lid]->state){
  //   case NONE:
  //     locks_c[lid]->state = ACQUIRING;
  //     pthread_mutex_unlock(&mut);
  //     printf("\tACQUIRING...\n");
  //     int r;
  //     lock_protocol::status re = cl->call(lock_protocol::acquire, lid, id, r);
  //     VERIFY(lock_protocol::OK == re);
  //     printf("\tClientAcquire:acquired from server\n");
      
  //     pthread_mutex_lock(&mut);
  //     locks_c[lid]->state = LOCKED;
  //     pthread_cond_broadcast(&locks_c[lid]->cond);
  //     break;
  //   case FREE:
  //     locks_c[lid]->state = LOCKED;
  //     locks_c[lid]->boss = pthread_self();
  //     break;
  //   case LOCKED:
  //     if(pthread_self() != locks_c[lid]->boss){
  //       while(LOCKED == locks_c[lid]->state){
  //         pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //       }
  //       pthread_mutex_unlock(&mut);
  //       acquire(lid);
  //       pthread_mutex_lock(&mut);
  //     }
  //     break;
  //   case ACQUIRING:
  //     while(ACQUIRING ==  locks_c[lid]->state){
  //       pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //     }
      
  //     pthread_mutex_unlock(&mut);
  //     acquire(lid);
  //     pthread_mutex_lock(&mut);     
  //     break;
  //   case RELEASING:
  //     while(RELEASING == locks_c[lid]->state){
  //       pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //     }
  //     pthread_mutex_unlock(&mut);
  //     acquire(lid);
  //     pthread_mutex_lock(&mut);         
  // }
  // if(NONE == locks_c[lid]->state)
  // {
  //   locks_c[lid]->state = ACQUIRING;
  //   pthread_mutex_unlock(&mut);
  //   printf("\tACQUIRING...\n");
  //   int r;
  //   lock_protocol::status re = cl->call(lock_protocol::acquire, lid, id, r);
  //   VERIFY(lock_protocol::OK == re);
  //   printf("\tClientAcquire:acquired from server\n");
    
  //   pthread_mutex_lock(&mut);
  //   locks_c[lid]->state = LOCKED;
  //   pthread_cond_broadcast(&locks_c[lid]->cond);
  // }else if(FREE == locks_c[lid]->state){
  //   locks_c[lid]->state = LOCKED;
  //   locks_c[lid]->boss = pthread_self();
  // }else if(LOCKED == locks_c[lid]->state){
  //   if(pthread_self() != locks_c[lid]->boss){
  //     while(LOCKED == locks_c[lid]->state){
  //       pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //     }
  //     pthread_mutex_unlock(&mut);
  //     acquire(lid);
  //     pthread_mutex_lock(&mut);
  //   }
  // }else if(ACQUIRING == locks_c[lid]->state){
  //   while(ACQUIRING ==  locks_c[lid]->state){
  //     pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //   }
    
  //   pthread_mutex_unlock(&mut);
  //   acquire(lid);
  //   pthread_mutex_lock(&mut);
  // }else if(RELEASING == locks_c[lid]->state){
  //   while(RELEASING == locks_c[lid]->state){
  //     pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //   }
  //   pthread_mutex_unlock(&mut);
  //   acquire(lid);
  //   pthread_mutex_lock(&mut);
  // }

  pthread_mutex_unlock(&mut);
  return lock_protocol::OK;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  pthread_mutex_lock(&mut);
  int r;

  if(locks_c.count(lid) > 0)
  {
    if(pthread_self() == locks_c[lid]->boss)
    {
      if(locks_c[lid]->isrevoked)//if(locks_c[lid]->revokenum > 0)
      {
        locks_c[lid]->state = RELEASING;
        locks_c[lid]->revokenum--;
        locks_c[lid]->isrevoked = false;
        pthread_mutex_unlock(&mut);
        lu->dorelease(lid);
        cl->call(lock_protocol::release, lid, id, r);
        pthread_mutex_lock(&mut);
      }else{
        locks_c[lid]->state = FREE;
        pthread_cond_broadcast(&locks_c[lid]->cond);
      }
    }
  }
  pthread_mutex_unlock(&mut);


  // if(locks_c.count(lid) > 0)
  // {
  //   while(FREE != locks_c[lid]->state)
  //   {
  //     if(ACQUIRING == locks_c[lid]->state)
  //     {
  //       while(ACQUIRING == locks_c[lid]->state)
  //       {
  //         pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //       }
  //       if(LOCKED == locks_c[lid]->state)
  //         locks_c[lid]->state = FREE;
  //     }else if(LOCKED == locks_c[lid]->state){
  //       locks_c[lid]->state = FREE;
  //     }
  //   }
  //   // if(ACQUIRING ==  locks_c[lid]->state)
  //   // {
  //   //   while(ACQUIRING == locks_c[lid]->state){
  //   //     pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //   //   }
  //   //   pthread_mutex_unlock(&mut);
  //   //   release(lid);
  //   //   pthread_mutex_lock(&mut);
  //   // }else if(LOCKED == locks_c[lid]->state){
  //   //   locks_c[lid]->state = FREE;
  //   // }else if(RELEASING == locks_c[lid]->state){
  //   // }
  // }
  return lock_protocol::OK;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
  int ret = rlock_protocol::OK;
  int r = 1;
  pthread_mutex_lock(&mut);

  if(locks_c.count(lid) > 0)
  {
    switch(locks_c[lid]->state)
    {
      case FREE:
        locks_c[lid]->state = RELEASING;
        locks_c[lid]->isrevoked = false;
        if(locks_c[lid]->revokenum > 0)
          locks_c[lid]->revokenum--;
        pthread_mutex_unlock(&mut);
        int r;
        lu->dorelease(lid);
        cl->call(lock_protocol::release, lid, id, r);
        pthread_mutex_lock(&mut);
        break;
      default:
        locks_c[lid]->isrevoked = true;
        locks_c[lid]->revokenum++;
    }
  }
  pthread_mutex_unlock(&mut);


  // if(FREE == locks_c[lid]->state)
  // {
  //   locks_c[lid]->state = NONE;
  // }else if(LOCKED == locks_c[lid]->state){
  //   locks_c[lid]->state = RELEASING;
  //   while(RELEASING == locks_c[lid]->state){
  //     pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //   }
  //   locks_c[lid]->state = NONE;
  //   pthread_cond_signal(&locks_c[lid]->cond);
  // }else if(ACQUIRING == locks_c[lid]->state){
  //   while(ACQUIRING ==  locks_c[lid]->state){
  //     pthread_cond_wait(&locks_c[lid]->cond, &mut);
  //   }
  //   pthread_mutex_unlock(&mut);
  //   revoke_handler(lid, r);
  //   pthread_mutex_lock(&mut);
  // }else if(RELEASING == locks_c[lid]->state){
  //   locks_c[lid]->state = NONE;
  //   pthread_cond_broadcast(&locks_c[lid]->cond);
  // }
  return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  int ret = rlock_protocol::OK;

  pthread_mutex_lock(&mut);
  if(locks_c.count(lid) > 0)
  {
    locks_c[lid]->retry = true;
    pthread_cond_broadcast(&locks_c[lid]->cond);
  }
  pthread_mutex_unlock(&mut);

  return ret;
}



