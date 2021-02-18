#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdbool.h>
#include "arpnet.h"

#define IP_ADDRESS "13.95.210.235"

bool turnLeader=false;
node_id IDnode;
bool lastNode;
int state=0;
int sockfd;
int voteCount = 0;
int lenghtTable = 0;
struct timeval timeout;
message_t message_temp;
node_id previous_TL=0;

void handshake();
void vote();
void turn();

int main(int argc, char *argv[]){

	sockfd=net_server_init();
	msg_init(&message_temp);
	
	if (sockfd < 0){
        printf("Error initializing the server");
        exit(-1);
    }

    IDnode=iptab_get_ID_of(IP_ADDRESS);
    lenghtTable=iptab_len();

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    while(voteCount<10){
    	if(state==0 && voteCount==0){
    		printf("Handshake \n");
            handshake();
    	}
    	if(state==1){
    		printf("Vote\n");
    		vote();
    	}
    	if(state==2){
    		printf("Turn\n");
    		turn();
    	}
    }
    printf("Program ended\n");
    close(sockfd);
    return 0;
}

void handshake(){
	
	int imFirst=hsh_imfirst(IP_ADDRESS);
	handshake_t hshmsg;

	if(imFirst==1){
		printf("First node\n");
		hsh_init(&hshmsg);
		int res=hsh_check_availability(IDnode,&hshmsg);
		if(res==0){
			printf("Version not correct\n");
			voteCount=10;
			return;
		}
		node_id nextNode;
		for(int j=0;j<lenghtTable;j++){
			nextNode=iptab_get_next(IDnode+j);
			char* ip_next=iptab_getaddr(nextNode);
			int sock_client=net_client_connection_timeout(ip_next, &timeout);
			if(sock_client>0){
				printf("Node with id %d is available \n", nextNode);
				write(sock_client, &hshmsg, sizeof(handshake_t));
				close(sock_client);
				break;
			}
			printf("Node with id %d is not available \n", nextNode);
		}
		int newsockfd = net_accept_client(sockfd, NULL);
		read(newsockfd,&hshmsg,sizeof(handshake_t));
		close(newsockfd);
		hsh_update_iptab(&hshmsg);
		int sock_client2=net_client_connection(iptab_getaddr(nextNode));
		write(sock_client2, &hshmsg, sizeof(handshake_t));
		close(sock_client2);
		int newsockfd2=net_accept_client(sockfd, NULL);
		read(newsockfd2, &hshmsg, sizeof(handshake_t));
		close(newsockfd2);
		state=1;
	}
	if(imFirst==0){
		printf("I'm not the first node\n");
		hsh_init(&hshmsg);
		node_id nextNode;
		int newsockfd=net_accept_client(sockfd,NULL);
		read(newsockfd, &hshmsg, sizeof(handshake_t));
		close(newsockfd);
		int res=hsh_check_availability(IDnode, &hshmsg);
		for(int j = 0; j < lenghtTable - IDnode; j++) {
			nextNode = iptab_get_next(IDnode + j);
			char* ip_next = iptab_getaddr(nextNode);
			int sock_client=net_client_connection_timeout(ip_next, &timeout);
			if(sock_client>0){
				printf("Node with id %d is available \n", nextNode);
				write(sock_client, &hshmsg, sizeof(handshake_t));
				close(sock_client);
				break;
			}
			printf("Node with id %d is not available \n", nextNode);
		}
		if(res==0){
			printf("Version not corret\n");
			voteCount=10;
			return;
		}
		int newsockfd2 = net_accept_client(sockfd, NULL);
		read(newsockfd2, &hshmsg, sizeof(handshake_t));
		close(newsockfd2);
		hsh_update_iptab(&hshmsg);
		int sock_client2=net_client_connection(iptab_getaddr(nextNode));
		write(sock_client2,&hshmsg,sizeof(handshake_t));
		close(sock_client2);
		state=1;
	}
}

void vote(){
	int imFirst=hsh_imfirst(IP_ADDRESS);
	votation_t votemsg;
	if((voteCount==0 && imFirst==1) || (turnLeader==true)){
		vote_init(&votemsg);
		vote_do_votation(&votemsg);
		node_id nextNode=iptab_get_next(IDnode);
		int sock_client=net_client_connection(iptab_getaddr(nextNode));
		write(sock_client, &votemsg, sizeof(votation_t));
		close(sock_client);
		int newsockfd = net_accept_client(sockfd, NULL);
		read(newsockfd, &votemsg, sizeof(votation_t));
		close(newsockfd);
		node_id nodeWinner=vote_getWinner(&votemsg);
		printf("Next turn leader: %d\n", nodeWinner);
		if(nodeWinner==IDnode){
			turnLeader=true;
		}
		if(nodeWinner!=IDnode){
			turnLeader=false;
			previous_TL=nodeWinner;
		}
		message_t msg_win;
		msg_init(&msg_win);
		msg_set_ids(&msg_win, IDnode, nodeWinner);
		int sock_winner=net_client_connection(iptab_getaddr(nextNode));
		write(sock_winner, &msg_win, sizeof(message_t));
		close(sock_winner);
		state=2;
		voteCount=voteCount+1;
		return;
	}
	if((imFirst==0) || (turnLeader==false)){
		vote_init(&votemsg);
		int newsockfd = net_accept_client(sockfd, NULL);
		read(newsockfd, &votemsg, sizeof(votation_t));
		close(newsockfd);
		vote_do_votation(&votemsg);
		node_id nextNode=iptab_get_next(IDnode);
		int sock_client=net_client_connection(iptab_getaddr(nextNode));
		write(sock_client, &votemsg, sizeof(votation_t));
		close(sock_client);
		int newsockfd2=net_accept_client(sockfd,NULL);
		read(newsockfd2,&message_temp,sizeof(message_t));
		close(newsockfd2);
		node_id nodeWinner=msg_get_turnLeader(&message_temp);
		printf("Turn leader is the node %d\n",nodeWinner);
		if(nodeWinner==IDnode){
			turnLeader=true;
		}
		if(nodeWinner!=IDnode){
			turnLeader=false;
		}
		if(nextNode!=previous_TL){
			int sock_client2=net_client_connection(iptab_getaddr(nextNode));
			write(sock_client2,&message_temp,sizeof(message_t));
			close(sock_client2);
		}
		previous_TL=nodeWinner;
		state=2;
		voteCount=voteCount+1;
		return;
	}
}

void turn(){

	struct timeval totalTimes[10];
    struct timeval flyTimes[10];
    int availableNodes = iptab_len_av();

	for(int i=0;i<10;i++){
		
		if(turnLeader==false){
			int newsockfd=net_accept_client(sockfd, NULL);
			int res = read(newsockfd, &message_temp, sizeof(message_t));
			close(newsockfd);

			printf("Size of a message_t is %li and size of vote message is %li and bytes received are: %i \n", sizeof(message_t), sizeof(votation_t), res);
            fflush(stdout);

			node_id nextNode;
			msg_set_recv(&message_temp);
			msg_mark(&message_temp,IDnode);
			if(msg_all_visited(&message_temp)){
				nextNode=msg_get_turnLeader(&message_temp);
				lastNode=true;
			}
			else{
				nextNode=msg_rand(&message_temp);
				lastNode=false;
			}
			node_id turnLeader=msg_get_turnLeader(&message_temp);
			msg_set_ids(&message_temp,IDnode,turnLeader);
			msg_set_sent(&message_temp);
			int sock_client=net_client_connection(iptab_getaddr(nextNode));
			write(sock_client,&message_temp,sizeof(message_t));
			close(sock_client);
			if(lastNode==false){
				nextNode=msg_get_turnLeader(&message_temp);
				int sock_client2=net_client_connection(iptab_getaddr(nextNode));
				write(sock_client2,&message_temp,sizeof(message_t));
				close(sock_client2);
			}
			msg_init(&message_temp);
		}
		
		if(turnLeader==true){

			struct timeval firstSendTime;
            struct timeval lastRecTime;
            struct timeval totalSendTime;
            struct timeval totalRecTime;
            totalSendTime.tv_sec = 0;
            totalSendTime.tv_usec = 0;
            totalRecTime.tv_sec = 0;
            totalRecTime.tv_usec = 0;
			message_t message;
			msg_init(&message);
			msg_set_ids(&message,IDnode,IDnode);
			for(int tmp=0;tmp<lenghtTable;tmp++){
				node_id tempNode = tmp;
				if(tempNode==IDnode)
					;
				if(tempNode!=IDnode){
					if((iptab_is_available(tempNode))==0)
						msg_mark(&message,tempNode);
				}
			}
			msg_mark(&message,IDnode);
			node_id nextNode=msg_rand(&message);
			msg_set_sent(&message);
			firstSendTime=msg_get_sent(&message);
			int sock_client=net_client_connection(iptab_getaddr(nextNode));
			write(sock_client, &message, sizeof(message_t));
			close(sock_client);
			for(int var=0;var<availableNodes-1;var++){
				int newsockfd=net_accept_client(sockfd,NULL);
				read(newsockfd, &message, sizeof(message_t));
				close(newsockfd);
				timeradd(&totalSendTime, &message.sent, &totalSendTime);
                timeradd(&totalRecTime, &message.recvd, &totalRecTime);
                if (var == availableNodes - 2){
                    lastRecTime = msg_get_recv(&message);
                    timersub(&message.recvd, &firstSendTime, &totalTimes[i]);
                    timersub(&totalSendTime, &totalRecTime, &flyTimes[i]);
                }
                if (i == 9){
	                // Define and initialize the timeval variables
	                struct timeval averageTotTime;
	                struct timeval averageFlyTime;
	                averageTotTime.tv_sec = 0;
	                averageTotTime.tv_usec = 0;
	                averageFlyTime.tv_sec = 0;
	                averageFlyTime.tv_usec = 0;

	                // Define the message variable
	                stat_t stmsg;
	                // Initialize the message
	                stat_message_init(&stmsg);

	                for (int var = 0; var < 10; var++)
	                {
	                    timeradd(&averageTotTime, &totalTimes[var], &averageTotTime);
	                    timeradd(&averageFlyTime, &flyTimes[var], &averageFlyTime);
	                }

	                printf("Average tot time: sec: %ld usec: %ld \n", averageTotTime.tv_sec, averageTotTime.tv_usec);
	                fflush(stdout);
	                printf("Average fly time: sec: %ld usec: %ld \n", averageFlyTime.tv_sec, averageFlyTime.tv_usec);
	                fflush(stdout);

	                // Compute total bandwith
	                float totBand = (float)((8000000*sizeof(message_t)) / ((averageTotTime.tv_sec  * 1000000) + averageTotTime.tv_usec));
	                float flyBand = (float)((8000000*sizeof(message_t)) / ((averageFlyTime.tv_sec  * 1000000) + averageFlyTime.tv_usec));

	                averageTotTime.tv_sec = 0;
	                averageTotTime.tv_usec = 0;
	                averageFlyTime.tv_sec = 0;
	                averageFlyTime.tv_usec = 0;
	                
	                printf("Total band: %f \n", totBand);
	                fflush(stdout);
	                printf("Fly band: %f \n", flyBand);
	                fflush(stdout);

	                // Store results in the meessage
	                /*stat_message_set_totBitrate(&stmsg, totBand);
	                stat_message_set_flyBitrate(&stmsg, flyBand);

	                // Connect to the Professor server
	                int fdSocketProf = net_client_connection(stat_get_RURZ_addr());

	                // Check if the sever of the Profesoor is available
	                if(fdSocketProf > 0) {
	                    // Send the message to the server
	                    if (write(fdSocketProf, &stmsg, sizeof(stat_t)) <= 0)
	                    {
	                        perror("Error sending message to the server of the Professor");
	                        exit(-1);
	                    }

	                    // Close the the communication with server
	                    close(fdSocketProf);

	                    printf("Message sent to the server of the Professor \n");

	                }

	                else {
	                    printf("Server of the Professor is not available \n");
	                    fflush(stdout);
	                }*/
            	}

			}

		}
	
	}

	state=1;
}




