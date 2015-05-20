#ifndef TASK_H
#define TASK_H

#ifndef TDS_HEAD
#define TDS_HEAD

#include <list>
#include <utility>
#include <array>
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
using std::array;
using std::vector;
using std::string;
using std::list;
using std::iterator;
using std::cout;
using std::endl;

#define tq_vec vector< pair<TaskQueue * , CRITICAL_SECTION > >

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

	CRITICAL_SECTION lock;
	vector< pair<TaskQueue * , CRITICAL_SECTION > > * taskQueueTable;
	list< int > indexList;
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

	bool EnterSpecifyQueueCS( const int & row , const int & col ){
		if ((row >= 0 && row <maxQueueNum) && (col >= 0 && col < taskQueueTable[row].size())){
			EnterCriticalSection(&taskQueueTable[row][col].second);
			return true;
		}
		return false;
	}

	bool LeaveSpecifyQueueCS( const int & row , const int & col ){
		if ((row >= 0 && row < maxQueueNum) && (col >= 0 && col < taskQueueTable[row].size())){
			LeaveCriticalSection(&taskQueueTable[row][col].second);
			return true;
		}
		return false;
	}

	bool getSpecifyQueue( const int & row , const int & col , TaskQueue **pQueue ){
		if ((row >= 0 && row < maxQueueNum) && (col >= 0 && col < taskQueueTable[row].size())){
			*pQueue = taskQueueTable[row][col].first;
			return true;
		}
		return false;
	}

	bool popSpecifyQueueTask(const int & row, const int & col , Task & task){
		if ((row >= 0 && row < maxQueueNum) && (col >= 0 && col < taskQueueTable[row].size())){
			return taskQueueTable[row][col].first->pop_front(&task);
		}
		return false;
	}

	void EnterTaskManagerCS( void ){
		EnterCriticalSection( &(this->lock) );
	}

	void LeaveTaskManagerCS( void ){
		LeaveCriticalSection( &(this->lock) );
	}

public:
	TaskManager( int maxNum = 512){
		curQueueNum = 0;
		maxQueueNum = (maxNum>512)?maxNum:512;
		threadStatus = false;

		if( InitializeCriticalSectionAndSpinCount( &lock , 4096) != TRUE){
			Log("[TaskManager]:create global cs failed.");
			exit(EXIT_FAILURE);
		}

		taskQueueTable = new vector< pair< TaskQueue * , CRITICAL_SECTION> >[maxNum];
	}

	//need test
	~TaskManager(){
		int temp;
		list<int>::iterator it = indexList.begin();
		list<int>::iterator end = indexList.end();
		DeleteCriticalSection(&lock);

		while (it != end){
			temp = taskQueueTable[*it].size();
			for (int i = 0; i < temp; ++ i){
				DeleteCriticalSection(&taskQueueTable[*it][i].second);
				delete taskQueueTable[*it][i].first;
			}
			++it;
		}
	}

	//need test
	list<int> getIndexList(void){
		return indexList;
	}

	int hash( list<string> & keys ){
		list<string>::iterator it = keys.begin();
		list<string>::iterator end = keys.end();
		const char * str = NULL;
		int sum = 0;
		int limit = 0;
		int i = 0;

		while( it != end ){
			limit = it->length();
			str = it->c_str();
			for( i = 0 ; i < limit ; i += 32 ){
				sum += str[i];
			}
			++ it;
		}

		return sum%maxQueueNum;
	}

	bool findByKeys( list<string> & keys , int * row , int * col){
		int pos = hash( keys );
		int limit = taskQueueTable[pos].size();

		EnterCriticalSection( &lock );
		for (int i = 0; i < limit; ++i){
			EnterCriticalSection(&taskQueueTable[pos][i].second);
			if (taskQueueTable[pos][i].first->getKey() == keys){
				*row = pos;
				*col = i;
				LeaveCriticalSection(&taskQueueTable[pos][i].second);
				LeaveCriticalSection(&lock);
				return true;
			}
			LeaveCriticalSection(&taskQueueTable[pos][i].second);
		}
		LeaveCriticalSection( &lock );
		return false;
	}

	bool addTaskToSpecifyQueue( Task & task , list<string> & keys ){
		int index = hash(keys);
		int limit = keys.size();
		bool ret = false;

		EnterCriticalSection( &lock );
		if( index >= 0 && index < maxQueueNum ){
			for (int i = 0; i < limit; ++i){
				if (taskQueueTable[index][i].first->getKey() == keys){
					EnterCriticalSection(&taskQueueTable[index][i].second);
					taskQueueTable[index][i].first->push_back(task);
					LeaveCriticalSection(&taskQueueTable[index][i].second);
					ret = true;
					break;
				}
			}
			LeaveCriticalSection( &lock );
			return ret;
		}
		LeaveCriticalSection( &lock );
		return false;
	}

	//need test
	bool addTask( const int & row , const int & col , Task & task ){
		bool ret = false;
		if ((row >= 0 && row <= taskQueueTable->size()) && (col > 0 && col < taskQueueTable[row].size())){
			EnterCriticalSection(&taskQueueTable[row][col].second);
			ret = taskQueueTable[row][col].first->push_back(task);
			LeaveCriticalSection(&taskQueueTable[row][col].second);
		}
		return ret;
	}

	//need test
	bool addQueue(const int & row, list<string> * pList = NULL){
		EnterCriticalSection(&lock);
		if (row >= 0 && row < maxQueueNum){
			TaskQueue * queue = new TaskQueue(*pList);
			CRITICAL_SECTION cs;
			if (InitializeCriticalSectionAndSpinCount(&cs, 4096)){
				taskQueueTable[row].push_back(pair< TaskQueue *, CRITICAL_SECTION>(queue, cs));
				indexList.push_back(row);
				indexList.unique();
				++curQueueNum;
				LeaveCriticalSection(&lock);
				return true;
			}
			else{
				LeaveCriticalSection(&lock);
				return false;
			}
		}
		else{
			LeaveCriticalSection(&lock);
			return false;
		}
	}

	bool addQueue( list<string> & keys ){
		int pos = hash( keys );
		
		EnterCriticalSection( &lock );
		if( curQueueNum == maxQueueNum ){
			LeaveCriticalSection( &lock );
			return false;
		}
		
		TaskQueue * queue = new TaskQueue(keys);
		CRITICAL_SECTION cs;
		if( InitializeCriticalSectionAndSpinCount( &cs , 4096 ) ){
			taskQueueTable[pos].push_back( pair< TaskQueue * , CRITICAL_SECTION>( queue , cs) );
			indexList.push_back( pos );
			indexList.unique();
			++ curQueueNum;
		}
		
		LeaveCriticalSection( &lock );
		return true;
	}

	bool removeQueue( list< string > & keys ){
		int pos = hash( keys );
		int dest = -1;
		EnterCriticalSection( &lock );
		
		if( curQueueNum == 0 ){
			LeaveCriticalSection( &lock );
			return false;
		}

		int limit = taskQueueTable[pos].size();
		for( int i = 0 ; i < limit ; ++ i ){
			if( taskQueueTable[pos][i].first->getKey() == keys ){
				dest = i;
				break;
			}
		}

		list<int>::iterator it = indexList.begin();
		list<int>::iterator end = indexList.end();
		if( dest != -1 ){
			taskQueueTable[pos].erase( taskQueueTable[pos].begin() + dest );
			--curQueueNum;
			if( limit - 1 == 0 ){
				while( it != end ){
					if( * it == pos ){
						break;
					}
					++ it;
				}
				indexList.erase( it );
			}
		}

		LeaveCriticalSection( &lock );
		return true;
	}

	void showAll(void){
		int temp;
		list<int>::iterator it = indexList.begin();
		list<int>::iterator end = indexList.end();

		Log("show all:size %d\n" , curQueueNum);
		while (it != end){
			temp = taskQueueTable[*it].size();
			Log("%d: ", *it);
			for (int i = 0; i < temp; ++i){
				Log("size:%d\n", taskQueueTable[*it][i].first->size());
			}
			++it;
		}
	}

public:
	static HANDLE startProcessThread( TaskManager * pTM ,void (*run)(TaskManager * pTM , int row ,int col , Task & task)){
		if( pTM == NULL || run == NULL ){
			return INVALID_HANDLE_VALUE;
		}

		st_processTasks * pst_pt = new st_processTasks;
		HANDLE h = INVALID_HANDLE_VALUE;
		pst_pt->pTM = pTM;
		pst_pt->run = run;

		if( pTM->threadStatus == false ){
			h = (HANDLE)_beginthreadex( NULL , 0 , processTasks , pst_pt , 0 , NULL );
			pTM->threadStatus = true;
			Sleep(500);
			//WaitForSingleObject(h, INFINITE);
		}

		return h;
	}

private:
	struct st_processTasks{
		TaskManager * pTM;
		void ( *run)( TaskManager* pTM , int row ,int col , Task & task);
	};

	struct st_taskThread{
		st_processTasks * st_pt;
		Task task;
		int index;
		int col;
	};

	static unsigned int __stdcall taskThread( LPVOID lpArg ){
		st_processTasks * pst_pt = ((st_taskThread *)lpArg)->st_pt;
		Task task = ((st_taskThread *)lpArg)->task;
		int index = ((st_taskThread *)lpArg)->index;
		int col = ((st_taskThread *)lpArg)->col;
		delete ((st_taskThread *)lpArg);

		if( task.data ){
			pst_pt->run(pst_pt->pTM ,index ,col ,task);
		}
		pst_pt->run(pst_pt->pTM ,index ,col ,task);

		return 0;
	}

	static unsigned int __stdcall processTasks( LPVOID lpArg ){
		st_processTasks st_pt = *(( st_processTasks * )lpArg);
		st_taskThread * pst_tt = NULL;
		TaskManager * pTM = st_pt.pTM;

		int limit = 0;
		int taskNum = 0;
		int index = 0;
		list<int>::iterator it;
		list<int>::iterator end;

		for ( ; ; ){
			it = pTM->indexList.begin();
			end = pTM->indexList.end();
			st_pt.pTM->EnterTaskManagerCS();
			while (it != end){
				index = *it;
				limit = pTM->taskQueueTable[index].size();
				for (int i = 0; i < limit; ++i){
					taskNum = pTM->taskQueueTable[index][i].first->size();
					for (int d = 0; d < taskNum; ++d){
						pst_tt = new st_taskThread;
						pTM->EnterSpecifyQueueCS(index, i);
						pst_tt->st_pt = &st_pt;
						pst_tt->index = index;
						pst_tt->col = i;
						pTM->popSpecifyQueueTask(index, i, pst_tt->task);
						pTM->LeaveSpecifyQueueCS(index, i);
						CloseHandle((HANDLE)_beginthreadex(NULL, 0, taskThread, pst_tt, 0, NULL));
					}
					
				}
				++it;
			}
			st_pt.pTM->LeaveTaskManagerCS();
			Sleep(0);
		}

		delete [] pst_tt;
		return 0;
	}

};

//************************************************************************************************

//初始化socket
void initSocket(void){
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		exit(EXIT_FAILURE);
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2){
		WSACleanup();
		exit(EXIT_FAILURE);
	}
}

struct arg_st{
	TaskManager * pTM;
	char * data;
	int len;
	string ip;
};

unsigned int __stdcall taskThread(LPVOID lpArg){
	TaskManager * pTM = ((arg_st*)lpArg)->pTM;
	char * data = ((arg_st*)lpArg)->data;
	int length = ((arg_st*)lpArg)->len;
	string ip = ((arg_st*)lpArg)->ip;
	delete lpArg;

	

	return 0;
}

void run(TaskManager * pTm, int index, int col, Task & task){
	printf("run\n");
	pTm->showAll();
}

//参数是一个指向TaskManager的指针
unsigned int __stdcall listenThread(LPVOID lpArg){
#define BUF_SIZE 1024000
	SOCKET sockSrv;	//服务器SOCKET
	SOCKET sockClient;
	SOCKADDR_IN addrSrv;
	SOCKADDR_IN addrClient;
	string ip;

	int len = sizeof(SOCKADDR);
	int count = 0;
	int num = 0;
	int limit = BUF_SIZE * 4;
	char * data = NULL;
	char * pData = data;
	char * buffer = NULL;
	bool flag = false;
	buffer = new char[BUF_SIZE];

	sockSrv = socket(AF_INET, SOCK_STREAM, 0);
	// 将INADDR_ANY转换为网络字节序，调用 htonl(long型)或htons(整型)
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(6000);

	int opt = 1;
	//初始化服务器socket
	setsockopt(sockSrv, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	//绑定服务器端口
	bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	//准备监听
	listen(sockSrv, 512);

	while (true){
		//获取连接的客户端socket
		sockClient = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
		if (sockClient != INVALID_SOCKET){
			num = 0;
			count = 0;
			data = new char[limit];
			pData = data;
			flag = false;

			//接受数据
			while ((num = recv(sockClient, buffer, limit, 0)) > 0){
				count += num;
				if (count < limit){
					memcpy(pData, buffer, num);
					pData += num;
				}
				else{
					flag = true;
					break;
				}
				num = 0;
			}

			if (flag == false){
				arg_st * pArg_st = new arg_st;
				pArg_st->data = data;
				pArg_st->len = count;
				pArg_st->ip = string(inet_ntoa(addrClient.sin_addr));
				pArg_st->pTM = (TaskManager*)lpArg;
				CloseHandle((HANDLE)_beginthreadex(NULL,0,taskThread,pArg_st,0,NULL));
			}
			
			closesocket(sockClient);
		}
	}

	free(buffer);
	return 0;
}

#endif // !TASK_H
