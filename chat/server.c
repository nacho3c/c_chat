#ifdef _WIN32
#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#pragma comment (lib, "Ws2_32.lib")

#define inet_ntop InetNtop
#endif

#include <stdio.h>
#include "config.h"

void error(char *msg);
void process_cmd(SOCKET i, char *buf, int bytecount);
void process_msg(SOCKET i, char *buf, int bytecount);

char * stdmsg_welcome    = "[Server] Hallo, du hast dich zum CChat Server verbunden!\n"
                           "[Server] Bevor du chatten kannst sende bitte /nickname <deinname>"
                           " um deinen Benutzernamen zu setzten!\n\n"
                           "[Server] Folgende Kommandos stehen außerdem zur Verfügung:\n"
                           "[Server] /users - Listet alle verbundenen Clients auf\n"
                           "[Server] /quit  - Trennt die Verbindung zum Server.\n\n";
char * stdmsg_userinuse  = "[Server] Der Benutzername ist bereits vergeben, bitte versuche es erneut!";
char * stdmsg_badname    = "[Server] Ein Benutzername darf nur ASCII-Zeichen enthalten, bitte versuche es erneut!";
char * stdmsg_nonameset  = "[Server] Du kannst erst chatten wenn du einen Benutzernamen gesetzt hast!";
char * stdmsg_userset    = "[Server] Benutzername gesetzt, viel Spaß beim Chatten!";
char * stdmsg_unknowncmd = "[Server] Unbekanntes Kommando.";
char * stdmsg_disconnect = "[Server] Bis bald!";

char * cname_notset = "#NOTSET";

SOCKET master, new_sock;
struct sockaddr_storage client_addr;
socklen_t addr_size;
fd_set set;
char buf[512], clientIp[INET6_ADDRSTRLEN];
int bytecount, i;
SOCKET client_socket[FD_SETSIZE];
char client_name[FD_SETSIZE][MAX_USER_LEN];

int main() {
    printf("CChat Server v%d.%d (compiled with %s %s)\n", CCHAT_VERSION_MAJOR, CCHAT_VERSION_MINOR, CCHAT_COMPILER,
           CCHAT_COMPILER_VERSION);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) error("Konnte Winsock nicht laden.\n");

    master = socket(AF_INET, SOCK_STREAM, 0);
    if(master < 0) exit("Socket konnte nicht erstellt werden.");
    FD_ZERO(&set);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(17347);


    if(bind(master, (struct sockaddr*) &addr, sizeof(addr)) < 0) error("Bind fehlgeschlagen.");
    if(listen(master, 5) == -1) exit("Listen fehlgeschlagen.");

    FD_SET(master, &set);

    for(int i = 0; i < FD_SETSIZE; i++)
        client_socket[i] = 0;

    printf("~ Warte auf Verbindungen... (port 17347)\n\n");

    for(;;)
    {
        FD_ZERO(&set);
        FD_SET(master, &set);

        for(int i = 0; i < FD_SETSIZE; i++)
            if(client_socket[i] > 0)
                FD_SET(client_socket[i], &set);

        if(select(0, &set, NULL, NULL, NULL) == -1)
            exit("Select fehlgeschlagen.");

        if(FD_ISSET(master, &set)) {
            //Aktivität auf Master-Socket -> Neuer Client will sich verbinden
            addr_size = sizeof client_addr;
            if ((new_sock = accept(master, (struct sockaddr *) &client_addr, &addr_size)) == -1)
                perror("~ Konnte Verbindung nicht entgegennehmen.\n");
            else {
                FD_SET(new_sock, &set);
                printf("~ Neue Verbindung von %s (socket %d).\n",
                       inet_ntop(client_addr.ss_family, &(((struct sockaddr_in *) &client_addr)->sin_addr),
                                 clientIp, INET6_ADDRSTRLEN), i);

                for (int j = 0; j < FD_SETSIZE; j++) {
                    if (client_socket[j] == 0) {
                        client_socket[j] = new_sock;
                        strcpy(client_name[j], "#NOTSET");
                    }
                }
            }
        }

        for(i = 0; i < FD_SETSIZE; i++)
        {
            if(FD_ISSET(client_socket[i], &set)) {
                //Aktivität von Client-Socket -> Nachricht oder Disconnect erhalten
                if((bytecount = recv(client_socket[i], buf, sizeof buf, 0)) <= 0){
                    //Fehler oder Disconnect
                    if(bytecount == 0) printf("~ Client %d hat die Verbindung geschlossen.\n", i);
                    else perror("~ Lesefehler bei Nachricht von Client.");
                    close(i);
                    FD_CLR(i, &set);
                } else {
                    //Nachricht erhalten :D
                    if(buf[0] == '/')
                        process_cmd(client_socket[i], buf, bytecount);
                    else process_msg(client_socket[i], buf, bytecount);
                }
            }
        }
    }

    WSACleanup();
    return 0;
}

int find_sock(SOCKET i)
{
    for(int j = 0; j < FD_SETSIZE; j++)
        if(client_socket[j] == i) return j;
    return -1;
}

char * cmd_nickname = "/nickname ";
char * cmd_users = "/users";
char * cmd_quit = "/quit";

void process_cmd(SOCKET i, char *buf, int bytecount) {

    //quit
    if(bytecount < strlen(cmd_quit)) {
        send(i, stdmsg_unknowncmd, strlen(stdmsg_unknowncmd) +1, NULL);
        return;
    } else if(strncmp(cmd_quit, buf, strlen(cmd_quit)) == 0)
    {
        send(i, stdmsg_disconnect, strlen(stdmsg_disconnect) +1, NULL);
        close(i);
        FD_CLR(i, &master);
        return;
    }
    if(bytecount >= strlen(cmd_users) && strncmp(cmd_users, buf, strlen(cmd_users)) == 0)
    {
        //TODO: reply with user list
        return;
    }
    else if(bytecount >= strlen(cmd_nickname) && strncmp(cmd_nickname, buf, strlen(cmd_nickname)) == 0)
    {
        int len = bytecount - strlen(cmd_nickname);
        char * user_name = malloc(len+1);
        if(!user_name)
        {
            perror("~ Konnte Speicher für Benutzernamen nicht reservieren!");
            return;
        }
        strncpy(user_name, buf + strlen(cmd_nickname), len);
        for(int h = 0; h < len; h++)
        {
            if(!isascii(user_name[h]))
            {
                free(user_name);
                send(i, stdmsg_badname, strlen(stdmsg_badname) +1, NULL);
                return;
            }
        }
        for(int h = 0; h < FD_SETSIZE; h++)
        {
            if(client_name[h] != -1 && strcmp(user_name, client_name[h]) == 0)
            {
                send(i, stdmsg_userinuse, strlen(stdmsg_userinuse) +1, NULL);
                return;
            }
        }
        int id = find_sock(i);
        strcpy(client_name[id], user_name);
        send(i, stdmsg_userset, strlen(stdmsg_userset) +1, NULL);
        return;
    }
}


void process_msg(SOCKET i, char *buf, int bytecount) {
    int id = find_sock(i);
    if(id == -1)
    {
        printf("~ Konnte Socket %llu nicht finden?!", i);
        return;
    }
    else {
        if(strcmp(client_name[id], cname_notset) == 0)
            send(i, stdmsg_nonameset, strlen(stdmsg_nonameset), NULL);
        else
        {
            char * msg = malloc(sizeof client_name[id] + bytecount + 4);
            if(!msg)
            {
                perror("~ Konnte Speicher für Nachrichtenverteilung nicht reservieren.");
                return;
            }
            strcat(msg, client_name[id]);
            strcat(msg, ": ");
            strcat(msg, buf);

            for(int j = 0; j < FD_SETSIZE; j++)
            {
                if(client_socket[j] > 0)
                {
                    if(client_socket[j] != master) {
                        if(send(client_socket[j], msg, sizeof msg, NULL) == -1)
                        {
                            perror("~ Fehler beim Verteilen der Nachricht an einen Client.");
                        }
                    }
                }
            }
            free(msg);
            return;
        }
    }
}

void error(char *msg)
{
    perror(msg);
    WSACleanup();
    exit(1);
}