#ifndef byteArr_h
#define byteArr_h
#include <cstdlib>

class byteArr {
public:
	char * data;
	int size;
public:
	byteArr( char * data = NULL , int size = 0 ){
		this->data = data;
		this->size = size;
	}
};
#endif // !byteArr_h
