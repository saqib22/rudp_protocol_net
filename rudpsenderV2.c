#include <stdio.h>
#include <string.h>  
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdbool.h> 
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <math.h>

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

struct ack{
    int acknowlegment_number;
};

struct fin{
    bool FIN;
};

int createSocket(){
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
    return sock;
}

struct sockaddr_in getReceiverAddress(){
    struct sockaddr_in si_other;

    memset((char *) &si_other, 0, sizeof(si_other));
     
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    si_other.sin_addr.s_addr = htonl(INADDR_ANY);

    return si_other;
}
 
int main(int argc, char** argv)
{
    if(argc < 2)
        die("Error: Please provide the name of file");
    
    int s = createSocket();
    struct sockaddr_in si_other = getReceiverAddress();
    int slen=sizeof(si_other);

    FILE *file = fopen(argv[1], "rb");
    size_t len;
    unsigned int sent_bytes = 0;

    struct timeval read_timeout;
    read_timeout.tv_sec = 1/1000;
    read_timeout.tv_usec = 0;
    
    if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout) == -1){
        die("Cannot set the socket options");
    }

    struct rudp_packet *packets[WINDOW_SIZE];
    for(int i = 0; i < sizeof(packets) / sizeof(packets[0]); i++)
        packets[i] = malloc(sizeof(struct rudp_packet));

    while(1){
        int pckt_number = 0;
        int ack_counter = 0;
        int pkt_counter = 0;
        bool acks[5] = {false};
        struct ack *acknowledgment = malloc(sizeof(struct ack));

        for(int i = 0; i < WINDOW_SIZE; i++){
            len = fread(packets[i]->data, 1, PAYLOAD_SIZE , file);
            if(len == 0){
                packets[i] -> END_OF_FILE = true;
                break;
            }
            sent_bytes += len;
            packets[i] -> sequence_number = i+1;
            packets[i] -> END_OF_FILE = false;
        }
        
        for(int i = 0; i < WINDOW_SIZE; i++){
            if(packets[i]-> END_OF_FILE == true)
                break;
            printf("Sent Packet Size = %d \n", (int) sendto(s, packets[i], sizeof(struct rudp_packet) , 0 , (struct sockaddr *) &si_other, slen));
            pkt_counter++;
        }

        while(1){
            recvfrom(s, acknowledgment, sizeof(acknowledgment), 0, (struct sockaddr *) &si_other, &slen);

            if(acknowledgment -> acknowlegment_number != 0){
                printf("Received Ack for %d\n", acknowledgment->acknowlegment_number);
                acks[acknowledgment -> acknowlegment_number - 1] = true;
                ack_counter++;
            }

            if(acknowledgment -> acknowlegment_number == 0 && ack_counter < pkt_counter){
                for(int i = 0; i < WINDOW_SIZE; i++){
                    if(acks[i] == false){
                        printf("Retransmitted %d Packet Size = %d\n",acknowledgment->acknowlegment_number ,(int) sendto(s, packets[i], sizeof(struct rudp_packet) , 0 , (struct sockaddr *) &si_other, slen));
                    }
                }
            }

            if(ack_counter == pkt_counter){
                free(acknowledgment);
                break;
            }
        }
        
        if(len == 0){
            printf("\n\nTCP Termination\n\n");
            struct fin *fin_from_receiver = malloc(sizeof(struct fin));
            struct rudp_packet *fin_sender = malloc(sizeof(struct rudp_packet));
            struct ack *fin_ack = malloc(sizeof (struct ack));

            while(1){
                
                fin_sender -> END_OF_FILE = true;
                sendto(s, fin_sender, sizeof(struct rudp_packet) , 0 , (struct sockaddr *) &si_other, slen);

                recvfrom(s, fin_from_receiver, sizeof(struct fin), 0, (struct sockaddr *) &si_other, &slen);

                if(fin_from_receiver -> FIN == true){
                    printf("Received FIN from server\n");
                    break;
                }
            }

            read_timeout.tv_sec = 10;
            setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
            while(1){
                fin_ack -> acknowlegment_number = 1;
                sendto(s, fin_ack, sizeof(fin_ack) , 0 , (struct sockaddr *) &si_other, slen);
                
                struct fin *remaining_data = malloc(sizeof(struct fin));
                remaining_data -> FIN = false;
                recvfrom(s, remaining_data, sizeof(struct rudp_packet), 0, (struct sockaddr *) &si_other, &slen);
                if(remaining_data -> FIN == false){
                    printf("Waited 10 seconds for receiver. No data in connection. Now closing !\n");
                    break;
                }
                free(remaining_data);
            }
            
            free(fin_from_receiver);
            free(fin_ack);
            break;
        }
    }

    for(int i = 0; i < WINDOW_SIZE; i++)
        free(packets[i]);

    printf("File Sent Successfully\n");
    fclose(file);
    close(s);
    return 0;
}