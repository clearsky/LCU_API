#pragma once
#include "curl/curl.h"
#include "utils.h"
#include <vector>
#include <iostream>

static size_t WriteFunction(void* input, size_t uSize, size_t uCount, void* avg)
{
	size_t uLen = uSize * uCount;
	std::string* pStr = (std::string*)(avg);
	pStr->append((char*)(input), uLen);
	return uLen;
}

class httpClient {
public:
	httpClient() {
		init();
	};
	~httpClient() {
		if (!pCurl) {
			curl_easy_cleanup(pCurl);
		}
	};
public:
	void init() {
		pCurl = curl_easy_init();
		if (pCurl) {
			// 设置超时时间
			curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 5);
			// 忽略https验证
			curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 0);
			curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0);
			// 数据回调函数
			curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &WriteFunction);
		}
	}
	// 设置请求头
	bool setHeader(std::string header) {
		if (!pCurl) {
			return false;
		}
		if (!header.empty()) {
			std::vector<std::string> result;
			utils->split(header, result, '\n');
			curl_slist* pHeaders = NULL;
			for (std::string& item : result) {
				pHeaders = curl_slist_append(pHeaders, item.c_str());
			}
			curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pHeaders);
		}
		return true;
	};
	// 设置请求端口
	bool setPort(int port = -1) {
		if (!pCurl) {
			return false;
		}
		curl_easy_setopt(pCurl, CURLOPT_PORT, port);
		return true;
	};
	// 设置http2
	bool setHttp2(bool http2) {
		if (!pCurl) {
			return false;
		}
		if (http2) {
			curl_easy_setopt(pCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
		}
		else {
			curl_easy_setopt(pCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
		}
		return true;
	};
	// 请求
	bool request(std::string url, std::string& ret, const std::string& reuqestData, const std::string&  method) {
		if (!pCurl) {
			return false;
		}
		// 设置url
		curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str());
		// 设置请求方法
		std::string method_ = utils->ToLower(method);
		if (method.empty()) {
			curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, "get");
		}else{
			curl_easy_setopt(pCurl, CURLOPT_CUSTOMREQUEST, method_.c_str());
		}
		// 设置请求数据
		if (method_ != "get" && !reuqestData.empty()) {
			curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, reuqestData.c_str());
		}
		// 数据回调函数的参，一般为Buffer或文件fd
		std::string sBuffer;
		curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &sBuffer);		
		CURLcode code;
		try {
			code = curl_easy_perform(pCurl);
		}
		catch(...) {
			if (pCurl) {
				curl_easy_cleanup(pCurl);
				pCurl = nullptr;
			}
			init();
			return false;
		}
		
		if (code != CURLE_OK) {
			std::cout << "Request OnError" << std::endl;
			return false;
		}
		long retCode = 0;
		code = curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &retCode);
		if (code != CURLE_OK) {
			std::cout << "HTTP Error Code:" << CURLE_OK<< std::endl;
			return false;
		}
		ret = sBuffer;
		return true;
	}
private:
	CURL* pCurl;
};