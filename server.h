#ifndef SERVER_H
#define SERVER_H

#include <libnavajo/libnavajo.hh>
#include <functional>
#include <rapidjson/document.h>

class internpage_t : public DynamicPage {
public:
    bool getPage(HttpRequest *request, HttpResponse *response) {
        if(request->getRequestType() == HttpRequestMethod::POST_METHOD) {
            std::vector<uint8_t> &payload = request->getPayload();
            if(callback)
                callback(payload);
            std::string ret = "Success";
            return fromString(ret, response);
        }
        return false;
    }

    void setcallback(std::function<void(std::vector<uint8_t>)> cb) {
        callback = cb;
    }

private:
    std::function<void(std::vector<uint8_t>)> callback = nullptr;

};

class server_t {
public:
    server_t() {
        webserver = new WebServer;
        page.setcallback(std::bind(&server_t::rx, this, std::placeholders::_1));
        r.add("/callback", &page);
        webserver->addRepository(&r);
        webserver->setServerPort(port);
    }

    void launch() {
        webserver->startService();
        webserver->wait();
    }

    void setcallback(std::function<void(rapidjson::Document&)> cb) {
        callback = cb;
    }


private:
    internpage_t page;
    WebServer *webserver;
    DynamicRepository r;
    unsigned short port = 8080;
    std::function<void(rapidjson::Document&)> callback = nullptr;

    void rx(std::vector<uint8_t> payload) {
        std::string body(payload.begin(), payload.end());
        rapidjson::Document d;
        d.Parse(body.c_str());
        if(callback)
            callback(d);
    }
};

#endif // SERVER_H
