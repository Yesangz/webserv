#ifndef MISSIONS_H
#define MISSIONS_H

#include "arpa/inet.h"
#include "unistd.h"
#include "cstdio"
#include "exception"
#include "sys/epoll.h"
#include "fcntl.h"
#include "errno.h"
#include "string.h"
#include "string"
#include "sys/mman.h"
#include "stdarg.h"
#include "sys/uio.h"
#include "assert.h"
#include "sys/stat.h"
#include"iostream"

// 定义HTTP响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";

// 网站的根目录
const char *doc_root = "/home/yesang/Desktop/webserver/resources";

class missions
{
	/*用于任务执行等的内容*/
private:
	struct sockaddr_in address;
	int fd;

public:
	static int epollfd; // epoll实例
	static int user_count;
	bool connected;

	missions();

	~missions();

	void process(); // 处理HTTP请求的入口，调用HTTP请求处理函数，根据请求的结果调用相应的响应函数
	static void setnonblock(int fd);

	static bool addfd(int epollfd, int fd, bool oneshot);

	static bool removefd(int epollfd, int fd);

	static bool modfd(int epollfd, int fd, int ev);

	void init(int connfd, struct sockaddr_in client_address);

	bool close_conn();

	bool read();

	bool write();

	/*用于HTTP请求处理的内容*/
public:
	static const int FILENAME_LEN = 200;		 // 文件名的最大长度
	static const int MAX_READ_BUFFER = 2048;	 // 读缓冲区的大小
	static const int MAX_WRITE_BUFFER = 3072;	 // 写缓冲区的大小

	enum METHOD
	{
		GET,
		POST,
		HEAD,
		PUT,
		DELETE,
		TRACE,
		OPTIONS,
		CONNECT
	};
	// 用于处理HTTP请求的状态机
	enum CHECK_STATE
	{
		CHECKING_STATE_REQUESTLINE = 0,
		CHECKING_STATE_HEADER,
		CHECKING_STATE_CONTENT
	}; // 主状态机的状态，分别为：正在解析请求行、正在分析请求头。
	enum LINE_STATE
	{
		LINE_OK,
		LINE_BAD,
		LINE_INCOMPLETE
	}; // 从状态机的状态，分别为：读取到完整行、行出错、行数据不完整。用于解析一行的内容
	/*服务器处理HTTP请求的结果：
		”NO_REQUEST请求不完整，需要继续读取数据“、
		“GET_REQUEST获得了完整的客户请求”、
		"BAD_REQUEST客户请求有语法错误"、
		“FORBIDDEN_REQUEST客户对资源没有足够访问权限”、
		”INTERNAL_ERROR“服务器内部错误”、
		“CLOSED_CONNECTION客户端已关闭连接”
		"NO_RESOURCE表示服务器没有请求对应的资源"
		”FILE_REQUEST文件请求,获取文件成功“
	  用于返回解析完 行/头 后的结果
	*/
	enum HTTP_CODE
	{
		NO_REQUEST,
		GET_REQUEST,
		BAD_REQUEST,
		FORBIDDEN_REQUEST,
		INTERNAL_ERROR,
		CLOSED_CONNECTION,
		FILE_REQUEST,
		NO_RESOURCE
	};
	CHECK_STATE checkstate; // 主状态机的状态记录
	int read_index;			// 标识读缓冲区中已经读入的客户端数据的最后一个字节的下一个位置
	int check_index;		// 当前正在分析的字符在读缓冲区中的位置
	int start_line;			// 行在buffer钟的起始位置
	int m_content_len;		// 消息体的长度
	int write_index;		//表示写缓冲区中已经被写入的数据的最后一个字节的下一个位置

	// 请求方法
	METHOD method;

	// 客户请求的目标文件的文件名
	char *m_url;

	// 客户请求的请求方法
	char *m_method;

	// HTTP版本协议号
	char *m_Vertion;

	// 目标文件完整路径：相当于doc_root + m_url
	char m_real_file[FILENAME_LEN];

	// 是否保持连接
	bool m_linger;

	// 主机名
	char *m_host;

	// 目标文件被映射到内存中的起始位置
	char *m_file_address;

	// 目标文件的状态。通过它我们可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息
	struct stat m_file_stat;

private:
	char rbuffer[MAX_READ_BUFFER]; // 读缓冲区
	char wbuffer[MAX_WRITE_BUFFER];// 写缓冲区

	/*解析HTTP请求的起始函数,*/
	HTTP_CODE process_read();
	/*从状态机，解析一行的内容，一直读到'\r''\n'为止*/
	LINE_STATE parse_line();
	/*解析请求行，返回解析得到的状态*/
	HTTP_CODE parse_requestLine(char *temp);
	/*解析请求头，返回解析得到的状态*/
	HTTP_CODE parse_requestHead(char *temp);
	/*解析请求体*/
	HTTP_CODE parse_content(char *temp);
	/*得到完整且正确的HTTP请求时，分析目标文件是否存在，若存在且可读，则内存映射到file_address处*/
	HTTP_CODE do_request();
	
	/*响应*/
	bool process_write(HTTP_CODE ret);
	/*以下这一组函数都被process_write用于调用填充HTTP响应*/
	// 关闭内存映射
	void unmap();
	//向写缓冲写入数据
	bool add_response(const char* format, ...);
	//给响应报文添加状态行
	bool add_state_line(int state, const char* title);
	//添加响应头
	bool add_response_header(int content_length);
	//向响应头中写入正文长度
	bool add_content_len_head(const int length);
	//向响应头写入正文内容
	bool add_content_type_head();
	//向响应头中写入连接状态
	bool add_linger();
	//向响应头添加 隔离 响应正文的\r\n
	bool add_blank_line();
	//添加响应正文
	bool add_content(const char* content);

	//分散写，集中读
	struct iovec m_iv[2];
	int m_iv_count;
	int byte_to_send;
	int byte_have_send;

	//定时器
	
};

int missions::user_count = 0;
int missions::epollfd = -1;

missions::missions() {}

missions::~missions() {}

void missions::init(int connfd, struct sockaddr_in client_address)
{
	this->fd = connfd;
	this->address = client_address;
	checkstate = CHECKING_STATE_REQUESTLINE;
	start_line = 0;
	read_index = 0;
	check_index = 0;
	method = GET;
	m_url = nullptr;
	m_method = nullptr;
	m_Vertion = nullptr;
	m_linger = false;
	m_content_len = 0;
	connected = true;
	m_host = nullptr;
	m_file_address = nullptr;
	m_iv_count = 0;
	byte_have_send = 0;
	byte_to_send = 0;
	bzero(m_real_file, MAX_READ_BUFFER);
}

void missions::setnonblock(int fd)
{
	int flag = fcntl(fd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flag);
}

bool missions::addfd(int epollfd, int fd, bool oneshot)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLRDHUP;
	if (oneshot)
	{
		event.events |= EPOLLONESHOT;
	}
	setnonblock(fd);
	return epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) == 0;
}

bool missions::removefd(int epollfd, int fd)
{
	return epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == 0;
}

bool missions::modfd(int epollfd, int fd, int ev)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
	return epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event) == 0;
}

bool missions::read()
{
	while (true)
	{
		int num = recv(fd, rbuffer + read_index, MAX_READ_BUFFER - read_index, 0);
		if (num == 0)
		{
			return false;
		}
		else if (num == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			throw std::exception();
			return false;
		}
		read_index += num;
	}
	return true;
}

bool missions::write()
{
	printf("write start:---------------------\n");
	int temp = 0;
	printf("byte_to_send : %d\n",byte_to_send);
	if(byte_to_send == 0){
		modfd(epollfd,fd,EPOLLIN);
		init(fd,address);
		return true;
	}

	while (true)
	{
		temp = writev(fd,m_iv,m_iv_count);
		printf("temp : %d\n",temp);
		if(temp < 0){
			if(errno == EAGAIN){
				modfd(epollfd,fd,EPOLLIN);
				return true;
			}
			unmap();
			return false;
		}
		byte_have_send += temp;
		byte_to_send -= temp;

		if(byte_have_send >= m_iv[0].iov_len){
			m_iv[0].iov_len = 0;
			m_iv[1].iov_base += (byte_have_send - write_index);
			m_iv[1].iov_len = byte_to_send;
		}
		else{
			m_iv[0].iov_base += byte_have_send;
			m_iv[0].iov_len -= byte_have_send;
		}

		if(byte_to_send <= 0){
			unmap();
			modfd(epollfd,fd,EPOLLIN);
			std::cout << "m_linger is "<< m_linger << " sent message" << std::endl; 
			if(m_linger){
				return true;
			}
			else{
				return false;
			}
		}
	}
	
}

bool missions::close_conn()
{
	if (fd != -1)
	{
		removefd(epollfd, fd);
		fd = -1;
		user_count--;
		connected = false;
		return true;
	}
	return false;
}

void missions::unmap()
{
	if (m_file_address != nullptr)
	{
		munmap((void *)m_file_address, m_file_stat.st_size);
		m_file_address = nullptr;
	}
}

missions::LINE_STATE missions::parse_line()
{
	char temp;
	for(; check_index < read_index; check_index++)
	{
		temp = rbuffer[check_index];
		if (temp == '\r')
		{
			if (rbuffer[check_index + 1] == '\n')
			{ //'\r'接着'\n'说明读取到了完整的行
				rbuffer[check_index++] = '\0';
				rbuffer[check_index++] = '\0';
				return LINE_OK;
			}
			else if (check_index + 1 == read_index)
			{
				std::cout<<"l2 LINE_INCOMPLETE\n";
				return LINE_INCOMPLETE;
			}
			std::cout<<"l2 LINE_BAD\n";
			return LINE_BAD;
		}
		else if (temp == '\n')
		{
			if (check_index > 1 && rbuffer[check_index - 1] == '\r')
			{
				rbuffer[check_index - 1] = '\0';
				rbuffer[check_index++] = '\0';
				return LINE_OK;
			}
			std::cout<<"end LINE_BAD\n";
			return LINE_BAD;
		}
	}
	return LINE_INCOMPLETE;
}

missions::HTTP_CODE missions::parse_requestLine(char *temp)
{
	// 请求方法
	m_url = strpbrk(temp, " \t"); // 该函数返回 str1 中第一个匹配 str2 中任意一个字符的字符，如果未找到匹配字符则返回NULL
	if (!m_url)
	{
		return BAD_REQUEST;
	}
	*m_url++ = '\0';
	m_method = temp;
	if (strcasecmp(temp, "GET") == 0)
	{
		method = GET;
	}
	else
	{
		return BAD_REQUEST;
	}

	// 目标文件
	m_url += strspn(m_url, " \t");			   // 该函数返回 str1 中第一个不在字符串 str2 中出现的字符下标。
	if (strncasecmp(m_url, "http://", 7) == 0) // 比较字符串的前n个字符
	{
		m_url += 7;
		m_url = strchr(m_url, '/'); // 找目标文件
	}
	if (!m_url || m_url[0] != '/')
	{
		return BAD_REQUEST;
	}
	m_Vertion = strpbrk(m_url, " \t");
	if (!m_Vertion)
	{
		return BAD_REQUEST;
	}
	*m_Vertion++ = '\0';

	// 协议版本
	m_Vertion += strspn(m_Vertion, " \t");
	if (strcasecmp(m_Vertion, "HTTP/1.1") != 0)
	{
		return BAD_REQUEST;
	}

	checkstate = CHECKING_STATE_HEADER;
	return NO_REQUEST;
}

missions::HTTP_CODE missions::parse_requestHead(char *temp)
{
	/*因为请求头是一行一行，每一行都通过\r\n来隔开的，当读到开头没有内容时，即为请求头结束
	没有内容指的是“ 空格或者\r\n，请求头结束标志是\r\n\r\n连续两次，一次头部字段名的结束,一次是请求头 ”
	*/
	if (temp[0] == '\0')
	{
		if (m_content_len != 0)
		{
			checkstate = CHECKING_STATE_CONTENT;
			return NO_REQUEST;
		}
		return GET_REQUEST;
	}
	else if (strncasecmp(temp, "Connection:", 11) == 0)
	{
		temp += 11;
		temp += strspn(temp, " \t"); // 避免冒号后面还有空格
		if (strcasecmp(temp, "keep-alive") == 0)
		{
			m_linger = true;
		}
		std::cout << "m_linger : " << m_linger << std::endl;
	}
	else if (strncasecmp(temp, "Content-Length:", 15) == 0)
	{
		temp += 15;
		temp += strspn(temp, " \t");
		m_content_len = atoi(temp);
		printf("m_content_len : %d\n", m_content_len);
	}
	else if (strncasecmp(temp, "Host:", 5) == 0)
	{
		temp += 5;
		temp += strspn(temp, " \t");
		m_host = temp;
		printf("%s\n",temp);
	}
	else
	{
		printf("unknowen header%s\n", temp);
	}
	return NO_REQUEST;
}

missions::HTTP_CODE missions::parse_content(char *temp)
{
	// 检测是否被完整读入
	if (read_index >= (check_index + m_content_len))
	{
		temp[m_content_len] = '\0';
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

missions::HTTP_CODE missions::do_request()
{
	strcpy(m_real_file, doc_root);
	int len = strlen(doc_root);
	strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);//m_real_file + len是为了跳过已经写入的doc_root
	if (stat(m_real_file, &m_file_stat) != 0)
	{
		return NO_RESOURCE;
	}
	if (!(m_file_stat.st_mode & S_IROTH))
	{ // 查询文件是否可读
		return FORBIDDEN_REQUEST;
	}
	if (S_ISDIR(m_file_stat.st_mode))
	{ // 查询路径对应的是文件还是文件夹
		return BAD_REQUEST;
	}
	int mmp_fd = open(m_real_file, O_RDONLY);
	m_file_address = (char *)mmap(NULL, m_file_stat.st_size, PROT_READ, MAP_SHARED, mmp_fd, 0);
	close(mmp_fd);
	printf("%d writable\n",this->fd);
	return FILE_REQUEST;
}

missions::HTTP_CODE missions::process_read()
{
	HTTP_CODE parse_process = NO_REQUEST; // 记录HTTP请求的处理结果
	LINE_STATE line_state = LINE_OK;	  // 记录读取一行内容的状态
	while ((checkstate == CHECKING_STATE_CONTENT && line_state == LINE_OK)  || (line_state = parse_line()) == LINE_OK)
	{									   // 循环读取每一行内容，再将每行内容交给 行分析或头分析
		char *temp = rbuffer + start_line; // 使用temp的原因是为了实现startline来确定一整段头和尾
		start_line = check_index;
		switch (checkstate)
		{
		case CHECKING_STATE_REQUESTLINE:
			parse_process = parse_requestLine(temp);
			if (parse_process == BAD_REQUEST)
			{
				printf("requestLine parse failed\n");
				return BAD_REQUEST;
			}
			break;
		case CHECKING_STATE_HEADER:
			parse_process = parse_requestHead(temp);
			if (parse_process == GET_REQUEST)
			{
				return do_request();
			}
			else if (parse_process == BAD_REQUEST)
			{
				printf("requestHead parse failed\n");
				return BAD_REQUEST;
			}
			std::cout << "header parse : " << parse_process << std::endl;
			break;
		case CHECKING_STATE_CONTENT: // 消息体的状态变化在请求头的解析里面
			parse_process = parse_content(temp);
			if (parse_process == GET_REQUEST)
			{
				printf("%d parse complete\n",this->fd);
				return do_request();
			}
			line_state = LINE_INCOMPLETE;
			break;
		default:
			return INTERNAL_ERROR;
			break;
		}
		if (line_state == LINE_INCOMPLETE)
		{
			return NO_REQUEST;
		}
		else if (line_state == LINE_BAD)
		{
			return BAD_REQUEST;
		}
	}
	return NO_REQUEST;
}

bool missions::add_response(const char* format, ...){
	if(write_index >= MAX_WRITE_BUFFER){
		return false;
	}
	va_list arg_list;
	va_start(arg_list,format);
	int len = vsnprintf(wbuffer + write_index, MAX_WRITE_BUFFER - write_index - 1, format, arg_list);
	if(len < 0){
		printf("printf failed\n");
		return false;
	}
	else if(len >= MAX_WRITE_BUFFER - write_index - 1){
		return false;
	}
	write_index += len;
	va_end(arg_list);
	return true;
}

bool missions::add_state_line(int state, const char*title){
	return add_response("%s %d %s\r\n","HTTP//1.1",state,title);
}

bool missions::add_content_len_head(const int length){
	return add_response("Content-Length:%d\r\n",length);
}

bool missions::add_content_type_head(){
	return add_response("Content-Type:%s\r\n","text/html");
}

bool missions::add_linger(){
	return add_response("Connection:%s\r\n",(m_linger == true)?"keep-alive":"close");
}

bool missions::add_blank_line(){
	return add_response("%s","\r\n");
}

bool missions::add_content(const char* content){
	return add_response("%s",content);
}

bool missions::add_response_header(int content_length){
	add_content_len_head(content_length);
	add_content_type_head();
	add_linger();
	add_blank_line();
}

bool missions::process_write(HTTP_CODE ret){
	switch (ret)
	{
	case BAD_REQUEST:
		add_state_line(500,error_500_title);
		add_response_header(strlen(error_500_form));
		if(!add_content(error_500_form)){
			return false;
		}
		break;
	case FORBIDDEN_REQUEST:
		add_state_line(403,error_403_title);
		add_response_header(strlen(error_403_form));
		add_content(error_403_form);
		break;
	case INTERNAL_ERROR:
		add_state_line(404,error_403_title);
		add_response_header(strlen(error_404_form));
		add_content(error_404_form);
		break;
	case FILE_REQUEST:
		add_state_line(200,ok_200_title);
		if(m_file_stat.st_size != 0){
			add_response_header(m_file_stat.st_size);
			m_iv[0].iov_base = wbuffer;
			m_iv[0].iov_len = write_index;
			m_iv[1].iov_base = m_file_address;
			m_iv[1].iov_len = m_file_stat.st_size;
			m_iv_count = 2;
			byte_to_send += write_index + m_file_stat.st_size;
			printf("process byte_to_send is :%d\n",byte_to_send);
			printf("%d response prepare completed\n",this->fd);
			return true;
		}
		else{
			const char* ok_string = "<html><body></body><html>";
			add_response_header(strlen(ok_string));
			if(!add_content(ok_string)){
				return false;
			}
		}
		break;
	default:
		return false;
		break;
	}
	//如果是其他情况：
	m_iv[0].iov_base = wbuffer;
	m_iv[0].iov_len = write_index;
	m_iv_count = 1;
	byte_to_send += write_index;
	return true;
}

void missions::process()
{
	HTTP_CODE read_state = process_read();
	if (read_state == NO_REQUEST)
	{
		printf("%d request not complete\n", this->fd);
		modfd(epollfd, fd, EPOLLIN);
		return;
	}

	bool write_state = process_write(read_state);
	if(!write_state){
		close_conn();
	}
	modfd(epollfd,fd,EPOLLOUT);
	printf("end process\n");
}

#endif