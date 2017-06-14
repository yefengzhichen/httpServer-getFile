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
int writeMessage(string, char*);
int readContent(httpRequest, char*);


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
		char content[BUFFER_SIZE];
		char message[BUFFER_SIZE];
		httpRequest request;
		httpHeaderParse(buffer, requestLen, request);
		printf("After httpParser:version:%s\r\n", request.uri.c_str());
		int contentLen = readContent(request, content);
		printf("File content:%s\r\n", content);
		int messageLen = writeMessage(string(content), message);
		printf("Respond message:%s\r\n", message);
		int n = send(connectFd, message, messageLen, 0);
		if(n < 0)
		{
			printf("Send header error!\r\n");
			exit(-1);
		}
		n = send(connectFd, content, contentLen, 0);
		if(n < 0)
		{
			printf("Send content error!\r\n");
			exit(-1);
		}
		close(connectFd);
	}
	return 0;
}

int writeMessage(string content, char message[])
{	
	string mes;
	mes += "\r\nHTTP/1.1 ";
	mes += "200 OK";
	mes += "\r\nContent-Type: text/html";
	mes += "\r\nServer:localhost";
	mes += "\r\nContent-Length:" + std::to_string(content.size());
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

int readContent(httpRequest request, char content[])
{
//	printf("%s", request.uri.c_str());
	string cont;
	int uriLen = request.uri.size();
//	printf("uri: %s\r\n", request.uri.substr(1, uriLen - 1).c_str());
	ifstream infile(request.uri.substr(1, uriLen - 1));
	string line;
	if(infile.is_open())
	{
		while(getline(infile, line))
		{
			cont += line;
		}
		infile.close();
	}
	cont.copy(content, cont.size(), 0);
//	printf("read Content: File content:%s\r\n", cont.c_str());
	*(content + cont.size()) = '\0';

//	printf("readContent: File content:%s\r\n", content);
	return cont.size() + 1;
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
