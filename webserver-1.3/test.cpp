#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
/* 用于与Acceptor建立连接，测试代码功能 */
int main() {
    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }
	static const in_addr_t kInaddrAny = INADDR_ANY;
    // 设置服务器地址
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9981); // 设置服务器端口号
   serverAddr.sin_addr.s_addr = htonl(kInaddrAny);// 设置服务器 IP 地址

    // 连接服务器
    int ret = connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret == -1) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    std::cout << "Connected to server successfully!" << std::endl;

    // 可以在这里进行读写操作
    
   // 从服务器读取数据
   ssize_t bytesReceived = -1;
   char buffer[1024];
   while(bytesReceived == -1) {
    	bytesReceived = read(sockfd, buffer, sizeof(buffer) - 1);
    }

    buffer[bytesReceived] = '\0';
    std::cout << "Received message: " << buffer << std::endl;
    
    // 关闭套接字
    close(sockfd);

    return 0;
}

