//
// Created by Administrator on 2022/3/18.
//


#include "WebSerUtil.h"
#include "WebServerEpoll.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/time.h>
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cstdlib>
#include <iostream>
#include <cstring>


pthread_mutex_t MutexLockGuard::lock = PTHREAD_MUTEX_INITIALIZER;

WebSerMimeType::WebSerMimeType()
{
    m_lock = PTHREAD_MUTEX_INITIALIZER;
}

WebSerMimeType::~WebSerMimeType()
{
    pthread_mutex_destroy(&m_lock);
}

std::string WebSerMimeType::WebSerGetMime(const std::string &strSuffix) {
    if (m_mapMime.size() == 0) {
        pthread_mutex_lock(&m_lock);
        if (m_mapMime.size() == 0) {
            m_mapMime[".html"] = "text/html";
            m_mapMime[".avi"] = "video/x-msvideo";
            m_mapMime[".bmp"] = "image/bmp";
            m_mapMime[".c"] = "text/plain";
            m_mapMime[".doc"] = "application/msword";
            m_mapMime[".gif"] = "image/gif";
            m_mapMime[".gz"] = "application/x-gzip";
            m_mapMime[".pdf"] = "application/pdf";
	        m_mapMime[".htm"] = "text/html";
            m_mapMime[".ico"] = "application/x-ico";
            m_mapMime[".jpg"] = "image/jpeg";
            m_mapMime[".png"] = "image/png";
            m_mapMime[".txt"] = "text/plain";
            m_mapMime[".mp3"] = "audio/mp3";
            m_mapMime[".json"] = "application/json;charset=UTF-8";
            m_mapMime[".pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	        m_mapMime[".js"] = "application/javascript;charset=UTF-8";
	        //m_mapMime["default"] = "text/html";
	        m_mapMime["default"] = "text/plain";
        
	}
        pthread_mutex_unlock(&m_lock);
    }
    if (m_mapMime.find(strSuffix) == m_mapMime.end())
        return m_mapMime["default"];
    else
        return m_mapMime[strSuffix];
}

void WebSerMimeType::WebSerHandleExpiredEvent()
{
    MutexLockGuard lock;
    while (!m_myTimerQueue.empty()) {
        std::shared_ptr <MyTimer> ptimer_now = m_myTimerQueue.top();
        if (ptimer_now->IsDeleted()) {
            m_myTimerQueue.pop();
            //delete ptimer_now;
        } else if (ptimer_now->IsValid() == false) {
            m_myTimerQueue.pop();
            //delete ptimer_now;
        } else {
            break;
        }
    }
}

WebSerRequestData::WebSerRequestData() :
        m_nNowReadPos(0),
        m_nState(STATE_PARSE_URI),
        h_state(h_start),
        m_bKeepAlive(false),
        m_nAgainTimes(0) {
}

WebSerRequestData::WebSerRequestData(int nEpollFd, int nFd, std::string strPath) :
        m_nNowReadPos(0),
        m_nState(STATE_PARSE_URI),
        h_state(h_start),
        m_bKeepAlive(false),
        m_nAgainTimes(0),
        m_strPath(strPath),
        m_nFd(nFd),
        m_nEpollFd(nEpollFd) {
}

WebSerRequestData::~WebSerRequestData() {
    //Epoll::epoll_del(fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
    //struct epoll_event ev;
    // 超时的一定都是读请求，没有"被动"写。
    //ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    //ev.data.fd = fd;
    //epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
    // if (timer.lock())
    // {
    //     shared_ptr<mytimer> my_timer(timer.lock())
    //     my_timer->clearReq();
    //     timer.reset();
    // }
    close(m_nFd);
}

void WebSerRequestData::WebSerAddTimer(std::shared_ptr <MyTimer> pTimer) {
    // shared_ptr重载了bool, 但weak_ptr没有
    //if (!timer.lock())
    m_pTimer = pTimer;
}

int WebSerRequestData::WebSerGetFd() {
    return m_nFd;
}

void WebSerRequestData::WebSerSetFd(int nFd) {
    m_nFd = nFd;
}

void WebSerRequestData::WebSerReset() {
    m_nAgainTimes = 0;
    m_strContent.clear();
    m_strFileName.clear();
    m_strPath.clear();
    m_nNowReadPos = 0;
    m_nState = STATE_PARSE_URI;
    h_state = h_start;
    m_mapHeaders.clear();
    m_bKeepAlive = false;

    if (m_pTimer.lock()) {
        std::shared_ptr <MyTimer> pMytimer(m_pTimer.lock());
        pMytimer->ClearReq();
        m_pTimer.reset();
    }

}

void WebSerRequestData::WebSerSeperateTimer() {
    if (m_pTimer.lock()) {
        std::shared_ptr <MyTimer> pMytimer(m_pTimer.lock());
        pMytimer->ClearReq();
        m_pTimer.reset();
    }
}

void WebSerRequestData::WebSerHandleRequest() {
    char buff[MAX_BUFF];
    bool isError = false;
    while (true) {
        int nReadNum = readn(m_nFd, buff, MAX_BUFF);
        if (nReadNum < 0) {
            perror("1");
            isError = true;
            break;
        } else if (nReadNum == 0) {
            // 有请求出现但是读不到数据，可能是Request Aborted，或者来自网络的数据没有达到等原因
            perror("nReadNum == 0");
            if (errno == EAGAIN) {
                if (m_nAgainTimes > AGAIN_MAX_TIMES)
                    isError = true;
                else
                    ++m_nAgainTimes;
            } else if (errno != 0)
                isError = true;
            break;
        }
        //content:GET / HTTP/1.1
        //Host: 192.168.1.105:8888
        //User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36
        //Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
        //Accept-Encoding: gzip, deflate
        //Accept-Language: zh-CN,zh;q=0.9
        //Cache-Control: max-age=0
        //Connection: keep-alive
        //Upgrade-Insecure-Requests: 1

        //std::string now_read(buff, buff + nReadNum);
        //m_strContent += now_read;
        m_strContent.assign(buff, 0 ,nReadNum);
        if (m_nState == STATE_PARSE_URI) {
            int flag = this->WebSerParseURI();
            if (flag == PARSE_URI_AGAIN) {
                break;
            } else if (flag == PARSE_URI_ERROR) {
                perror("2");
                isError = true;
                break;
            }
        }
        if (m_nState == STATE_PARSE_HEADERS) {
            int flag = this->WebSerParseHeaders();
            if (flag == PARSE_HEADER_AGAIN) {
                break;
            } else if (flag == PARSE_HEADER_ERROR) {
                perror("3");
                isError = true;
                break;
            }
            if (m_nMethod == METHOD_POST) {
                m_nState = STATE_RECV_BODY;
            } else {
                m_nState = STATE_ANALYSIS;
            }
        }
        if (m_nState == STATE_RECV_BODY) {
            int content_length = -1;
            if (m_mapHeaders.find("Content-length") != m_mapHeaders.end()) {
                content_length = stoi(m_mapHeaders["Content-length"]);
            } else {
                isError = true;
                break;
            }
            if (m_strContent.size() < content_length)
                continue;
            m_nState = STATE_ANALYSIS;
        }
        if (m_nState == STATE_ANALYSIS) {
            int flag = this->WebSerAnalysisRequest();
            if (flag < 0) {
                isError = true;
                break;
            } else if (flag == ANALYSIS_SUCCESS) {

                m_nState = STATE_FINISH;
                break;
            } else {
                isError = true;
                break;
            }
        }
    }

    if (isError) {
        //delete this;
        return;
    }
    // 加入epoll继续
    if (m_nState == STATE_FINISH) {
        if (m_bKeepAlive) {
            //printf("ok\n");
            this->WebSerReset();
        } else {
            //delete this;
            return;
        }
    }
    // 一定要先加时间信息，否则可能会出现刚加进去，下个in触发来了，然后分离失败后，又加入队列，最后超时被删，然后正在线程中进行的任务出错，double free错误。
    // 新增时间信息
    //cout << "shared_from_this().use_count() ==" << shared_from_this().use_count() << endl;

    std::shared_ptr <MyTimer> mtimer(new MyTimer(shared_from_this(), 500));
    this->WebSerAddTimer(mtimer);
    {
        MutexLockGuard lock;
        WebSerMimeType::GetInstance()->m_myTimerQueue.push(mtimer);
    }

    __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
    int ret = WebServerEpoll::GetInstance()->WebSerEpollMod(m_nFd, shared_from_this(), _epo_event);
    //cout << "shared_from_this().use_count() ==" << shared_from_this().use_count() << endl;
    if (ret < 0) {
        // 返回错误处理
        //delete this;
        return;
    }
}

int WebSerRequestData::WebSerParseURI() {
    std::string &str = m_strContent;
    //printf("WebSerParseURI:%s \n",str.c_str());
    // 读到完整的请求行再开始解析请求
    int pos = str.find('\r', m_nNowReadPos);
    if (pos < 0) {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    std::string strRequestLine = str.substr(0, pos); //content:GET / HTTP/1.1 /r
    if (str.size() > pos + 1)
        str = str.substr(pos + 1);
    else
        str.clear();
    // Method
    pos = strRequestLine.find("GET");
    if (pos < 0) {
        pos = strRequestLine.find("POST");
        if (pos < 0) {
            return PARSE_URI_ERROR;
        } else {
            m_nMethod = METHOD_POST;
        }
    } else {
        m_nMethod = METHOD_GET;
    }
    //printf("method = %d\n", method);
   // printf("strRequestLine:%s \n",strRequestLine.c_str());
    // filename
    pos = strRequestLine.find("/", pos);
    if (pos < 0) {
        return PARSE_URI_ERROR;
    } else {
        int _pos = strRequestLine.find(' ', pos);
        if (_pos < 0)
            return PARSE_URI_ERROR;
        else {
            if (_pos - pos > 1) {
                m_strFileName = strRequestLine.substr(pos + 1, _pos - pos - 1);// todo
                int __pos = m_strFileName.find('?');
                if (__pos >= 0) {
                    m_strFileName = m_strFileName.substr(0, __pos);
                }
            } else
                m_strFileName = "index.html";
        }
        pos = _pos;
    }
    //cout << "m_strFileName: " << m_strFileName << endl;
    // HTTP 版本号
    pos = strRequestLine.find("/", pos);
    if (pos < 0) {
        return PARSE_URI_ERROR;
    } else {
        if (strRequestLine.size() - pos <= 3) {
            return PARSE_URI_ERROR;
        } else {
            std::string ver = strRequestLine.substr(pos + 1, 3); //[ )
            if (ver == "1.0")
                m_nHTTPVersion = HTTP_10;
            else if (ver == "1.1")
                m_nHTTPVersion = HTTP_11;
            else
                return PARSE_URI_ERROR;
        }
    }
    m_nState = STATE_PARSE_HEADERS;
    return PARSE_URI_SUCCESS;
}

int WebSerRequestData::WebSerParseHeaders() {
    std::string &str = m_strContent;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    for (int i = 0; i < str.size() && notFinish; ++i) {
        switch (h_state) {
            case h_start: {         //去掉Host前面的\r\n，以Host字母H为起始
                if (str[i] == '\n' || str[i] == '\r')
                    break;
                h_state = h_key;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case h_key: {           //检查是否有key值
                if (str[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0)
                        return PARSE_HEADER_ERROR;
                    h_state = h_colon;
                } else if (str[i] == '\n' || str[i] == '\r')
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_colon: {     //去掉key和value中间的空格
                if (str[i] == ' ') {
                    h_state = h_spaces_after_colon;
                } else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_spaces_after_colon: {    //拿到去掉空格后的value起始值
                h_state = h_value;
                value_start = i;
                break;
            }
            case h_value: {             //检查value是否正确
                if (str[i] == '\r') {
                    h_state = h_CR;
                    value_end = i;
                    if (value_end - value_start <= 0)
                        return PARSE_HEADER_ERROR;
                } else if (i - value_start > 255)
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_CR: {            //http以\n结尾，截取value值
                if (str[i] == '\n') {
                    h_state = h_LF;
                    std::string key(str.begin() + key_start, str.begin() + key_end);
                    std::string value(str.begin() + value_start, str.begin() + value_end);
                    m_mapHeaders[key] = value;
                    now_read_line_begin = i;
                } else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_LF: {            //http结尾是以\r\n
                if (str[i] == '\r') {
                    h_state = h_end_CR;
                } else {
                    key_start = i;
                    h_state = h_key;
                }
                break;
            }
            case h_end_CR: {
                if (str[i] == '\n') {
                    h_state = h_end_LF;
                } else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case h_end_LF: {
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
    }
    if (h_state == h_end_LF) {
        str = str.substr(now_read_line_begin);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

int WebSerRequestData::WebSerAnalysisRequest() {
    if (m_nMethod == METHOD_POST) {
        //get content
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        if (m_mapHeaders.find("Connection") != m_mapHeaders.end() && m_mapHeaders["Connection"] == "keep-alive") {
            m_bKeepAlive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }
        //cout << "content=" << m_strContent << endl;
        // test char*
        char *send_content = "I have receiced this.";

        sprintf(header, "%sContent-length: %zu\r\n", header, strlen(send_content));
        sprintf(header, "%s\r\n", header);
        size_t send_len = (size_t) writen(m_nFd, header, strlen(header));
        if (send_len != strlen(header)) {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }

        send_len = (size_t) writen(m_nFd, send_content, strlen(send_content));
        if (send_len != strlen(send_content)) {
            perror("Send content failed");
            return ANALYSIS_ERROR;
        }
        //std::cout << "content size ==" << m_strContent.size() << std::endl;
        return ANALYSIS_SUCCESS;
    } else if (m_nMethod == METHOD_GET) {
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        if (m_mapHeaders.find("Connection") != m_mapHeaders.end() && m_mapHeaders["Connection"] == "keep-alive") {
            m_bKeepAlive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }
        //printf("m_strFileName:%s \n",m_strFileName.c_str());
        int dot_pos = m_strFileName.find('.');
        const char *filetype;
        char *p = NULL;
        std::string strType;
        //printf("dot_pos:%d \n",dot_pos);
        if (dot_pos < 0)
            filetype = WebSerMimeType::GetInstance()->WebSerGetMime("default").c_str();
        else {
            //printf("type%s \n",WebSerMimeType::GetInstance()->WebSerGetMime(m_strFileName.substr(dot_pos)).c_str());
            strType = WebSerMimeType::GetInstance()->WebSerGetMime(m_strFileName.substr(dot_pos));
            filetype = strType.c_str();
        }
        struct stat sbuf;
        if (stat(m_strFileName.c_str(), &sbuf) < 0) {
            WebSerHandleError(m_nFd, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        //printf("filetype:%s \n",filetype);
        p = strstr((char* )filetype, "text");
        if(p != NULL) {
            sprintf(header, "%sContent-type: %s; charset=utf-8\r\n", header, filetype);

        }
        else
            sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        //printf("header:%s \n",header);
        // 通过Content-length返回文件大小
        sprintf(header, "%sContent-length: %ld\r\n", header, sbuf.st_size);

        sprintf(header, "%s\r\n", header);
        size_t send_len = (size_t) writen(m_nFd, header, strlen(header));
        if (send_len != strlen(header)) {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }
        int src_fd = open(m_strFileName.c_str(), O_RDONLY, 0);
        char *src_addr = static_cast<char *>(mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0));
        close(src_fd);
        // 发送文件并校验完整性
        send_len = writen(m_nFd, src_addr, sbuf.st_size);
        if (send_len != sbuf.st_size) {
            perror("Send file failed");
            return ANALYSIS_ERROR;
        }
        munmap(src_addr, sbuf.st_size);
        return ANALYSIS_SUCCESS;
    } else
        return ANALYSIS_ERROR;
}

void WebSerRequestData::WebSerHandleError(int fd, int err_num, std::string short_msg) {
    short_msg = " " + short_msg;
    char send_buff[MAX_BUFF];
    std::string body_buff, header_buff;
    body_buff += "<html><title>TKeed Error</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(err_num) + short_msg;
    body_buff += "<hr><em>  Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-type: text/html\r\n";
    header_buff += "Connection: close\r\n";
    header_buff += "Content-length: " + std::to_string(body_buff.size()) + "\r\n";
    header_buff += "\r\n";
    sprintf(send_buff, "%s", header_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
}

MyTimer::MyTimer(std::shared_ptr <WebSerRequestData> _request_data, int timeout) :
        boolDeleted(false),
        pRequestData(_request_data) {
    struct timeval now;
    gettimeofday(&now, NULL);
    // 以毫秒计
    nExpiredTime = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + timeout;
}

MyTimer::~MyTimer() {
    if (pRequestData) {
        WebServerEpoll::GetInstance()->WebSerEpollDel(pRequestData->WebSerGetFd(), EPOLLIN | EPOLLET | EPOLLONESHOT);
        //close(pRequestData->WebSerGetFd());
    }
    //request_data.reset();
    // if (request_data)
    // {
    //     cout << "request_data=" << request_data << endl;
    //     delete request_data;
    //     request_data = NULL;
    // }
}

void MyTimer::UpDate(int timeout) {
    struct timeval now;
    gettimeofday(&now, NULL);
    nExpiredTime = ((now.tv_sec * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool MyTimer::IsValid() {
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t temp = ((now.tv_sec * 1000) + (now.tv_usec / 1000));
    if (temp < nExpiredTime) {
        return true;
    } else {
        this->SetDeleted();
        return false;
    }
}

void MyTimer::ClearReq() {
    pRequestData.reset();
    this->SetDeleted();
}

void MyTimer::SetDeleted() {
    boolDeleted = true;
}

bool MyTimer::IsDeleted() const {
    return boolDeleted;
}

size_t MyTimer::GetExpTime() const {
    return nExpiredTime;
}

bool timerCmp::operator()(std::shared_ptr <MyTimer> &a, std::shared_ptr <MyTimer> &b) const {
    return a->GetExpTime() > b->GetExpTime();
}


MutexLockGuard::MutexLockGuard() {
    pthread_mutex_lock(&lock);
}

MutexLockGuard::~MutexLockGuard() {
    pthread_mutex_unlock(&lock);
}
