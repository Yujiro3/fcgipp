/**
 * fcgipp.cpp
 *
 * C++ versions 4.4.5
 *
 *      fcgipp : https://github.com/Yujiro3/fcgipp
 *      Copyright (c) 2011-2013 sheeps.me All Rights Reserved.
 *
 * @package         fcgipp
 * @copyright       Copyright (c) 2011-2013 sheeps.me
 * @author          Yujiro Takahashi <yujiro3@gmail.com>
 * @filesource
 */

#include <iostream>
#include <sstream>
#include <string>

#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "fcgipp.h"

namespace fcgi {
    /**
     * コンストラクタ
     *
     * @access public
     * @param string listen
     * @param int port
     */
    FastCGIClient::FastCGIClient(std::string listen, int port) {
        struct sockaddr_in *sin;
        sin = new struct sockaddr_in;

        bzero(sin, sizeof(struct sockaddr_in));
        sin->sin_family      = AF_INET;
        sin->sin_addr.s_addr = inet_addr(listen.c_str());
        sin->sin_port        = htons(port);

        sock    = socket(AF_INET, SOCK_STREAM, 0);
        address = (struct sockaddr *)sin;
        addrlen = sizeof(*sin);
    }

    /**
     * コンストラクタ
     *
     * @access public
     * @param string listen
     */
    FastCGIClient::FastCGIClient(std::string listen) {        
        struct sockaddr_un *sun;
        sun = new struct sockaddr_un;

        bzero(sun, sizeof(struct sockaddr_un));
        sun->sun_family = AF_LOCAL;
        strcpy(sun->sun_path, listen.c_str());

        sock     = socket(PF_UNIX, SOCK_STREAM, 0);
        address  = (struct sockaddr *)sun;
        addrlen  = sizeof(*sun);
    }

    /**
     * デストラクタ
     *
     * @access public
     */
    FastCGIClient::~FastCGIClient() {
        delete address;
    }

    /**
     * FastCGIアプリへ送信
     *
     * @access public
     * @param string stdin Content
     * @return void
     */
    void FastCGIClient::send(std::string *stdin) {
        _connect();
        _send(stdin);
        
        if (sock > 0) {
            close(sock);
        }
    }

    /**
     * FastCGIアプリへ送受信
     *
     * @access public
     * @param string stdin Content
     * @return string
     */
    std::string FastCGIClient::request(std::string *stdin) {
        _connect();
        _send(stdin);
    
        /* 末尾まで読み込み */
        int type;
        do {
            type = _read();
        } while (type != FCGI_END_REQUEST);

        if (sock > 0) {
            close(sock);
        }

        return response;
    }

    /**
     * ヘッダーパケットの送信
     *
     * @access private
     * @param std::string *stdin
     * @return void
     */
    void FastCGIClient::_send(std::string *stdin) {
        requestId = stdin->length() % 50;
        std::stringstream conlength;
        conlength << stdin->length();
        params["CONTENT_LENGTH"] = conlength.str();

        /* リクエストの開始準備 */
        FCGI_BeginRequestBody body;
        body.roleB1 = 0x00;
        body.roleB0 = FCGI_RESPONDER;
        body.flags  = 0x000;
        bzero(body.reserved, sizeof(body.reserved));
        _write(FCGI_BEGIN_REQUEST, (char*)&body, sizeof(body));
    
        /* FCGIパラメータの書き込み */
         FCGX_Stream *paramsStream = FCGX_CreateWriter(sock, requestId, WRITER_LEN, FCGI_PARAMS);
        param_t::iterator rows;
        for (rows = params.begin(); rows != params.end(); rows++) {
            _pair(paramsStream, (*rows).first.c_str(), (*rows).second.c_str());
        }
        FCGX_FClose(paramsStream);
        FCGX_FreeStream(&paramsStream);
    
        /* FastCGIコンテンツの送信 */
        _write(FCGI_STDIN, stdin->c_str(), stdin->length());
    }

    /**
     * ヘッダーパケットの読込
     *
     * @access private
     * @param FCGX_Stream *paramsStream
     * @param const char *key
     * @param const char *value
     * @return void
     */
    void FastCGIClient::_pair(FCGX_Stream *paramsStream, const char *key, const char *value) {
        char header[HEADER_LEN];
        char *hedptr = &header[0];

        int keyLength = strlen(key);
        if (keyLength < 0x80) {
            *hedptr++ = (unsigned char) keyLength;
        } else {
            *hedptr++ = (unsigned char)((keyLength >> 24) | 0x80);
            *hedptr++ = (unsigned char)(unsigned char) (keyLength >> 16);
            *hedptr++ = (unsigned char)(unsigned char) (keyLength >> 8);
            *hedptr++ = (unsigned char)(unsigned char)  keyLength;
        }

        int valueLength = strlen(value);
        if (valueLength < 0x80) {
            *hedptr++ = (unsigned char) valueLength;
        } else {
            *hedptr++ = (unsigned char)((valueLength >> 24) | 0x80);
            *hedptr++ = (unsigned char)(unsigned char) (valueLength >> 16);
            *hedptr++ = (unsigned char)(unsigned char) (valueLength >> 8);
            *hedptr++ = (unsigned char)(unsigned char)  valueLength;
        }

        /* ヘッダーの書き込み */
        int result = FCGX_PutStr(&header[0], hedptr - &header[0], paramsStream);
        if (result != hedptr - &header[0]) {
            throw "Unable to put header buffer.";
        }

        /* キーの書き込み */
        result = FCGX_PutStr(key, keyLength, paramsStream);
        if (result !=  keyLength) {
            throw "Unable to put key buffer.";
        }

        /* 値の書き込み */
        result = FCGX_PutStr(value, valueLength, paramsStream);
        if (result !=  valueLength) {
            throw "Unable to putvalue buffer.";
        }
    }

    /**
     * FastCGIアプリへの書き込み
     *
     * @access private
     * @param unsigned char type
     * @param const char *content
     * @param int length
     * @return int
     */
    int FastCGIClient::_write(unsigned char type, const char *content, int length) {
        FCGI_Header header;

        header.version         = (unsigned char) FCGI_VERSION_1;
        header.type            = (unsigned char) type;
        header.requestIdB1     = (unsigned char) ((requestId >> 8) & 0xff);
        header.requestIdB0     = (unsigned char) ((requestId     ) & 0xff);
        header.contentLengthB1 = (unsigned char) ((length >> 8   ) & 0xff);
        header.contentLengthB0 = (unsigned char) ((length        ) & 0xff);
        header.paddingLength   = (unsigned char) 0;
        header.reserved        = (unsigned char) 0;

        int count = -1;
        /* ヘッダーの送信 */
        count = write(sock, (char *)&header, HEADER_LEN);
        if (count != HEADER_LEN) {
            throw "Unable to write fcgi header.";
        }

        /* コンテンツの送信 */
        count = write(sock, content, length);
        if (count != length) {
            throw "Unable to write fcgi content.";
        }

        return HEADER_LEN + length;
    }

    /**
     * FastCGIアプリからの読み込み
     *
     * @access private
     * @return int
     */
    int FastCGIClient::_read() {
        FCGI_Header *header = new FCGI_Header();

        int count = -1;
        count = read(sock, (char *)header, sizeof(header));
        if (count != HEADER_LEN) {
            throw "Unable to read fcgi header.";
        }
        int length = (header->contentLengthB1 << 8) + header->contentLengthB0;

        if (length > 0) {
            char *buf = new char[(length + header->paddingLength + 1)];
            count = read(sock, buf, length + header->paddingLength);
            if (count != length + header->paddingLength) {
                throw "Unable to read fcgi content.";
            }
            if (header->type == FCGI_STDOUT) {
                response.append(buf);
            }
            delete[] buf;
        } // if (length > 0)

        count = header->type;
        delete header;

        return count;
    }

    /**
     * FastCGIサーバへ接続
     *
     * @access private
     * @return boolean
     */
    bool FastCGIClient::_connect() {
        if (connect(sock, address, addrlen)) {
            return false;
        }
        return true;
    }
} // namespace fcgi