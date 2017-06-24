#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

using namespace std;

#define DEFAULT_PORT 1221
#define BUFFER_SIZE 1024

typedef struct
{
	string method;
	string uri;
	string version;
	string host;
} httpRequest;

void httpHeaderParse(char*, int, httpRequest&); 
int writeMessage(int, char*);

int readVideoAndSend(int connectFd, string, struct stat sbuf);

int main(int argc, char** argv)
{
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd < 0)
	{
		printf("Socket error!\r\n");
		exit(-1);
	}

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(DEFAULT_PORT);
	if(bind(listenFd, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0)
	{
		printf("Bind error!\r\n");
		exit(-1);
	}
	if(listen(listenFd, 5) < 0)
	{
		printf("Listen error!\r\n");
		exit(-1);
	}
	printf("Before main loop!\r\n");
	while(1)
	{
		struct sockaddr_in clientAddr;
		socklen_t clientLen = sizeof(clientAddr);
		int connectFd = accept(listenFd, (struct sockaddr*) &clientAddr, &clientLen);
		if(connectFd < 0)
		{
			printf("Accept error!\r\n");
			exit(-1);
		}
		printf("Accept right!\r\n");
		char buffer[BUFFER_SIZE];
		memset(buffer, '\0', BUFFER_SIZE);
		int requestLen = recv(connectFd, buffer, BUFFER_SIZE, 0);
		if(requestLen < 0)
		{
			printf("Recieve error!\r\n");
			exit(-1);
		}
		printf("Recieve right!\r\n");
		char message[BUFFER_SIZE];
		httpRequest request;
		httpHeaderParse(buffer, requestLen, request);
		printf("After httpParser:uri:%s\r\n", request.uri.c_str());
		string filename = request.uri.substr(1, request.uri.size() - 1);	
		struct stat sbuf;
		printf("filename is:%s\r\n", filename.c_str());
		if(stat(filename.c_str(), &sbuf) < 0)
		{
			printf("The file isnot exist!\r\n");
		}
		int messageLen = writeMessage(sbuf.st_size, message);
		printf("Respond message:%s\r\n", message);
		int n = send(connectFd, message, messageLen, 0);
		if(n < 0)
		{
			printf("Send header error!\r\n");
			exit(-1);
		}
		n = readVideoAndSend(connectFd, filename, sbuf);
		if(n < 0)
		{
			printf("Send video error!\r\n");
			exit(-1);
		}
		close(connectFd);
	}
	return 0;
}

int writeMessage(int length, char message[])
{	
	string mes;
	mes += "\r\nHTTP/1.1 ";
	mes += "200 OK";
	mes += "\r\nContent-Type:video/x-mpegurl";
	mes += "\r\nServer:localhost";
	mes += "\r\nContent-Length:" + std::to_string(length);
	time_t timep;
	time(&timep);
	mes += "\r\nDate:" + (string)ctime(&timep);
	mes += "\r\n";
	mes.copy(message, mes.size(), 0);
//	printf("Write message: File content:%s\r\n", mes.c_str());
	*(message + mes.size()) = '\0'; 
//	printf("Write message: File content:%s\r\n", message);
	return mes.size()+1;
}

int readVideoAndSend(int connectFd, string filename, struct stat sbuf)
{
	void *srcp;
	int videoFd = open(filename.c_str(), O_RDONLY, 0);
	if(videoFd < 0)
	{
		printf("Open error!");
		exit(-1);
	}
	if((srcp = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, videoFd, 0)) == ((void *) -1))
	{
		printf("Mmap error!");
		exit(-1);
	}
//	close(videoFd);

	size_t nleft = sbuf.st_size;
	ssize_t nwritten;
//	char *bufp = (char*) srcp;
	char *bufp = static_cast<char*> (srcp);
	while(nleft > 0)
	{
		printf("nleft:%d\r\n", (int)nleft);
		if((nwritten = write(connectFd, bufp, nleft)) <= 0)
		{
			printf("write length:%d\r\n", (int)nwritten);
			if(errno == EINTR)
				nwritten = 0;
			else
			{
				printf("Write error!\r\n");
				exit(-1);
			}
		}
		nleft -= nwritten;
		bufp += nwritten;
	}
	if(munmap(srcp, sbuf.st_size) < 0)
	{
		printf("munmap error!");
		exit(-1);
	}
	close(videoFd);
	return sbuf.st_size;
}

void httpHeaderParse(char* buffer, int n, httpRequest& request)
{
	string indata = buffer;
	int last = 0;
	int i = indata.find(" ", last);
	request.method = indata.substr(last, i - last);
	last = i + 1;
	i = indata.find(" ", last);
	request.uri = indata.substr(last, i - last);
	last = i + 1;
	i = indata.find("\r\n", last);
	request.version = indata.substr(last, i - last);
//	printf("version:%s\r\n", request.version.c_str());
}
