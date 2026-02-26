#include <string>
#include <iostream>

#include <openssl/ssl.h>
#include <WinSock2.h>

#undef min

auto main() -> int {
    while (!OPENSSL_init_ssl(0, nullptr));
    WSADATA wsa_data{};
    WSAStartup(0x0202, &wsa_data);
    SSL_CTX* ssl_context{};
    while (!(ssl_context = SSL_CTX_new(TLS_client_method())));
    SSL* ssl_connection{};
    while (!(ssl_connection = SSL_new(ssl_context)));
    SOCKET network_socket{};
    while ((network_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        std::cout << "Error: " << WSAGetLastError() << "\n";
    }
    sockaddr_in peer_address{};
    peer_address.sin_family = AF_INET;
    peer_address.sin_addr.S_un.S_addr = inet_addr("117.56.19.139");
    peer_address.sin_port = htons(443);
    while (connect(network_socket, reinterpret_cast<const sockaddr*>(&peer_address), sizeof peer_address));
    SSL_set_fd(ssl_connection, network_socket);
    SSL_connect(ssl_connection);

    const std::string data_out{
        "POST /m3-emv-trn/exm/signUp HTTP/1.1\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n"
        "Accept-Language: zh-TW,zh;q=0.9,en-US;q=0.8,en;q=0.7\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Content-Length: 200\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Host: www.mvdis.gov.tw\r\n"
        "Origin: https://www.mvdis.gov.tw\r\n"
        "Pragma: no-cache\r\n"
        "Referer: https://www.mvdis.gov.tw/m3-emv-trn/exm/signUp\r\n"
        "Sec-Fetch-Dest: document\r\n"
        "Sec-Fetch-Mode: navigate\r\n"
        "Sec-Fetch-Site: same-origin\r\n"
        "Sec-Fetch-User: ?1\r\n"
        "Upgrade-Insecure-Requests: 1\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/133.0.0.0 Safari/537.36\r\n"
        "sec-ch-ua: \"Not(A:Brand\";v=\"99\", \"Google Chrome\";v=\"133\", \"Chromium\";v=\"133\"\r\n"
        "sec-ch-ua-mobile: ?0\r\n"
        "sec-ch-ua-platform: \"Windows\"\r\n"
        "\r\n"
        "method=add&secDateStr=2025-08-05&secId=1&divId=1&dmvNo=70&licenseTypeCode=3&otp=&idNo=Q224519940&birthdayStr=0941018&name=%E8%94%A1%E6%AC%A3%E5%A6%AE&contactTel=0971642710&email=q0905527645q@gmail.com\r\n"
        "\r\n"
    };
    SSL_write(ssl_connection, data_out.data(), data_out.size());
    std::string data_in{};
    data_in.resize(0x10000);
    int write_pos{ 0 };
    int byte_count{};
    while ((byte_count = SSL_read(ssl_connection, data_in.data() + write_pos, 0x10000)) > 0) {
        data_in.resize(data_in.size() + byte_count);
        write_pos += byte_count;
    }
    for (int i{ 0 }; i < data_in.size() / 0x1000; ++i) {
        std::cout << data_in.substr(i * 0x1000, std::min(0x1000, static_cast<int>(data_in.size()) - i * 0x1000));
    }

    SSL_free(ssl_connection);
    SSL_CTX_free(ssl_context);
    WSACleanup();
    while (true) {}
    return 0;
}