//PIQUET GIANLUCA 4206820
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
FILE *fLog; 
int turnCount = 0;
int lenghtTable = 0;
struct timeval timeout;

// Functions
void handshake();
void vote();
void turn(message_t* msg);

void handshake()
{
    // Define message variable
    handshake_t hshmsg;
    
    int firstNode = hsh_imfirst(ipAddr);

    if (firstNode == 1)
    {
	    printf("I'm the first node");
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
                // Obtain the info about the next node
                node_id nextNodeId = iptab_get_next(idNode + j);
                char* ipNextNode = iptab_getaddr(nextNodeId);
                // Connect to the next node
                int fdSocketNextNode = net_client_connection_timeout(ipNextNode, &timeout);

                // Control if it is available
                if(fdSocketNextNode > 0) {

                    printf("Node with id %i is available \n", nextNodeId);
                    fflush(stdout);

                    // Send the message to the next node
                    if (write(fdSocketNextNode, &hshmsg, sizeof(handshake_t)) <= 0)
                    {
                        perror("Error sending message to the next node");
                        fprintf(fLog, "Error sending message to the next node \n");
                        fflush(fLog);
                        exit(-1);
                    }

                    // Close the socket
                    close(fdSocketNextNode);

                    printf("Message sent to node: %s\n", ipNextNode);
                    fflush(stdout);

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
                fprintf(fLog, "Error reading message form last node \n");
                fflush(fLog);
                exit(-1);
            }

            // Close the last node socket
            close(fdSocketClient);

            printf("Message received \n");
            fflush(stdout);

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
                fprintf(fLog, "Error sending message to the next node \n");
                fflush(fLog);
                exit(-1);
            }

            // Close the socket
            close(fdSocketNextNode);

            printf("Message sent to node: %s\n", ipNextNode);
            fflush(stdout);

            // Wait the message of the last node
            fdSocketClient = net_accept_client(fdSocket, NULL);

            // Read the message from the client
            if (read(fdSocketClient, &hshmsg, sizeof(handshake_t)) <= 0)
            {
                perror("Error reading message form last node");
                fprintf(fLog, "Error reading message form last node \n");
                fflush(fLog);
                exit(-1);
            }

            // Close the last node socket
            close(fdSocketClient);

            printf("Message received \n");
            fflush(stdout);
        }

        else
        {
            printf("The version of the arpnet library is not updated!");
            fflush(stdout);
            fprintf(fLog, "The version of the arpnet library is not updated! \n");
            fflush(fLog);
            fprintf(fLog, "Process exiting... \n");
            fflush(fLog);

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
            fprintf(fLog, "Error reading message form last node \n");
            fflush(fLog);
            exit(-1);
        }

        // Close the previous node socket
        close(fdSocketClient);

        printf("Message received \n");
        fflush(stdout);

        // Control if our version of the libray is the correct one
        int correctVersion = hsh_check_availability(idNode, &hshmsg);

        printf("Check availability done! \n");
        fflush(stdout);

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
                    fprintf(fLog, "Error sending message to the next node \n");
                    fflush(fLog);
                    exit(-1);
                }

                // Close the socket
                close(fdSocketNextNode);

                printf("Message sent to node: %s\n", ipNextNode);
                fflush(stdout);

                break;
            }

            printf("Node with id %i is not available \n", nextNodeId);
            fflush(stdout);

        }

        if (correctVersion == 0)
        {
            printf("The version of the arpnet library is not updated!");
            fflush(stdout);
            fprintf(fLog, "The version of the arpnet library is not updated! \n");
            fflush(fLog);
            fprintf(fLog, "Process exiting... \n");
            fflush(fLog);

            exit(-1);
        }

        // Wait the message of the last node
        fdSocketClient = net_accept_client(fdSocket, NULL);

        // Read the message from the client
        if (read(fdSocketClient, &hshmsg, sizeof(handshake_t)) <= 0)
        {
            perror("Error reading message form last node");
            fprintf(fLog, "Error reading message form last node \n");
            fflush(fLog);
            exit(-1);
        }

        // Close the last node socket
        close(fdSocketClient);

        printf("Message received \n");
        fflush(stdout);

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
            fprintf(fLog, "Error sending message to the next node \n");
            fflush(fLog);
            exit(-1);
        }

        // Close the socket
        close(fdSocketNextNode);

        printf("Message sent to node: %s\n", ipNextNode);
        fflush(stdout);
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
            fprintf(fLog, "Error sending message to the next node \n");
            fflush(fLog);
            exit(-1);
        }

        // Close the socket
        close(fdSocketNextNode);

        printf("Message sent to node: %s\n", ipNextNode);
        fflush(stdout);

        // Wait the message of the last node
        int fdSocketClient = net_accept_client(fdSocket, NULL);

        // Read the message from the client
        if (read(fdSocketClient, &votemsg, sizeof(votation_t)) <= 0)
        {
            perror("Error reading message form last node");
            fprintf(fLog, "Error reading message form last node \n");
            fflush(fLog);
            exit(-1);
        }

        // Close the last node socket
        close(fdSocketClient);

        printf("Message received \n");
        fflush(stdout);

        /*for(int j = 0; j < lenghtTable; j++) {
            int available = iptab_is_available(j);
            if(available = 1) {
                printf("Node with id %i is available \n", j);
                fflush(stdout);
            }
            else {
                printf("Node with id %i is  not available \n", j);
                fflush(stdout);
            }
        }*/

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

            //printf("Id turn leader in the message: %i", winmsg.turnLeader);

            // Get ip address of the next node
            char* ipWinnerNode = iptab_getaddr(idWinner);
            // Connect to the next node
            int fdSocketWinnerNode = net_client_connection(ipWinnerNode);
            // Send the message to the next node
            if (write(fdSocketWinnerNode, &winmsg, sizeof(message_t)) <= 0)
            {
                perror("Error sending message to the winner node");
                fprintf(fLog, "Error sending message to the winner node \n");
                fflush(fLog);
                exit(-1);
            }

            // Close the winner node socket
            close(fdSocketWinnerNode);

            printf("Message sent to node: %s\n", ipWinnerNode);
            fflush(stdout);
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
            fprintf(fLog, "Error reading message form last node \n");
            fflush(fLog);
            exit(-1);
        }

        // Close the previous node socket
        close(fdSocketClient);

        printf("Message received \n");
        fflush(stdout);

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
            fprintf(fLog, "Error sending message to the next node \n");
            fflush(fLog);
            exit(-1);
        }

        // Close the socket
        close(fdSocketNextNode);

        printf("Message sent to node: %s\n", ipNextNode);
        fflush(stdout);

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
            fprintf(fLog, "Error reading message form last node \n");
            fflush(fLog);
            exit(-1);
        }

        // Close the the communication with the next node
        close(fdSocketClient);

        printf("Message received from node with id: %i \n", msg.id);
        fflush(stdout);

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
    struct timeval computationalTime;
    computationalTime.tv_sec = 0;
    computationalTime.tv_usec = 0;

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

            /*for(int j = 0; j < lenghtTable; j++) {
                int visited = msg_visited(&message, j);
                if(visited == 1) {
                    printf("Node with id %i is visited \n", j);
                    fflush(stdout);
                }
                else {
                    printf("Node with id %i is  not visited \n", j);
                    fflush(stdout);
                }
            }*/

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
                else {

                }
            }

            // Set our node as visited
            msg_mark(&message, idNode);

            /*for(int j = 0; j < lenghtTable; j++) {
                int visited = msg_visited(&message, j);
                if(visited == 1) {
                    printf("Node with id %i is visited \n", j);
                    fflush(stdout);
                }
                else {
                    printf("Node with id %i is  not visited \n", j);
                    fflush(stdout);
                }
            }*/

            // Select randomly the next node
            int idNextNode = msg_rand(&message);

            // Insert the sending time in the message
            msg_set_sent(&message);
            // Save the first sending time
            firstSendTime = msg_get_sent(&message);

            // Get ip address of the next node
            char* ipNextNode = iptab_getaddr(idNextNode);
            // Connect to the next node
            int fdSocketNextNode = net_client_connection(ipNextNode);
            // Send the message to the next node
            
            if (write(fdSocketNextNode, &message, sizeof(message_t)) <= 0)
            {
                perror("Error sending message to the next node");
                fprintf(fLog, "Error sending message to the next node \n");
                fflush(fLog);
                exit(-1);
            }

            // Close the the communication with the next node
            close(fdSocketNextNode);

            printf("Message sent to node: %s\n", ipNextNode);
            fflush(stdout);

            for (int var = 0; var < availableNodes - 1; var++)
            {
                // Wait the message from a node
                int fdSocketClient = net_accept_client(fdSocket, NULL);

                // Read the message from the client
                if (read(fdSocketClient, &message, sizeof(message_t)) <= 0)
                {
                    perror("Error reading message from node");
                    fprintf(fLog, "Error reading message from node \n");
                    fflush(fLog);
                    exit(-1);
                }

                // Close the node socket
                close(fdSocketClient);

                printf("Message received from node with id: %i \n", message.id);
                fflush(stdout);

                // Initialize the computational time of this node

                // Calculate the computaional time of this node and sum with the other
                struct timeval time_R;
                struct timeval time_S;
                time_R=msg_get_recv(&message);
                time_S=msg_get_sent(&message);
                timersub(&time_S, &time_R, &computationalTime);
                timeradd(&totalComputationalTime, &computationalTime, &totalComputationalTime);

                // If this is the last iteration compute total time and fly time
                if (var == availableNodes - 2)
                {   
                    struct timeval lastReceiving;
                    gettimeofday(&lastReceiving,NULL);
                    struct timeval tot_t;
                    timersub(&lastReceiving,&firstSendTime,&tot_t);
                    totalTimes[count]=tot_t;
                    timersub(&(totalTimes[count]), &totalComputationalTime, &(flyTimes[count]));
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
                float totBand = ((float)(8*sizeof(message_t)) / ((averageTotTime.tv_sec) + (averageTotTime.tv_usec/1000000.f)));
                float flyBand = ((float)(8*sizeof(message_t)) / ((averageFlyTime.tv_sec) + (averageFlyTime.tv_usec/1000000.f)));

                totalComputationalTime.tv_usec=0;
                totalComputationalTime.tv_sec=0;
                
                printf("Total band: %f \n", totBand);
                fflush(stdout);
                printf("Fly band: %f \n", flyBand);
                fflush(stdout);

                /*// Store results in the meessage
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
                        fprintf(fLog, "Error sending message to the server of the Professor \n");
                        fflush(fLog);
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
                    fprintf(fLog, "Error reading message from previous node \n");
                    fflush(fLog);
                    exit(-1);
                }

                // Close the socket node
                close(fdSocketClient);

                printf("Message received from node with id: %i \n", message.id);
                fflush(stdout);
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
            }

            else
            {
                // Select randomly the next node
                idNextNode = msg_rand(&message);
            }

            // Set our id in the message
            msg_set_ids(&message, idNode, message.turnLeader);

            /*for(int j = 0; j < lenghtTable; j++) {
                int visited = msg_visited(&message, j);
                if(visited == 1) {
                    printf("Node with id %i is visited \n", j);
                    fflush(stdout);
                }
                else {
                    printf("Node with id %i is  not visited \n", j);
                    fflush(stdout);
                }
            }*/

            // Set sending time n the message
            char* ipNextNode = iptab_getaddr(idNextNode);
            // Connect to the next node
            int fdSocketNextNode = net_client_connection(ipNextNode);
            
            msg_set_sent(&message);
            // Send the message to the next node
            if (write(fdSocketNextNode, &message, sizeof(message_t)) <= 0)
            {
                perror("Error sending message to the next node");
                fprintf(fLog, "Error sending message to the next node \n");
                fflush(fLog);
                exit(-1);
            }

            // Close the the communication with the next node
            close(fdSocketNextNode);

            printf("Message sent to node: %s\n", ipNextNode);
            fflush(stdout);

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
                    fprintf(fLog, "Error sending message to the turn leader \n");
                    fflush(fLog);
                    exit(-1);
                }

                // Close the the communication with the next node
                close(fdSocketNextNode);

                printf("Message sent to turn leader node: %s\n", ipTurnLeader);
                fflush(stdout);
            }
        }
    }

    // Change state
    state = 1;
    turnCount++;
}

int main() {

    // Initialize variables
    FILE *fLog; // Log file

    // Create the log file
    fLog = fopen("assignment3.log", "w");

    // Control if there is an error in function fopen
    if (fLog == NULL)
    {
        perror("Creating log file");
        exit(-1);
    }

    // Initialize the srand function
    srand(time(NULL));

    // initialize the server
    fdSocket = net_server_init();
    // Control if there is an error initilizing the socket
    if (fdSocket <= 0)
    {
        printf("Error initializing the server");
        fflush(stdout);
        fprintf(fLog, "Error initializing the server \n");
        fflush(fLog);
        fprintf(fLog, "Process exiting... \n");
        fflush(fLog);
        exit(-1);
    }

    // Obtain the id of our node
    idNode = iptab_get_ID_of(ipAddr);

    printf("The id of this node is: %i \n", idNode);
    fflush(stdout);

    // Get lenght of the address table
    lenghtTable = iptab_len();

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

    fprintf(fLog, "Master is exiting... \n");
    fflush(fLog);
    fprintf(fLog, "Master exited with return value: 0 \n");
    fflush(fLog);

    // Close log fie and control if there is an error in function fclose
    if (fclose(fLog) < 0)
    {
        perror("Close log file");
        exit(-1);
    }

    printf("\nMaster is exiting...");
    fflush(stdout);
    printf("\nReturn value: 0 \n\n");
    fflush(stdout);
    return 0;
}
