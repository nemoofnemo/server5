#ifndef SERVER_H_
#define SERVER_H_

#ifndef TDS_HEAD
#define TDS_HEAD

#include <list>
#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <process.h>
#include <Winsock2.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib") 

#define Log(format,...) fprintf( stdout , (format) , __VA_ARGS__)
#define SRV_ERROR -1
#define SRV_NOT_FOUND -1

using std::pair;
using std::vector;
using std::string;
using std::list;
using std::iterator;
using std::cout;
using std::endl;
#endif

class Task{
public:
	void * data;
	int length;
	int status;
public:
	Task( void * src = NULL , int length = 0 , int stat = 0){
		this->data = src;
		this->length = length;
		status = stat;
	}
};

class TaskQueue{
private:
	struct m_queue {
		int size;
		int maxSize;
		int head;
		int tail;
		Task * taskArr;
	};

	m_queue queue;
	list<string> keys;

private:
	//not available
	TaskQueue( const TaskQueue & tq ){
		
	}

	//not available
	TaskQueue & operator= ( const TaskQueue & tq ){
		
		return (*this);
	}

public:
	TaskQueue(list<string> & keyList, int maxNum = 512 ){
		queue.size = 0;
		queue.maxSize = (maxNum<512)?512:maxNum;
		queue.head = 0;
		queue.tail = 0;
		queue.taskArr = new Task[maxNum];
		keys = keyList;
	}

	~TaskQueue(){
		int index = queue.head;
		for( int i = 0 ; i < queue.size ; ++ i ){
			index %= queue.maxSize;
			if( queue.taskArr[index].data )
				delete [] queue.taskArr[index].data;
			++ index;
		}

		delete [] queue.taskArr;
		keys.clear();
	}

	//warning
	void deleteAllTasks( void ){
		int index = queue.head;
		for( int i = 0 ; i < queue.size ; ++ i ){
			index %= queue.maxSize;
			if( queue.taskArr[index].data )
				delete [] queue.taskArr[index].data;
			queue.taskArr[index].data = NULL;
			queue.taskArr[index].length = 0;
			queue.taskArr[index].status = 0;
			++ index;
		}

		queue.head = 0;
		queue.tail = 0;
		queue.size = 0;
	}

	bool push_back( Task & arg ){
		if( queue.size == queue.maxSize ){
			Log("queue full\n");
			return false;
		}
		else{
			queue.taskArr[queue.tail] = arg;
			++ queue.tail;
			queue.tail %= queue.maxSize;
			++ queue.size;
			return true;
		}
	}

	bool push_back( void * pData , int length , int status = 0 ){
		Task arg;
		if( queue.size == queue.maxSize ){
			Log("queue full\n");
			return false;
		}
		else{
			arg.data = pData;
			arg.length = length;
			arg.status = status;

			queue.taskArr[queue.tail] = arg;
			++ queue.tail;
			queue.tail %= queue.maxSize;
			++ queue.size;
			return true;
		}
	}

	bool pop_front( void ){
		if( queue.size == 0 ){
			Log( "queue empty\n");
			return false;
		}
		else{
			queue.taskArr[queue.head].data = NULL;
			queue.taskArr[queue.head].length = 0;
			++ queue.head;
			queue.head %= queue.maxSize;
			-- queue.size;
			return true;
		}
	}

	bool pop_front( Task * pTask ){
		if( queue.size == 0 ){
			Log( "queue empty\n");
			pTask->data = NULL;
			pTask->length = 0;
			pTask->status = 0;
			return false;
		}
		else{
			( *pTask ) = queue.taskArr[queue.head];
			queue.taskArr[queue.head].data = NULL;
			queue.taskArr[queue.head].length = 0;
			++ queue.head;
			queue.head %= queue.maxSize;
			-- queue.size;
			return true;
		}
	}

	bool getFirstTask( Task * pTask ){
		if( queue.size != 0 ){
			(*pTask) = queue.taskArr[queue.head];
			return true;
		}
		else{
			return false;
		}
	}

	list<string> getKey( void ){
		return this->keys;
	}

	void setKeys( const list<string> & keys){
		this->keys = keys;
	}

	int isKeyExist( const string & key ){
		int limit = keys.size();
		list<string>::iterator it = keys.begin();
		for( int i = 0 ; i < limit ; ++ i , ++ it){
			if( string(it->c_str()) == key ){
				return i;
			}
		}
		return SRV_NOT_FOUND;
	}

	int size( void ){
		return this->queue.size;
	}

	void show( void ){
		int index = queue.head;
		for( int i = 0 ; i < queue.size ; ++ i ){
			index %= queue.maxSize;
			Log( "%3d:0x%p %d\n" , i , queue.taskArr[index].data , queue.taskArr[index].length );
			++ index;
		}
	}
};

class TaskManager{
private:
	int curQueueNum;
	int maxQueueNum;
	bool threadStatus;

	vector< pair<TaskQueue * , CRITICAL_SECTION > > taskQueues;
	CRITICAL_SECTION lock;
private:
	//not available
	TaskManager( const TaskManager & arg ){
		//.
	}

	//not available
	TaskManager & operator = ( const TaskManager & ){
		//..
		return (*this);
	}

	//need test
	int removeQueue(string key){
		EnterCriticalSection( &lock );
		if( curQueueNum == 0 ){
			LeaveCriticalSection( &lock );
			return SRV_NOT_FOUND;
		}

		int ret = findByKey( key );
		if( ret != -1 ){
			delete ( taskQueues[ret].first );
			DeleteCriticalSection( &taskQueues[ret].second );
			taskQueues.erase( taskQueues.begin() + ret );
			-- curQueueNum;
		}

		LeaveCriticalSection( &lock );
		return ret;
	}

	void EnterSpecifyQueueCS( int index ){
		if( index >= 0 && index < curQueueNum ){
			EnterCriticalSection(&taskQueues[index].second);
		}
	}

	void LeaveSpecifyQueueCS( int index ){
		if( index >= 0 && index < curQueueNum ){
			LeaveCriticalSection(&taskQueues[index].second);
		}
	}

	bool getSpecifyQueue( int index , TaskQueue ** pQueue ){
		if( index > 0 && index < curQueueNum ){
			(*pQueue) = (taskQueues[index].first);
			return true;
		}else{
			(*pQueue ) = NULL;
			return false;
		}
	}

	bool popSpecifyQueueTask( int index , Task & task ){
		if( index >= 0 && index < curQueueNum ){
			return (taskQueues[index].first->pop_front(&task));
		}else{
			return false;
		}
	}

	void EnterTaskManagerCS( void ){
		EnterCriticalSection( &(this->lock) );
	}

	void LeaveTaskManagerCS( void ){
		LeaveCriticalSection( &(this->lock) );
	}

public:
	TaskManager( int maxNum = 32){
		curQueueNum = 0;
		maxQueueNum = maxNum;
		threadStatus = false;

		if( InitializeCriticalSectionAndSpinCount( &lock , 4096) != TRUE){
			puts("[taskManager]:create global cs failed.");
			exit(EXIT_FAILURE);
		}
	}

	~TaskManager(){
		DeleteCriticalSection( &lock );
		int limit = taskQueues.size();
		for( int i = 0 ; i < limit ; ++ i ){
			DeleteCriticalSection( &taskQueues[i].second );
			delete taskQueues[i].first;
		}
		taskQueues.clear();
	}

	int findByKey( string key ){
		EnterCriticalSection( &lock );
		int ret = SRV_NOT_FOUND;
		for( int i = 0 ; i < curQueueNum ; ++ i ){
			if( taskQueues[i].first->isKeyExist(key) ){
				ret = i;
				break;
			}
		}
		LeaveCriticalSection( &lock );
		return ret;	//cannot find -1 (SRV_NOT_FOUND).
	}

	bool addTaskToSpecifyQueue( Task & task , int index ){
		EnterCriticalSection( &lock );
		bool ret = false;
		if( index >= 0 && index < curQueueNum ){
			ret = (taskQueues[index].first)->push_back( task );
			LeaveCriticalSection( &lock );
			return ret;
		}
		LeaveCriticalSection( &lock );
		return false;
	}

	bool addQueue( list<string> & keys ){
		int i = 0;
		int count = -1;
		
		EnterCriticalSection( &lock );
		if( curQueueNum == maxQueueNum ){
			LeaveCriticalSection( &lock );
			return false;
		}
		
		TaskQueue * queue = new TaskQueue(keys);
		CRITICAL_SECTION cs;
		InitializeCriticalSectionAndSpinCount( &cs , 4096 );
		taskQueues.push_back( pair< TaskQueue * , CRITICAL_SECTION>( queue , cs) );
		++ curQueueNum;
		LeaveCriticalSection( &lock );
		return true;
	}

	bool removeQueue( int index ){
		EnterCriticalSection( &lock );
		if( curQueueNum == 0 ){
			LeaveCriticalSection( &lock );
			return false;
		}

		if( index < 0 || index >= curQueueNum ){
			LeaveCriticalSection( &lock );
			return false;
		}

		delete taskQueues[index].first;
		DeleteCriticalSection( &taskQueues[index].second );
		taskQueues.erase( taskQueues.begin() + index );
		-- curQueueNum;
		LeaveCriticalSection( &lock );
		return true;
	}

public:
	static HANDLE startProcessThread( TaskManager * pTM ,void (*run)(TaskManager * pTM , int index , Task & task)){
		if( pTM == NULL || run == NULL ){
			return INVALID_HANDLE_VALUE;
		}

		st_processTasks * pst_pt = new st_processTasks;
		HANDLE h = INVALID_HANDLE_VALUE;
		pst_pt->pTM = pTM;
		pst_pt->run = run;

		if( pTM->threadStatus == false ){
			h = (HANDLE)_beginthreadex( NULL , 0 , processTasks , pst_pt , 0 , NULL );
			CloseHandle( h );
			pTM->threadStatus = true;
		}

		return h;
	}

private:
	struct st_processTasks{
		TaskManager * pTM;
		void ( *run)( TaskManager* pTM , int index , Task & task);
	};

	struct st_taskThread{
		st_processTasks * st_pt;
		Task task;
		int index;
	};

	static unsigned int __stdcall taskThread( LPVOID lpArg ){
		st_processTasks * pst_pt = ((st_taskThread *)lpArg)->st_pt;
		Task task = ((st_taskThread *)lpArg)->task;
		int index = ((st_taskThread *)lpArg)->index;
		delete ((st_taskThread *)lpArg);

		if( task.data ){
			pst_pt->run( pst_pt->pTM , index , task );
		}
		pst_pt->run( pst_pt->pTM , index , task );

		return 0;
	}

	static unsigned int __stdcall processTasks( LPVOID lpArg ){
		st_processTasks st_pt = *(( st_processTasks * )lpArg);
		st_taskThread * pst_tt = NULL;

		while( true ){
			st_pt.pTM->EnterTaskManagerCS();
			for( int i = 0 ; i < st_pt.pTM->curQueueNum ; ++ i ){
				while( (((st_pt.pTM)->taskQueues[i]).first)->size() ){
					pst_tt = new st_taskThread;//release in task thread
					pst_tt->st_pt = &st_pt;
					st_pt.pTM->EnterSpecifyQueueCS( i );

					if( st_pt.pTM->popSpecifyQueueTask( i , pst_tt->task ) == true ){
						pst_tt->st_pt->pTM = st_pt.pTM;
						pst_tt->st_pt->run = st_pt.run;
						CloseHandle((HANDLE)_beginthreadex(NULL, 0, taskThread, pst_tt , 0, NULL));
					}
					else{
						st_pt.pTM->LeaveSpecifyQueueCS( i );
						break;
					}
					st_pt.pTM->LeaveSpecifyQueueCS( i );
				}
			}

			st_pt.pTM->LeaveTaskManagerCS();
			//system("pause");
			Sleep(500);
		}

		delete [] pst_tt;
		return 0;
	}

};

#endif // !SERVER_H_
