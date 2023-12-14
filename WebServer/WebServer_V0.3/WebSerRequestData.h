//
// Created by Administrator on 2022/3/18.
//

#ifndef WEBSERVER_0_2_WEBSERREQUESTDATA_H
#define WEBSERVER_0_2_WEBSERREQUESTDATA_H

#include <string>
#include <unordered_map>
#include <memory>
#include <queue>

const int EPOLL_WAIT_TIME = 500;

const int STATE_PARSE_URI = 1;
const int STATE_PARSE_HEADERS = 2;
const int STATE_RECV_BODY = 3;
const int STATE_ANALYSIS = 4;
const int STATE_FINISH = 5;

const int MAX_BUFF = 4096;

// 有请求出现但是读不到数据,可能是Request Aborted,
// 或者来自网络的数据没有达到等原因,
// 对这样的请求尝试超过一定的次数就抛弃
const int AGAIN_MAX_TIMES = 200;

const int PARSE_URI_AGAIN = -1;
const int PARSE_URI_ERROR = -2;
const int PARSE_URI_SUCCESS = 0;

const int PARSE_HEADER_AGAIN = -1;
const int PARSE_HEADER_ERROR = -2;
const int PARSE_HEADER_SUCCESS = 0;

const int ANALYSIS_ERROR = -2;
const int ANALYSIS_SUCCESS = 0;

const int METHOD_POST = 1;
const int METHOD_GET = 2;
const int HTTP_10 = 1;
const int HTTP_11 = 2;

enum HeadersState {
    h_start = 0,
    h_key,
    h_colon,
    h_spaces_after_colon,
    h_value,
    h_CR,
    h_LF,
    h_end_CR,
    h_end_LF
};

struct MyTimer;

struct timerCmp {
    bool operator()(std::shared_ptr <MyTimer> &a, std::shared_ptr <MyTimer> &b) const;
};

class WebSerMimeType {
public:
    static WebSerMimeType* GetInstance()
    {
        static WebSerMimeType instance;
        return &instance;
    }

public:
    std::priority_queue <std::shared_ptr<MyTimer>, std::deque<std::shared_ptr< MyTimer>>, timerCmp> m_myTimerQueue;
private:
    pthread_mutex_t m_lock;
    std::unordered_map <std::string, std::string> m_mapMime;

    WebSerMimeType();
    ~WebSerMimeType();

    WebSerMimeType(const WebSerMimeType &m);

public:
    std::string  WebSerGetMime(const std::string &strSuffix);
    void WebSerHandleExpiredEvent();
};

class WebSerRequestData :public std::enable_shared_from_this<WebSerRequestData>{
public:
    int m_nAgainTimes;
    std::string m_strPath;
    int m_nFd;
    int m_nEpollFd;
    // content的内容用完就清
    std::string m_strContent;
    int m_nMethod;
    int m_nHTTPVersion;
    std::string m_strFileName;
    int m_nNowReadPos;
    int m_nState;
    int h_state;
    bool m_bIsFinish;
    bool m_bKeepAlive;
    std::unordered_map <std::string, std::string> m_mapHeaders;
    std::weak_ptr <MyTimer> m_pTimer;

private:
    int WebSerParseURI();

    int WebSerParseHeaders();

    int WebSerAnalysisRequest();

public:

    WebSerRequestData();

    WebSerRequestData(int nEpollFd, int nFd, std::string strPath);

    ~WebSerRequestData();

    void WebSerAddTimer(std::shared_ptr <MyTimer> pTimer);

    void WebSerReset();

    void WebSerSeperateTimer();

    int WebSerGetFd();

    void WebSerSetFd(int nFd);

    void WebSerHandleRequest();

    void WebSerHandleError(int nFd, int nErrNum, std::string strShortMsg);
};

struct MyTimer {
    bool boolDeleted;
    size_t nExpiredTime;
    std::shared_ptr <WebSerRequestData> pRequestData;

    MyTimer(std::shared_ptr <WebSerRequestData> pRequestData, int nTimeOut);

    ~MyTimer();

    void UpDate(int nTimeout);

    bool IsValid();

    void ClearReq();

    void SetDeleted();

    bool IsDeleted() const;

    size_t GetExpTime() const;
};

class MutexLockGuard {
public:
    explicit MutexLockGuard();

    ~MutexLockGuard();

private:
    static pthread_mutex_t lock;

private:
    MutexLockGuard(const MutexLockGuard &);

    MutexLockGuard &operator=(const MutexLockGuard &);
};
#endif //WEBSERVER_0_2_WEBSERREQUESTDATA_H
