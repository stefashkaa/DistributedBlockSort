#ifndef EVERESTAPI_CONNECTOR_CPP
#define EVERESTAPI_CONNECTOR_CPP

#include "curl/curl.h"
#include <sys/stat.h>
#include "iostream"

using namespace std;

class Connector {

private:
    CURL *curl;

public:
    struct Response {
        long code;
        string response;
    };

    Connector() {
        init();
    }

    ~Connector() {
        curl_easy_cleanup(curl);
    }

    void init() {
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "build/cookie.txt");
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "build/cookie.txt");

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        //Debug
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, true);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    Response postRequest(string URL, string data) {
        if (!curl) {
            throw runtime_error("curl crushed");
        }

        Response newResponse;

        curl_easy_setopt(curl, CURLOPT_HTTPPOST, true);
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        if (!data.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &newResponse.response);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &newResponse.code);
        curl_easy_cleanup(curl);

        return newResponse;
    }

    Response getRequest(string URL) {
        if (!curl) {
            throw runtime_error("curl crushed");
        }

        Response newResponse;
        curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &newResponse.response);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &newResponse.code);
        curl_easy_cleanup(curl);

        return newResponse;
    }

    Response deleteRequest(string URL) {
        if (!curl) {
            throw runtime_error("curl crushed");
        }

        Response newResponse;
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &newResponse.response);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &newResponse.code);
        curl_easy_cleanup(curl);

        return newResponse;
    }

    Response uploadFile(string URL, string fullName) {
        if (!curl) {
            throw runtime_error("curl crushed");
        }
        FILE* file = fopen(fullName.c_str(), "rb");

        if (!file) {
            throw runtime_error("file error");
        }
        struct stat file_info;

        if(fstat(fileno(file), &file_info) != 0) {
            throw runtime_error("file reading error");
        }
        struct curl_slist *headers = NULL;

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        headers = curl_slist_append(headers, "Expect:");
        //headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, &fread);
        curl_easy_setopt(curl, CURLOPT_READDATA, file);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

        Response newResponse;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &newResponse.response);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &newResponse.code);
        curl_easy_cleanup(curl);

        fclose(file);
        return newResponse;
    }

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        ((string *) userp)->append((char *) contents, size * nmemb);
        return size * nmemb;
    }
};

#endif