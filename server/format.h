#include "taskDataStructure.h"
#include <Winsock2.h>
#include <cstdlib>
#include <iostream>
#pragma comment(lib, "ws2_32.lib") 

int main()
{
	// ����socket��̬���ӿ�(dll)
	WORD wVersionRequested;
	WSADATA wsaData;	// ��ṹ�����ڽ���Wjndows Socket�Ľṹ��Ϣ��
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );	// ����1.1�汾��WinSock��

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		return -1;			// ����ֵΪ���ʱ���Ǳ�ʾ�ɹ�����WSAStartup
	}

	if ( LOBYTE( wsaData.wVersion ) != 2 ||	HIBYTE( wsaData.wVersion ) != 2 ) {
		// ���������ֽ��ǲ���1�����ֽ��ǲ���1��ȷ���Ƿ������������1.1�汾
		// ����Ļ�������WSACleanup()�����Ϣ����������
		WSACleanup( );
		return -1; 
	}

	// ����socket������������ʽ�׽��֣������׽��ֺ�sockSrv
	// SOCKET socket(int af, int type, int protocol);
	// ��һ��������ָ����ַ��(TCP/IPֻ����AF_INET��Ҳ��д��PF_INET)
	// �ڶ�����ѡ���׽��ֵ�����(��ʽ�׽���)�����������ض���ַ�������Э�飨0Ϊ�Զ���
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);	

	// �׽���sockSrv�뱾�ص�ַ����
	// int bind(SOCKET s, const struct sockaddr* name, int namelen);
	// ��һ��������ָ����Ҫ�󶨵��׽��֣�
	// �ڶ���������ָ�����׽��ֵı��ص�ַ��Ϣ���õ�ַ�ṹ�������õ�����Э��Ĳ�ͬ����ͬ
	// ������������ָ��������Э���ַ�ĳ���
	// PS: struct sockaddr{ u_short sa_family; char sa_data[14];};
	//                      sa_familyָ���õ�ַ���壬 sa_data��ռλռ��һ���ڴ������������
	//     ��TCP/IP�У���ʹ��sockaddr_in�ṹ�滻sockaddr���Է�����д��ַ��Ϣ
	// 
	//     struct sockaddr_in{ short sin_family; unsigned short sin_port; struct in_addr sin_addr; char sin_zero[8];};
	//     sin_family��ʾ��ַ�壬����IP��ַ��sin_family��Ա��һֱ��AF_INET��
	//     sin_portָ����Ҫ������׽��ֵĶ˿ڡ�
	//     sin_addr�����׽��ֵ�����IP��ַ��
	//     sin_zero[8]�������������sockaddr_in��sockaddr�ṹ�ĳ���һ����
	//     ��IP��ַָ��ΪINADDR_ANY�������׽������κη�������ػ�����IP��ַ���ͻ�������ݡ�
	//     �����ֻ���׽���ʹ�ö��IP�е�һ����ַ����ָ��ʵ�ʵ�ַ����inet_addr()������
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY); // ��INADDR_ANYת��Ϊ�����ֽ��򣬵��� htonl(long��)��htons(����)
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(6000);

	bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)); // �ڶ�����Ҫǿ������ת��

	// ���׽�������Ϊ����ģʽ���������󣩣� listen()֪ͨTCP������׼���ý�������
	// int listen(SOCKET s,  int backlog);
	// ��һ������ָ����Ҫ���õ��׽��֣��ڶ�������Ϊ���ȴ����Ӷ��е���󳤶ȣ�
	listen(sockSrv, 10);

	// accept()���������ӣ��ȴ��ͻ�������
	// SOCKET accept(  SOCKET s,  struct sockaddr* addr,  int* addrlen);
	// ��һ������������һ�����ڼ���״̬�µ��׽���
	// �ڶ���������sockaddr���ڱ���ͻ��˵�ַ����Ϣ
	// ����������������ָ�������ַ�ĳ���
	// ���ص��������������״̬�µ��׽���ͨ�ŵ��׽���

	// �ͻ������û��˽���ͨ��

	// send(), ���׽����Ϸ�������
	// int send( SOCKET s,  const char* buf,  int len,  int flags);
	// ��һ����������Ҫ������Ϣ���׽��֣�
	// �ڶ�����������������Ҫ�����͵����ݣ�
	// ������������buffer�����ݳ��ȣ�
	// ���ĸ�������һЩ���Ͳ���������

	// recv(), ���׽����Ͻ�������
	// int recv(  SOCKET s,  char* buf,  int len,  int flags);
	// ��һ���������������Ӻ���׽��֣�
	// �ڶ�����������������
	// �������������������ݵĳ��ȣ�
	// ���ĸ�������һЩ���Ͳ���������


	char * buffer = (char *)malloc(sizeof(char) * 10240);
	char * data= (char *)malloc(sizeof(char) * 10240000);
	char * pData = data;

	int getCount = 0;
	int num = 0;
	int len = sizeof(SOCKADDR);
	SOCKADDR_IN  addrClient;

	//while(true){	// ���ϵȴ��ͻ�������ĵ���
	/*SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);

	char recvBuf[1024];
	int num = recv(sockConn, recvBuf, 1024, 0);
	printf("%d\n" , num );
	for( int i = 0 ; i < num ; ++ i ){
	printf("%c ",recvBuf[i]);
	}

	closesocket(sockConn);*/
	//}

	// ==================================================
	/*FILE * fp;
	if( (fp = fopen( "temp.jpg" , "rb" )) == NULL ){
		return 0;
	}
	fseek (fp , 0 , SEEK_END);
	long fileSize = ftell( fp );
	rewind( fp );
	fread( data , (size_t)fileSize , 1 , fp );
	fclose(fp);*/

	SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);//!!!!!!!!
	//printf("%d" , send(sockConn , data , (int )fileSize , 0 )); 

	while( true ){
		while(( num = recv(sockConn, buffer, 10240, 0))>0){
			memcpy( pData , buffer , num );
			getCount += num;
			pData += num;
		}

		printf("%d\n" , getCount );
		for( int i = 0 ; i < getCount ; ++ i ){
			printf("%02x ",(unsigned char)data[i]);
		}
		closesocket(sockConn);
	}
	/*FILE * fp;
	if( (fp = fopen( "temp.jpg" , "wb" )) == NULL ){
		return -2;
	}

	fwrite( data , getCount , 1 ,fp );
	fclose( fp );*/

	free(data);
	free(buffer);

	WSACleanup();
	printf("\n");
	return 0;
}