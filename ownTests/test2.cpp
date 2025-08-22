#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <cstdio>
#include <cerrno>

#define BACKLOG 10
#define BUF_SIZE 1024

// Başlık dosyaları: standart giriş/çıkış, ağ işlemleri, soket ve poll için gerekli olanlar
// (Aşağıdaki include'lar programın çeşitli bölümlerinde kullanılır)

// fd'yi non-blocking moda al
// Bu fonksiyon, bir dosya tanıtıcısını (soket de dahil) bloklamayan moda geçirir.
void setNonBlocking(int fd) {
    // Mevcut bayrakları al
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) flags = 0;
    // O_NONBLOCK bitini ekleyerek non-blocking yap
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char** argv) {
    // Program argüman kontrolü: bir port ve bir şifre bekleniyor
    if (argc != 3) {
        std::cerr << "Kullanım: " << argv[0] << " <port> <password>\n";
        return 1;
    }

    const char* port = argv[1];
    // const char* password = argv[2]; // Şimdilik kullanılmıyor

    // Adres bilgisi hazırlığı: IPv4, TCP, pasif (sunucu için)
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    // Dinleme soketi oluşturma
    int listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    // SO_REUSEADDR: program tekrar başlatıldığında aynı porta yeniden bind edebilmek için
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Soketi adresle ilişkilendir
    if (bind(listen_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        return 1;
    }

    // Adres bilgisini serbest bırak
    freeaddrinfo(res);

    // Dinlemeye başla
    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        return 1;
    }

    // Dinleme soketini non-blocking moda al (accept bloklamasın)
    setNonBlocking(listen_fd);

    std::cout << "IRC sunucu baslatildi. Port: " << port << std::endl;

    // poll dizisini kur: pfds[0] her zaman dinleme soketini tutar
    struct pollfd pfds[BACKLOG + 1];
    pfds[0].fd = listen_fd;
    pfds[0].events = POLLIN; // gelen bağlantılar için hazır olmayı dinle

    int nfds = 1; // pfds dizisinde kullanılan eleman sayısı

    while (true) {
        // poll: bir veya daha fazla fd üzerinde olay bekle
        int ready = poll(pfds, nfds, -1);
        if (ready < 0) {
            perror("poll");
            break;
        }

        // Yeni bağlantı kontrolü: dinleme soketinde veri (yeni accept çağrısı)
        if (pfds[0].revents & POLLIN) {
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            int client_fd = accept(listen_fd, (struct sockaddr*)&client, &len);
            if (client_fd >= 0) {
                // Yeni client soketini de non-blocking yap
                setNonBlocking(client_fd);
                // poll dizisine ekle (yeni client için veri bekle)
                pfds[nfds].fd = client_fd;
                pfds[nfds].events = POLLIN;
                nfds++;

                // Yeni bağlanan client'a hoşgeldin mesajı gönder
                std::string welcome = ":server NOTICE * :Welcome to simple IRC server!\r\n";
                send(client_fd, welcome.c_str(), welcome.size(), 0);

                // Bağlananın IP adresini yazdır
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
                std::cout << "Yeni bağlantı: " << ip << std::endl;
            }
        }

        // Mevcut client'lardan gelen veriyi işle
        for (int i = 1; i < nfds; i++) {
            if (pfds[i].revents & POLLIN) {
                char buf[BUF_SIZE];
                // recv: client'tan gelen veriyi oku
                int n = recv(pfds[i].fd, buf, sizeof(buf) - 1, 0);
                if (n <= 0) {
                    // n==0 => bağlantı kapandı, n<0 => hata (non-blocking sebepli olabilir)
                    close(pfds[i].fd);
                    // Kaldırılan öğe yerine son öğeyi koy ve diziyi küçült
                    pfds[i] = pfds[nfds-1];
                    nfds--;
                    std::cout << "Bir client ayrildi." << std::endl;
                } else {
                    // Gelen veriyi sonlandır ve yazdır
                    buf[n] = '\0';
                    std::cout << "Client'tan gelen: " << buf;
                }
            }
        }
    }

    // Döngü kırıldıysa dinleme soketini kapat
    close(listen_fd);
    return 0;
}
