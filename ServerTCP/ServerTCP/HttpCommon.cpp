#define NOMINMAX         
#include "HttpCommon.h"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <winsock2.h>


using namespace std;

namespace common
{

    std::string httpDate()
    {	// RFC 1123 date format
        char buf[128]; time_t t = time(nullptr); struct tm tm;
        gmtime_s(&tm, &t);
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
        return buf;
    }

    HttpMethod toMethod(const string& t)
    {
        if (t == "GET") return HttpMethod::GET;
        if (t == "HEAD")return HttpMethod::HEAD;
        if (t == "POST")return HttpMethod::POST;
        if (t == "PUT") return HttpMethod::PUT;
        if (t == "DELETE")return HttpMethod::DELETE_;
        if (t == "OPTIONS")return HttpMethod::OPTIONS;
        if (t == "TRACE") return HttpMethod::TRACE;
        return HttpMethod::NONE;
    }

    bool requestComplete(const string& buf)
    {
        size_t hdr = buf.find("\r\n\r\n");
        if (hdr == string::npos) return false;
        size_t need = hdr + 4;
        size_t cl = buf.find("Content-Length:", 0);
        if (cl != string::npos && cl < hdr) {
            size_t val = buf.find_first_of("0123456789", cl + 15);
            need += std::stoul(buf.substr(val));
        }
        return buf.size() >= need;
    }

    bool loadFile(const std::string& p, std::string& out)
    {
        std::ifstream f(p, std::ios::binary);
        if (!f) return false;
        std::ostringstream ss; ss << f.rdbuf(); out = ss.str(); return true;
    }
}