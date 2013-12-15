// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_info::lock_info()
{
	islocked = false;
	clt = -1;
	waitnum = 0;
	pthread_cond_init(&cond, NULL);
}

lock_info::~lock_info()
{
	pthread_cond_destroy(&cond);
}

lock_server::lock_server():
  nacquire (0)
{
	pthread_mutex_init(&mut, NULL);
}

lock_server::~lock_server()
{
	map<lock_protocol::lockid_t, lock_info*>::iterator it;
	pthread_mutex_lock(&mut);//make sure that there is no one hold it.
	pthread_mutex_destroy(&mut);
	for(it = locks.begin(); it != locks.end(); it++)
	{
		pthread_cond_destroy(&it->second->cond);//destroy every condition variable
		free(it->second);
}	}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
	lock_protocol::status ret = lock_protocol::OK;
	printf("\tserver: %d acquire lock %d\n", clt, lid);
	pthread_mutex_lock(&mut);
	map<lock_protocol::lockid_t, lock_info*>::iterator it = locks.find(lid);
	if(it == locks.end())
	{
		lock_info *newlock = new lock_info();//whether it need to be released?
		locks.insert(pair<lock_protocol::lockid_t, lock_info*>(lid, newlock));
	}
	locks[lid]->waitnum++;
	while(locks[lid]->islocked == true)
	{
		pthread_cond_wait(&locks[lid]->cond, &mut);
	}
	locks[lid]->waitnum--;
	locks[lid]->islocked = true;
	locks[lid]->clt = clt;
	printf("\tserver: %d acquired lock\n", clt);
	pthread_mutex_unlock(&mut);
	return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
	lock_protocol::status ret = lock_protocol::OK;
	printf("\tserver: %d release lock %d\n", clt, lid);
	pthread_mutex_lock(&mut);
	map<lock_protocol::lockid_t, lock_info*>::iterator it = locks.find(lid);
	if(it != locks.end())
	{
		if(locks[lid]->islocked && (locks[lid]->clt == clt))
		{
			locks[lid]->islocked = false;
			locks[lid]->clt = -1;
			if(locks[lid]->waitnum > 0)
				pthread_cond_signal(&locks[lid]->cond);
		}
	}
	printf("\tserver: %d released lock\n", clt);
	pthread_mutex_unlock(&mut);
	return ret;
}