#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h> 
#include <stdbool.h> 
#include <errno.h>

#define PORT 12345
#define PAYLOAD_SIZE 500
#define WINDOW_SIZE 5

void die(char *s)
{
    perror(s);
    exit(1);
}

struct rudp_packet{
    int sequence_number;
    bool END_OF_FILE;
    char data[PAYLOAD_SIZE];
};

struct fin{
    bool FIN;
};

struct ack{
    int acknowlegment_number;
};

int createSocket(){
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
    return sock;
}

struct sockaddr_in getMyAddress(){
    struct sockaddr_in si_me;

    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    return si_me;
}

void bindSocket(int *sock, struct sockaddr_in addr){
    if( bind(*sock , (struct sockaddr*)&addr, sizeof (addr) ) == -1)
    {
        die("bind");
    }
}

void copyPackets(struct rudp_packet *dest, struct rudp_packet *src){
    dest -> sequence_number = src -> sequence_number;
    dest -> END_OF_FILE = src -> END_OF_FILE;
    memcpy(dest -> data, &src -> data, sizeof(src -> data));
}

int main(int argc, char** argv)
{
    if(argc < 2)
        die("Error: Please provide the name of file");
    
    struct sockaddr_in si_other;
     
    int slen = sizeof(si_other) , recv_len;
     
    int s = createSocket();
    struct sockaddr_in si_me = getMyAddress();
    
    bindSocket(&s, si_me);
    
    int index = 0;
    FILE *file = fopen(argv[1], "wb");
    struct rudp_packet *packet = malloc(sizeof(struct rudp_packet));
    struct ack *ackknowlegment = malloc(sizeof(struct ack));
    

    struct rudp_packet *packets[WINDOW_SIZE];
    for(int i = 0; i < WINDOW_SIZE; i++){
        packets[i] = malloc(sizeof(struct rudp_packet));
        packets[i] ->sequence_number = 0;
    }

    int packets_received = 0;
    
    while(1)
    {
        recv_len = recvfrom(s, packet, sizeof(struct rudp_packet), 0, (struct sockaddr *) &si_other, &slen);

        printf("Received Bytes = %d\n", recv_len);

        if(packet -> END_OF_FILE == true){
            printf("Entered End\n");
            for(int i = 0; i < WINDOW_SIZE; i++){
                if(packets[i] ->sequence_number != 0)
                    fwrite(packets[i] -> data, 1, PAYLOAD_SIZE, file);
                packets[i] -> sequence_number = 0;
            }
            break;
        }
        
        if((packet -> sequence_number == 1 && packets_received == WINDOW_SIZE)){
            for(int i = 0; i < WINDOW_SIZE; i++){
                fwrite(packets[i] -> data, 1, PAYLOAD_SIZE, file);
                packets[i] -> sequence_number = 0;
            }            
            packets_received = 0;
        }

        index = packet -> sequence_number - 1;
        if(packets[index] -> sequence_number == 0){
            packets_received++;
            printf("Sequence Numner = %d\n", packet->sequence_number);
            copyPackets(packets[index], packet);
        }
        
        ackknowlegment -> acknowlegment_number = packet -> sequence_number;
        sendto(s, ackknowlegment, sizeof(ackknowlegment) , 0 , (struct sockaddr *) &si_other, slen);

    }


    printf("TCP termination");
    struct timeval read_timeout;
    read_timeout.tv_sec = 1/1000;
    read_timeout.tv_usec = 0;
    
    if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout) == -1){
        die("Cannot set the socket options");
    }

    while(1){
        struct fin *fin_to_sender = malloc(sizeof(struct fin));
        fin_to_sender -> FIN = true;
        sendto(s, fin_to_sender, sizeof(struct rudp_packet), 0, (struct sockaddr *) &si_other, slen);

        struct ack *acknowledgment = malloc(sizeof(struct ack));
        if(recv_len = recvfrom(s, acknowledgment, sizeof(acknowledgment), 0, (struct sockaddr *) &si_other, &slen) == -1){
            fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
        }

        if(acknowledgment -> acknowlegment_number == 1){
            printf("FIN of receiver ACKed by the sender. Closing Receiver now !\n");
            break;
        }
    }

    free(packet);
    free(ackknowlegment);
    fclose(file);

    close(s);
    return 0;
}