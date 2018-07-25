//libraries
#include "threadpool.h"
#include <stdio.h>

//-----------------------------Defines---------------------------------//
#ifndef threadpool_c
#define threadpool_c

//---------------------Declaration of functions------------------------//
void error(char *err,threadpool *pool,int flag);
void enqueue(threadpool* pool,work_t *newJob);
work_t *Dequeue(threadpool *pool);

//======================================================================================FUNCTIONS==============================================================================================//

//----------------------------------------------------create_threadpool------------------------------------------------------//
/**
 * create_threadpool creates a fixed-sized thread
 * pool.  If the function succeeds, it returns a (non-NULL)
 * "threadpool", else it returns NULL.
 * this function should:
 * 1. input sanity check 
 * 2. initialize the threadpool structure
 * 3. initialized mutex and conditional variables
 * 4. create the threads, the thread init function is do_work and its argument is the initialized threadpool. 
 */
threadpool* create_threadpool(int num_threads_in_pool)
{
	if(num_threads_in_pool>MAXT_IN_POOL || num_threads_in_pool<1)//check input
		printf("Usage: threadpool <pool-size> <max-number-of-jobs>\n");
	threadpool *pool = (threadpool*)calloc(1,sizeof(threadpool));
	if(pool==NULL)
	{
		perror("calloc");
		exit(1);
	}

	pool->num_threads=num_threads_in_pool;
	pool->threads=(pthread_t*)calloc(num_threads_in_pool,sizeof(pthread_t));
	if(pool->threads==NULL)
		error("calloc",pool,-1);

	pool->qsize=0;
	pool->qhead=pool->qtail=NULL;
	pool->shutdown=pool->dont_accept=0;
	if(pthread_mutex_init(&(pool->qlock), NULL)!=0)
		error("pthread_mutex_init",pool,0);
	if(pthread_cond_init(&(pool->q_not_empty), NULL)!=0)
		error("pthread_cond_init",pool,0);
	if(pthread_cond_init(&(pool->q_empty), NULL)!=0)
		error("pthread_cond_init",pool,0);
	
	int i;
	for(i=0;i<num_threads_in_pool;i++)//creating threads
	{
		if(pthread_create(&(pool->threads[i]),NULL,do_work,(void*)pool)!=0)
			error("pthread_create",pool,1);
	}

return pool;
}

//-----------------------------do_work-------------------------------------//
/**
 * The work function of the thread
 * this function should:
 * 1. lock mutex
 * 2. if the queue is empty, wait
 * 3. take the first element from the queue (work_t)
 * 4. unlock mutex
 * 5. call the thread routine
 *
 */
void* do_work(void* p)
{
	threadpool *pool=(threadpool*)p; 
	work_t *job;
	while(1)
	{
		if(pool->shutdown==1)
			return NULL;
		if(pthread_mutex_lock(&(pool->qlock))!=0)
			error("pthread_mutex_lock",pool,1);
		while(pool->qsize==0)
		{
			if(pthread_cond_wait(&(pool->q_not_empty),&(pool->qlock))!=0)
				error("pthread_cond_wait",pool,1);
			if(pool->shutdown==1)
			{
				if(pthread_mutex_unlock(&(pool->qlock))!=0)
					error("pthread_mutex_unlock",pool,1);
				return NULL;
			}
		}
		job=Dequeue(pool);
		
		if(pool->qsize==0 && pool->dont_accept==1)
		{
			if(pthread_cond_signal(&(pool->q_empty))!=0)
				error("pthread_cond_signal",pool,1);
		}
		
		if(pthread_mutex_unlock(&(pool->qlock))!=0)
			error("pthread_mutex_unlock",pool,1);
		if(job->routine(job->arg)==-1){free(job);return NULL;}
		free(job);
		job=NULL;
	}
return NULL;
}

//-----------------------------dispatch---------------------------------//
/**
 * dispatch enter a "job" of type work_t into the queue.
 * when an available thread takes a job from the queue, it will
 * call the function "dispatch_to_here" with argument "arg".
 * this function should:
 * 1. create and init work_t element
 * 2. lock the mutex
 * 3. add the work_t element to the queue
 * 4. unlock mutex
 *
 */
void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg)
{
	if(from_me==NULL||from_me->dont_accept==1)
		return;
	work_t *job=(work_t *)calloc(1,sizeof(work_t));
	if(job==NULL)
		error("calloc",from_me,1);
	job->routine = dispatch_to_here;
	job->arg=arg;
	job->next=NULL;
	
	if(pthread_mutex_lock(&(from_me->qlock))!=0)
		error("pthread_mutex_lock",from_me,1);
	enqueue(from_me,job);
	from_me->qsize++;
	if(pthread_mutex_unlock(&(from_me->qlock))!=0)
		error("pthread_mutex_unlock",from_me,1);
}

//-----------------------------destroy_threadpool---------------------------------//
/**
 * destroy_threadpool kills the threadpool, causing
 * all threads in it to commit suicide, and then
 * frees all the memory associated with the threadpool.
 */
void destroy_threadpool(threadpool* destroyme)
{
	if(destroyme==NULL)
		return;
	if(pthread_mutex_lock(&(destroyme->qlock))!=0)
		error("pthread_mutex_unlock",destroyme,1);
	destroyme->dont_accept=1;
	while(destroyme->qsize!=0)
	{
		if(pthread_cond_wait(&(destroyme->q_empty),&(destroyme->qlock))!=0)
			error("pthread_cond_wait",destroyme,1);
	}
	destroyme->shutdown=1;
	if(pthread_cond_broadcast(&(destroyme->q_not_empty))!=0)
		error("pthread_cond_broadcast",destroyme,1);
	if(pthread_mutex_unlock(&(destroyme->qlock))!=0)
		error("pthread_mutex_unlock",destroyme,1);
	int i;
	for(i=0;i<destroyme->num_threads;i++)
	{
		if(pthread_cond_broadcast(&(destroyme->q_not_empty))!=0)
			error("pthread_cond_broadcast",destroyme,1);
		if(pthread_join(destroyme->threads[i],NULL)!=0)
			error("pthread_join",destroyme,1);
	}
	
	if(pthread_cond_destroy(&(destroyme->q_not_empty))!=0)
		error("pthread_cond_destroy",destroyme,1);
	if(pthread_cond_destroy(&(destroyme->q_empty))!=0)
		error("pthread_cond_destroy",destroyme,1);
	if(pthread_mutex_destroy(&(destroyme->qlock))!=0)
		error("pthread_mutex_destroy",destroyme,1);
	free(destroyme->threads);
	free(destroyme);
}

//-----------------------------Dequeue---------------------------------//
//this method remove the first element from the queue
work_t *Dequeue(threadpool *pool)
{
	work_t *temp=pool->qhead;
	pool->qhead=(pool->qhead)->next;
	pool->qsize--;
	if(pool->qsize==0)
	{
		pool->qhead=NULL;
		pool->qtail=NULL;
	}
	return temp;
}

//-----------------------------enqueue---------------------------------//
void enqueue(threadpool* pool,work_t *newJob)//this method enqueue jobs to an linked list.
{
	if(pool->qsize==0)
	{	
		pool->qhead=pool->qtail=newJob;
		if(pthread_cond_signal(&(pool->q_not_empty))!=0)
			error("pthread_cond_signal",pool,1);
	}
	else
	{
		pool->qtail->next=newJob;
		pool->qtail=pool->qtail->next;
		pool->qtail->next=NULL;	
	}
}

//-----------------------------error---------------------------------------//
void error(char *err,threadpool *pool,int flag)//taking care of all the errors
{
	perror(err);
	switch(flag)
	{
		case 0:
			free(pool->threads);
			break;
		case 1:
			free(pool->threads);
			work_t *temp;
			while((temp=pool->qhead)!=NULL)
			{
				pool->qhead=pool->qhead->next;
				free(temp);
			}
			break;
	}
	free(pool);
	exit(1);
}

#endif
