#include "taskDataStructure.h"
#define BUF_SIZE 10240000

int main( void ){
	initSocket();
	int row = 0;
	int col = 0;

	list<string> l;
	l.push_back("hi");
	l.push_back("hello");
	l.push_back("nemo");
	
	TaskManager tm;
	tm.addQueue( l );
	tm.addTaskToSpecifyQueue(Task(), l);
	tm.showAll();

	HANDLE h = tm.startProcessThread( &tm , run );	

	return 0;
}