/**
 * fcgipp.h
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

#ifndef __FCGI_CLIENT_H__
#define __FCGI_CLIENT_H__

#include <string>
#include <map>

#include <fastcgi.h>
#include <fcgiapp.h>

namespace fcgi {
    /**
     * 2次元配列コンテナの型定義
     * @typedef std::map<std::string, std::string>
     */
    typedef std::map<std::string, std::string> param_t;

    /**
     * ヘッダーサイズ
     * @const integer
     */
    const int HEADER_LEN = 8;

    /**
     * 書込サイズ
     * @const integer
     */
    const int WRITER_LEN = 8192;

    /**
     * FastCGIClientクラス
     *
     * @package     FastCGIClient
     * @author      Yujiro Takahashi <yujiro3@gmail.com>
     */
    class FastCGIClient {
    public:
        /**
         * FCGI用引数
         * @var param_t
         */
        param_t params;

        /**
         * レコード
         * @var string
         */
        std::string record;

        /**
         * Response
         * @var string
         */
        std::string response;

        /**
         * ソケット
         * @var integer
         */
        int requestId;

        /**
         * ソケット
         * @var integer
         */
        int sock;

        /**
         * 接続先アドレス
         * @var sockaddr
         */
        struct sockaddr *address;

        /**
         * 接続先アドレスサイズ
         * @var integer
         */
        int addrlen;

    public:
        /**
         * コンストラクタ
         *
         * @access public
         * @param string listen
         * @param int port
         */
        FastCGIClient(std::string listen, int port);

        /**
         * コンストラクタ
         *
         * @access public
         * @param string listen
         */
        FastCGIClient(std::string listen);
    
        /**
         * デストラクタ
         *
         * @access public
         */
        ~FastCGIClient();

        /**
         * FastCGIアプリへ送信
         *
         * @access public
         * @param string stdin Content
         * @return void
         */
        void send(std::string *stdin);

        /**
         * FastCGIアプリへ送受信
         *
         * @access public
         * @param string stdin Content
         * @return string
         */
        std::string request(std::string *stdin);

    private:
        /**
         * ヘッダーパケットの送信
         *
         * @access private
         * @param std::string *stdin
         * @return void
         */
        void _send(std::string *stdin);

        /**
         * ヘッダーパケットの読込
         *
         * @access private
         * @param FCGX_Stream *paramsStream
         * @param const char *key
         * @param const char *value
         * @return void
         */
        void _pair(FCGX_Stream *paramsStream, const char *key, const char *value);

        /**
         * ヘッダーパケットの読込
         *
         * @access private
         * @param unsigned char type
         * @param const char *content
         * @param int length
         * @return int
         */
        int _write(unsigned char type, const char *content, int length);

        /**
         * パケットの読込
         *
         * @access private
         * @return void
         */
        int _read();

        /**
         * FastCGIサーバへ接続
         *
         * @access private
         * @return boolean
         */
        bool _connect();
    }; // class FastCGIClient
} // namespace fcgi

#endif // #ifndef __FCGI_CLIENT_H__