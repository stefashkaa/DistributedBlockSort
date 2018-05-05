#ifndef EVERESTAPI_CONNECTOR_CPP
#define EVERESTAPI_CONNECTOR_CPP

#include "curl/curl.h"
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
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookie.txt");
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookie.txt");
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        //Debug
        curl_easy_setopt(curl, CURLOPT_VERBOSE, true);
    }

    ~Connector() {
        curl_easy_cleanup(curl);
    }

    Response postRequest(string URL, string data) {
        if (!curl)
            throw runtime_error("curl crushed");
        Response newResponse;
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, true);
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        if (!data.empty())
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &newResponse.response);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &newResponse.code);
        return newResponse;
    }

    Response getRequest(string URL) {
        if (!curl)
            throw runtime_error("curl crushed");
        Response newResponse;
        curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &newResponse.response);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &newResponse.code);
        return newResponse;
    }

    Response deleteRequest(string URL) {
        if (!curl)
            throw runtime_error("curl crushed");
        Response newResponse;
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &newResponse.code);
        return newResponse;
    }

    Response uploadFile(string URL, string filepath) {
        if (!curl)
            throw runtime_error("curl crushed");
        Response newResponse;
        //curl_mime *form = curl_mime_init(curl);
        //curl_mimepart *field = curl_mime_addpart(form);
        //curl_mime_name(field, "file");
        //curl_mime_filedata(field, filepath.c_str());
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Expect:");
        //headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        //curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &newResponse.response);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &newResponse.code);
        return newResponse;
    }

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
        ((string *) userp)->append((char *) contents, size * nmemb);
        return size * nmemb;
    }
};

#endif