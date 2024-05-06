#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 100
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET master_socket, new_socket, client_sockets[MAX_CLIENTS];
    struct sockaddr_in server_address;
    int addrlen = sizeof(server_address), activity, i, valread;
    char buffer[BUFFER_SIZE];

    // Winsock baþlat
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    // Master soket oluþtur
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("socket failed\n");
        return 1;
    }

    // Adres yapýsýný hazýrla
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "192.168.0.36", &server_address.sin_addr) <= 0) {
        printf("Invalid Server IP\n");
        return 1;
    }

    // Master soketin özelliklerini bind ile ata.
    if (bind(master_socket, (SOCKADDR*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        printf("bind failed\n");
        return 1;
    }

    // Master soketi dinle ve en fazla 1 baðlantýyý kabul et
    if (listen(master_socket, 1) == SOCKET_ERROR) {
        printf("listen failed\n");
        return 1;
    }

    printf("Waiting for connections...\n");

    // Client_sockets sýfýrlanarak baþlatýlýr
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    int max_fd = master_socket; //Accept fonksiyonunun nfds parametresi için

    while (1) {
        fd_set readfds;//socket kümesi oluþtur

        FD_ZERO(&readfds);//kümeyi sýfýrla
        FD_SET(master_socket, &readfds);//master socketi readfds kümesine ekle

        // Client soketlerinin eklemesini yap
        for (i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_sockets[i];
            if (sd != 0) {
                //eðer client socket boþ deðilse readfds kümesinin içerisine at.
                FD_SET(sd, &readfds);
                if (sd > max_fd)
                {
                    max_fd = sd;
                }
            }
        }

        // Select: parametre olarak verilen fd'lerin birinde etkinlik algýlanýrsa o etkinliðin sayýsýný döndürür
        //toplu olarak clientlarý gözlemle
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (activity == SOCKET_ERROR) {
            printf("select error\n");
            return 1;
        }

        // Yeni bir baðlantý mevcut mu? (belirtilen soketin readfds'de bulunup bulunmadýðý || master socket üzerinde etkinlik olup olmadýðý)
        if (FD_ISSET(master_socket, &readfds)) {
            //accept ile gelen baðlantý kabul edilir ve yeni soket oluþturulur
            if ((new_socket = accept(master_socket, (struct sockaddr*)&server_address, &addrlen)) == INVALID_SOCKET) {
                printf("accept failed\n");
                return 1;
            }

            printf("New connection, socket fd is %d, port : %d\n", new_socket, PORT);

            // Client soketini ekle
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
            FD_SET(new_socket, &readfds);
            if (new_socket > max_fd) {
                max_fd = new_socket;
            }

        }

        // Clientlardan gelen verileri oku
        for (i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_sockets[i];

            //Belirtilen soketin readfds kümesinde etkinlik var mý kontrolü yapar eðer bu soketten okunabilecek veri varsa bu blok çalýþýr
            if (FD_ISSET(sd, &readfds)) {
                //sd üzerinden okunan veriyi buffer'a ata - valread: gelen verinin boyutu
                if ((valread = recv(sd, buffer, BUFFER_SIZE, 0)) == SOCKET_ERROR) {
                    // Baðlantýyý kapat
                    closesocket(sd);
                    client_sockets[i] = 0;
                }
                else if (valread == 0) {
                    printf("Client disconnected\n");
                    closesocket(sd);
                    client_sockets[i] = 0;
                }
                else {

                    buffer[valread] = '\0';
                    printf("Received from client %d: %s\n", sd, buffer);

                    // Serverdan gelen yanýt
                    const char* response = "Hello from SERVER";
                    send(sd, response, strlen(response), 0);
                }
            }
        }
    }

    // Winsock'u kapat
    WSACleanup();

    return 0;
}
