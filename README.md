# rudp_protocol_net
## Introduction
This is an implementation of reliable User Datagram Protocol that aims to provide guaranteed delievery of application data similar to TCP. The transport layer protocols provides process to process communication so this is the protocol which directly communicates with application sockets. The TCP protocol offers Quality of Services in terms of reliability, congestion control, flow control, but these features increases the complexity of TCP which effects its speed. On the other hand, UDP is an unreliable, connectionless protocol which does not provide Quality of Service but UDP is much simpler than TCP and thus much faster. So, we have implemented a hybrid application layer protocol that uses UDP as a transport layer protocol but provides Quality of Service in terms of reliability of data similar to TCP. It increases the complexity on both sender end as well as on the receiving end of application  but this hybrid approach provides speed of UDP as well as reliability of TCP.

## Abstract Design/Protocol
It uses a stop and wait protocol with a window size of 5. The protocol does the following things:
    • The sender injects 5 packets into the network
    • The sender goes into the waiting state and waits for the acknowlegment of these 5 packets.
    • The receiver on the other hands receives the packets and sends the acknowlegment for every packet it receives.
    • If the sender does not receive acknowledgment for a packet then it retransmits the packet to the reciever and waits again.
    • The sender will not send the next 5 packets unitll it has received the acknowlegments for the sent 5 packets.
The diagram shows the stop and wait function of this 5 packets window stop and wait protocol.

## Implementational Details:
To achieve reliability we have introduced the following attributes with the transport layer  
protocol header.
    • Sequence Number
    • Total Packets
    • Fin : boolean
    • We have used a timeout inteval of 100ms on sender side.
This additional header information allowed us to achieve reliability using UDP.
    • The sequence numbers help us to order the out of order packets and place them at correct position in the file.
    • The total packets tells us the size of the file to be sent which is useful for allocation that much space on the receiving side. We have done this because the Disk I/O is slow so storing the packets on every window is an overhead. So we stored them in the memory and once the complete file is received we moved them to the disk.
    • Fin shows the end of file character and now the connection should be terminated oon both sides.
    • The timeout interval helps in retransmission of lost packets if their ack is not received or is lost We used the following function to set timeout
setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout)

Connection Termination:
The termination of connection on both ends is an important task and if left uncheck could lead to Segmentation errors or memory overflows. So the connection is closed gracefully and sender and receiving programs return the resources back to the system.
    • Sender sends the FIN packet to the receiver once it has sent the complete file and got it acknowledged by the receiver.
    • The receiver receives this FIN packet and sends another FIN packet back to the sender telling him that he has received and wants to close the connection.
    • The sender then acknowledges this FIN packet of receiver and goes into a failry long wait state of about 10 seconds according to our implementation. The receiver on receving this ack terminates while the sender terminates after this long wait period and if nothing is received from the receiver during this wait.
We have not used the standard 4-way termination protocol and not sent an ack to the sender FIN packet because according to my applcation the sender and recevier will want to close at the same time and there is no work to do for receiver after the sender is terminated so once the sender sends the FIN the receiver also sends the FIN in response and both  terminates gracefully returning the resources to the system.
