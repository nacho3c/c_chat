#ifdef _WIN32
#include <winsock2.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <unistd.h>
#define closesocket(x) close(x)
#define SOCKET unsigned int
#endif

#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 443

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf(":: Usage: tls_client <hostname>\n");
        exit(1);
    }

    int res;
    //Windows specific: Initialize winsock using WSAStartup(..)
    //
#ifdef _WIN32
    WSADATA wsaData;
    res = WSAStartup(MAKEWORD(2, 2), &wsaData); //MAKEWORD(2,2) => use Winsock ver 2.2
    if (res != 0)
    {
        printf(":: WSAStartup() failed.");
        exit(1);
    }
#endif

    SOCKET conn;
    struct sockaddr_in addr;
    struct hostent *host = gethostbyname(argv[1]);

    if(host == NULL)
    {
#ifdef _WIN32
        printf(":: Error resolving hostname, code %d", WSAGetLastError());
#else
        herror(":: Error resolving hostname");
#endif
        exit(1);
    }

    //Initialise OpenSSL context
    //
    printf(":: Initialising OpenSSL..\n");
    const SSL_METHOD  *method;
    SSL_CTX     *ctx;

    SSL_library_init(); //Needed on debian derivates
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    method = TLSv1_2_client_method();
#else
    method = TLS_client_method();
#endif
    ctx = SSL_CTX_new(method);
    if(ctx == NULL)
    {
        printf(":: Failed to create context.\n");
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    //Connect to server
    //
    printf(":: Connecting to server..\n");
    conn = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = *(u_long*)(host->h_addr);

    res = connect(conn, (struct sockaddr *)&addr, sizeof(addr));
    if(res != 0)
    {
        close(conn);
        perror(":: Could not connect to host");
        exit(1);
    }

    //Set up OpenSSL
    //
    printf(":: Connected, attaching OpenSSL to socket..\n");
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, conn);
    res = SSL_connect(ssl);
    if(res == -1)
    {
        printf(":: Failed to negotiate a secure connection.\n");
        ERR_print_errors_fp(stdout);
    } else {
        char msg[1024];
        char response[2048*5];
        X509 *cert = SSL_get_peer_certificate(ssl);
        printf(":: Secure connection established. (Cipher: %s)\n", SSL_get_cipher(ssl));
        if(cert != NULL) {
            printf(":: Subject - %s\n:: Issuer - %s\n\n",
                   X509_NAME_oneline(X509_get_subject_name(cert), 0, 0),
                   X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0)
            );
        }
        printf(":: Please type your request, hostname will be automatically appended:\n>");
        fgets(msg, 1024, stdin);
        strcat(msg, "Host: ");
        strcat(msg, argv[1]);
        strcat(msg, "\n\n");

        SSL_write(ssl, msg, strlen(msg) + 1);
        res = SSL_read(ssl, response, sizeof(response));
        response[res] = 0;
        printf("\n:: Server replied:\n%s", response);

        SSL_free(ssl);
    }
    closesocket(conn);
    SSL_CTX_free(ctx);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}