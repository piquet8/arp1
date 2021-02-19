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

// initialize global variables
bool turnLeader = false;
node_id idNode = 0;
int state = 0;
char* ipAddr = "13.95.210.235";
int fdSocket = 0;
FILE *fLog; // Log file
int turnCount = 0;
int lenghtTable = 0;
struct timeval timeout;

// Declare functions
void handshake();
void vote();
void turn(message_t* msg);

void handshake()
{
    // Define the message variable
    handshake_t hshmsg;
    
    int firstNode = hsh_imfirst(ipAddr);

    // Control if we are the first of the address table
    if (firstNode == 1)
    {
	    printf("I'm the first node \n");
        fflush(stdout);

        // Initialize the message
        hsh_init(&hshmsg);
        // Control if our version of the libray is the correct one
        if (hsh_check_availability(idNode, &hshmsg) == 1)
        {
            // Define variables
            node_id nextNodeId;

            // Find the next available node and send a message to it
            for(int j = 0; j < lenghtTable; j++) {
                // Obtain the id of the next node
                node_id nextNodeId = iptab_get_next(idNode + j);
                // Get ip address of the next node
                char* ipNextNode = iptab_getaddr(nextNodeId);
                // Connect to the next node
                int fdSocketNextNode = net_client_connection_timeout(ipNextNode, &timeout);

                // Control if the node is available
                if(fdSocketNextNode > 0) {

                    printf("Node with id %i is available \n", nextNodeId);
                    fflush(stdout);
                   

                    // Send the message to the next node
                    if (write(fdSocketNextNode, &hshmsg, sizeof(handshake_t)) <= 0)
                    {
                        perror("Error sending message to the next node");
                        exit(-1);
                    }

                    // Close the socket
                    close(fdSocketNextNode);


                    // Exit from the for loop
                    break;
                }

                printf("Node with id %i is not available \n", nextNodeId);
                fflush(stdout);

            }

            // Wait the message of the last node
            int fdSocketClient = net_accept_client(fdSocket, NULL);

            // Read the message from the client
            if (read(fdSocketClient, &hshmsg, sizeof(handshake_t)) <= 0)
            {
                perror("Error reading message form last node");
                exit(-1);
            }

            // Close the last node socket
            close(fdSocketClient);


            // Update address table
            hsh_update_iptab(&hshmsg);

            // Obtain the id of the next node
            nextNodeId = iptab_get_next(idNode);
            // Get ip address of the next node
            char* ipNextNode = iptab_getaddr(nextNodeId);
            // Connect to the next node
            int fdSocketNextNode = net_client_connection(ipNextNode);
            // Send the message to the next node
            if (write(fdSocketNextNode, &hshmsg, sizeof(handshake_t)) <= 0)
            {
                perror("Error sending message to the next node");
                exit(-1);
            }

            // Close the socket
            close(fdSocketNextNode);

    

            // Wait the message of the last node
            fdSocketClient = net_accept_client(fdSocket, NULL);

            // Read the message from the client
            if (read(fdSocketClient, &hshmsg, sizeof(handshake_t)) <= 0)
            {
                perror("Error reading message form last node");
                exit(-1);
            }

            // Close the last node socket
            close(fdSocketClient);

        }

        else
        {
            printf("The version of the arpnet library is not updated!");
            fflush(stdout);

            exit(-1);
        }
    }

    // Our node has not id zero in the address table
    else
    {
        // Initialize the message
        hsh_init(&hshmsg);
        // Define node_id variable
        node_id nextNodeId;

        // Wait the message of the previous
        int fdSocketClient = net_accept_client(fdSocket, NULL);

        // Read the message from the client
        if (read(fdSocketClient, &hshmsg, sizeof(handshake_t)) <= 0)
        {
            perror("Error reading message form last node");
            exit(-1);
        }

        // Close the previous node socket
        close(fdSocketClient);



        // Control if our version of the libray is the correct one
        int correctVersion = hsh_check_availability(idNode, &hshmsg);

      

        // Find the next available node and send a message to it
        for(int j = 0; j < lenghtTable - idNode; j++) {

            // Obtain the id of the next node
            node_id nextNodeId = iptab_get_next(idNode + j);
            // Get ip address of the next node
            char* ipNextNode = iptab_getaddr(nextNodeId);
            // Connect to the next node
            int fdSocketNextNode = net_client_connection_timeout(ipNextNode, &timeout);

            // Control if the node is available
            if(fdSocketNextNode > 0) {

                printf("Node with id %i is available \n", nextNodeId);
                fflush(stdout);

                // Send the message to the next node
                if (write(fdSocketNextNode, &hshmsg, sizeof(handshake_t)) <= 0)
                {
                    perror("Error sending message to the next node");
                    exit(-1);
                }

                // Close the socket
                close(fdSocketNextNode);


                break;
            }

            printf("Node with id %i is not available \n", nextNodeId);
            fflush(stdout);
        }

        if (correctVersion == 0)
        {
            printf("The version of the arpnet library is not updated!");
            fflush(stdout);

            exit(-1);
        }

        // Wait the message of the last node
        fdSocketClient = net_accept_client(fdSocket, NULL);

        // Read the message from the client
        if (read(fdSocketClient, &hshmsg, sizeof(handshake_t)) <= 0)
        {
            perror("Error reading message form last node");
            exit(-1);
        }

        // Close the last node socket
        close(fdSocketClient);


        // Update address table
        hsh_update_iptab(&hshmsg);

        // Obtain the id of the next node
        nextNodeId = iptab_get_next(idNode);
        // Get ip address of the next node
        char* ipNextNode = iptab_getaddr(nextNodeId);
        // Connect to the next node
        int fdSocketNextNode = net_client_connection(ipNextNode);
        // Send the message to the next node
        if (write(fdSocketNextNode, &hshmsg, sizeof(handshake_t)) <= 0)
        {
            perror("Error sending message to the next node");
            exit(-1);
        }

        // Close the socket
        close(fdSocketNextNode);
    }

    // Change state
    state = 1;
}

void vote()
{

    // Obtain the id of our node
    //idNode = iptab_get_ID_of(ipAddr);
    int firstNode = hsh_imfirst(ipAddr);

    // Control if is the fisrt iteration of vote state and control
    // if we are the first of the address table
    if ((turnCount == 0 && firstNode == 1) || turnLeader == true)
    {
        // Define the message variable
        votation_t votemsg;

        // Initialize the message
        vote_init(&votemsg);

        // Do the votation
        vote_do_votation(&votemsg);

        printf("Votation done! \n");
        fflush(stdout);

        // Obtain the id of the next node
        node_id nextNodeId = iptab_get_next(idNode);
        // Get ip address of the next node
        char* ipNextNode = iptab_getaddr(nextNodeId);
        // Connect to the next node
        int fdSocketNextNode = net_client_connection(ipNextNode);
        // Send the message to the next node
        if (write(fdSocketNextNode, &votemsg, sizeof(votation_t)) <= 0)
        {
            perror("Error sending message to the next node");
            exit(-1);
        }

        // Close the socket
        close(fdSocketNextNode);


        // Wait the message of the last node
        int fdSocketClient = net_accept_client(fdSocket, NULL);

        // Read the message from the client
        if (read(fdSocketClient, &votemsg, sizeof(votation_t)) <= 0)
        {
            perror("Error reading message form last node");
            exit(-1);
        }

        // Close the last node socket
        close(fdSocketClient);


        // Get the id of the winner
        node_id idWinner = vote_getWinner(&votemsg);

        printf("Next turn leader: %i \n", idWinner);

        // Control if our node is the winner
        if (idWinner == idNode)
        {
            turnLeader = true;
        }

        else
        {

            turnLeader = false;
            
            // Define the message variable
            message_t winmsg;

            // Initialize the message
            msg_init(&winmsg);

            // Set the id of the turn leader in the message
            msg_set_ids(&winmsg, idNode, idWinner);

            // Get ip address of the next node
            char* ipWinnerNode = iptab_getaddr(idWinner);
            // Connect to the next node
            int fdSocketWinnerNode = net_client_connection(ipWinnerNode);
            // Send the message to the next node
            if (write(fdSocketWinnerNode, &winmsg, sizeof(message_t)) <= 0)
            {
                perror("Error sending message to the winner node");
                exit(-1);
            }

            // Close the winner node socket
            close(fdSocketWinnerNode);

        }

        // Change state
        state = 2;
    }

    // Our node is not the first node of the table
    else
    {

        // Define the message variable
        votation_t votemsg;

        // Initialize the message
        vote_init(&votemsg);

        // Wait the message of the previous node
        int fdSocketClient = net_accept_client(fdSocket, NULL);

        // Read the message from the client
        if (read(fdSocketClient, &votemsg, sizeof(votation_t)) <= 0)
        {
            perror("Error reading message form last node");
            exit(-1);
        }

        // Close the previous node socket
        close(fdSocketClient);


        // Do the votation
        vote_do_votation(&votemsg);

        // Obtain the id of the next node
        node_id nextNodeId = iptab_get_next(idNode);
        // Get ip address of the next node
        char* ipNextNode = iptab_getaddr(nextNodeId);
        // Connect to the next node
        int fdSocketNextNode = net_client_connection(ipNextNode);
        // Send the message to the next node
        if (write(fdSocketNextNode, &votemsg, sizeof(votation_t)) <= 0)
        {
            perror("Error sending message to the next node");
            exit(-1);
        }

        // Close the socket
        close(fdSocketNextNode);


        // Close the the communication with the next node
        close(fdSocketClient);

        // Define the message variable
        message_t msg;

        // Initialize the message
        msg_init(&msg);

        // Wait the message of the previous node
        fdSocketClient = net_accept_client(fdSocket, NULL);

        // Read the message from the client
        if (read(fdSocketClient, &msg, sizeof(message_t)) <= 0)
        {
            perror("Error reading message form last node");
            exit(-1);
        }

        // Close the the communication with the next node
        close(fdSocketClient);


        // Get the id of the winner from the received message
        node_id idWinner = msg_get_turnLeader(&msg);

        printf("Next turn leader: %i \n", idWinner);
        fflush(stdout);

        // Control if we are the turn leader
        if (idWinner == idNode)
        {
            turnLeader = true;
            state = 2;
        }

        // Our node is not the turn leader
        else
        {
            printf("Turn %i\n", turnCount + 1);
            fflush(stdout);
            // Call the turn function
            turn(&msg);
        }
    }
}

void turn(message_t* msg)
{

    // Find the number of available nodes
    int availableNodes = iptab_len_av();

    // Initilize average variables
    struct timeval totalTimes[10];
    struct timeval flyTimes[10];

    // Start the loop
    for (int count = 0; count < 10; count++)
    {

        // Define the message variable
        message_t message;
        // Initialize the message
        msg_init(&message);

        printf("Iteration: %i \n", count + 1);
        fflush(stdout);

        // Control if we are the turn leader
        if (turnLeader == true)
        {
            if(count == 0) {
                printf("I'm turn leader \n");
                fflush(stdout);
            }

            // Define and initialize the timeval variables 
            //(Only the the last two because the first two will be overwritten)
            struct timeval firstSendTime;
            struct timeval totalComputationalTime;
            totalComputationalTime.tv_sec = 0;
            totalComputationalTime.tv_usec = 0;

            // Set the id of the turn leader in the message
            msg_set_ids(&message, idNode, idNode);

            // Start loop for set visited all the unavailable nodes
            for (int i = 0; i < lenghtTable; i++)
            {
                node_id tempNode = i;
                // Control if the i node of the table is available
                if (iptab_is_available(tempNode) == 0)
                {
                    // Set visited the node
                    msg_mark(&message, tempNode);
                }
            }

            // Set our node as visited
            msg_mark(&message, idNode);

          

            // Select randomly the next node
            int idNextNode = msg_rand(&message);

            // Save the first sending time
            firstSendTime = message.sent;

            // Get ip address of the next node
            char* ipNextNode = iptab_getaddr(idNextNode);
            // Connect to the next node
            int fdSocketNextNode = net_client_connection(ipNextNode);

            // Insert the sending time in the message
            msg_set_sent(&message);
            
            // Send the message to the next node
            if (write(fdSocketNextNode, &message, sizeof(message_t)) <= 0)
            {
                perror("Error sending message to the next node");
                exit(-1);
            }

            // Close the the communication with the next node
            close(fdSocketNextNode);


            for (int var = 0; var < availableNodes - 1; var++)
            {
                // Wait the message from a node
                int fdSocketClient = net_accept_client(fdSocket, NULL);

                // Read the message from the client
                if (read(fdSocketClient, &message, sizeof(message_t)) <= 0)
                {
                    perror("Error reading message from node");
                    exit(-1);
                }

                // Close the node socket
                close(fdSocketClient);

                //Initialize variable for last Receive
                struct timeval lastResult;
                gettimeofday(&lastResult, NULL);

              
                // Initialize the computational time of this node
                struct timeval computationalTime;
                computationalTime.tv_sec = 0;
                computationalTime.tv_usec = 0;

                // Calculate the computaional time of this node and sum with the other
                timersub(&message.sent, &message.recvd, &computationalTime);
                timeradd(&totalComputationalTime, &computationalTime, &totalComputationalTime);

                // If this is the last iteration compute total time and fly time
                if (var == availableNodes - 2)
                {
                    timersub(&lastResult, &firstSendTime, &totalTimes[count]);
                    timersub(&totalTimes[count], &totalComputationalTime, &flyTimes[count]);
                }
            }

            // If this is the last iteration compute
            if (count == 9)
            {
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

                // Compute the averages
                for (int var = 0; var < 10; var++)
                {
                    timeradd(&averageTotTime, &totalTimes[var], &averageTotTime);
                    timeradd(&averageFlyTime, &flyTimes[var], &averageFlyTime);
                }
                averageTotTime.tv_sec = (averageTotTime.tv_sec/10);
                averageTotTime.tv_usec = (averageTotTime.tv_usec/10);
                averageFlyTime.tv_sec = (averageFlyTime.tv_sec/10);
                averageFlyTime.tv_usec = (averageFlyTime.tv_usec/10);

                printf("Average tot time: sec: %ld usec: %ld \n", averageTotTime.tv_sec, averageTotTime.tv_usec);
                fflush(stdout);
                
                printf("Average fly time: sec: %ld usec: %ld \n", averageFlyTime.tv_sec, averageFlyTime.tv_usec);
                fflush(stdout);
                

                // Compute total bandwith
                float totBand = ((float)(8*sizeof(message_t)) / (averageTotTime.tv_sec + (averageTotTime.tv_usec/1000000.f)));
                float flyBand = ((float)(8*sizeof(message_t)) / (averageFlyTime.tv_sec + (averageFlyTime.tv_usec/1000000.f)));
                
                printf("Total band: %f \n", totBand);
                fflush(stdout);
                
                printf("Fly band: %f \n", flyBand);
                fflush(stdout);
                

                // Store results in the meessage
                stat_message_set_totBitrate(&stmsg, totBand);
                stat_message_set_flyBitrate(&stmsg, flyBand);

                // Obtain the ip of the Professor server
                char* ipProf = stat_get_RURZ_addr();

                // Connect to the Professor server
                int fdSocketProf = net_client_connection(ipProf);

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
                }
            }
        }

        // our node is not the turn leader
        else
        {

            if(count == 0) {
                printf("I'm not the turn leader \n");
                fflush(stdout);
                
            }

            // Initialize variable for last node
            bool lastNode = false;

            // Our node has not already received the message
            if (msg == NULL)
            {
                // Wait the message from a node
                int fdSocketClient = net_accept_client(fdSocket, NULL);

                // Read the message from the client
                if (read(fdSocketClient, &message, sizeof(message_t)) <= 0)
                {
                    perror("Error reading message from previous node");
                    
                    exit(-1);
                }

                // Close the socket node
                close(fdSocketClient);

            }

            // Our node has already received the message
            else
            {
                message = *msg;
                msg = NULL;
            }

            // Reset the receiving time
            msg_set_recv(&message);

            // Mark our node as visited
            msg_mark(&message, idNode);

            // Initilize the id of next node
            node_id idNextNode;

            // Control if all nodes are visited
            if (msg_all_visited(&message))
            {
                // Select the turn leader as next node
                idNextNode = message.turnLeader;
                lastNode = true;

                printf("All nodes are visited \n");
                
            }

            else
            {
                // Select randomly the next node
                idNextNode = msg_rand(&message);
            }

            // Set our id in the message
            msg_set_ids(&message, idNode, message.turnLeader);

            for(int j = 0; j < lenghtTable; j++) {
                int visited = msg_visited(&message, j);
                if(visited == 1) {
                   
                }
                else {
                    
                }
            }

            char* ipNextNode = iptab_getaddr(idNextNode);
            // Connect to the next node
            int fdSocketNextNode = net_client_connection(ipNextNode);
            // Set sending time n the message
            msg_set_sent(&message);
            // Send the message to the next node
            if (write(fdSocketNextNode, &message, sizeof(message_t)) <= 0)
            {
                perror("Error sending message to the next node");
                
                exit(-1);
            }

            // Close the the communication with the next node
            close(fdSocketNextNode);

           

            if (lastNode == false)
            {
                // Get ip address of the turn leader
                char* ipTurnLeader = iptab_getaddr(message.turnLeader);
                // Connect to the turn leader
                int fdSocketTurnLeader = net_client_connection(ipTurnLeader);
                // Send the message to the turn leader
                if (write(fdSocketTurnLeader, &message, sizeof(message_t)) <= 0)
                {
                    perror("Error sending message to the turn leader");
                    
                    exit(-1);
                }

                // Close the the communication with the next node
                close(fdSocketNextNode);

               
            }
        }
    }

    // Change state
    state = 1;
    turnCount++;
}

int main() {

    

    // Initialize the srand function
    srand(time(NULL));

    // initialize the server
    fdSocket = net_server_init();
    // Control if there is an error initilizing the socket
    if (fdSocket <= 0)
    {
        printf("Error initializing the server");
        
        exit(-1);
    }

    // Obtain the id of our node
    idNode = iptab_get_ID_of(ipAddr);

    printf("The id of this node is: %i \n", idNode);
    fflush(stdout);
   

    // Get lenght of the address table
    lenghtTable = iptab_len();

    for(int j = 0; j < lenghtTable; j++) {
        char* ip = iptab_getaddr(j);
       
    }

    // Set timeout time
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    while (turnCount < 10)
    {

        if (state == 0 && turnCount == 0)
        {
            printf("Handshake \n");
            fflush(stdout);
           

            handshake();
        }
        else if (state == 1)
        {
            printf("Vote \n");
            fflush(stdout);
            

            vote();
        }
        else if (state == 2)
        {
            printf("Turn %i\n", turnCount + 1);
            fflush(stdout);
            
        
            turn(NULL);
        }
    }

    // Close the socket server
    close(fdSocket);

	printf("\nMaster is exiting...");
    fflush(stdout);
    printf("\nReturn value: 0 \n\n");
    fflush(stdout);
    return 0;
}
