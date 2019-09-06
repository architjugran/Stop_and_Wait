// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <signal.h> 
#define MAXLINE 9000 
#define max_clients 30
int sockfd,client_socket[max_clients];


void sigintHandler(int sig_num)   // handler when Ctrl+c is pressed
{ 
    /* Reset handler to catch SIGINT next time. 
       Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler); 
    close(sockfd);
    for(int i=0;i<max_clients;i++)
    {
        if(client_socket[i]!=0)
            close(client_socket[i]);
    }
    printf("server closed \n");
    fflush(stdout); 
    exit(0);
} 

int checker(int n)          //    check whether socket is closed or not
{
    if(n>0)
        return 1;
    else
        return 0;
}

//check if the data recieved is corrupt or not through CRC.
int not_corrupt(int n,char received[MAXLINE])   //  check whether the received data is not corrupted ie it matches with CRC
{
    received[n]='\0';
    int len = strlen(received);
    char divisor[10] = "100000111";
    int divlen= strlen(divisor);
    
    //Divison starts here
    char copy[1000];
    strcpy(copy,received);
    for(int i=0;i<=len-divlen;i++)
    {
        if(copy[i]=='0')
            continue;
        for(int j=i;j<i+divlen;j++)
        {
            copy[j]=((copy[j]-48)^(divisor[j-i]-48))+48;
        }
    }

	// Copy[i] contains the remainder of division. So, if any of the remainder bits is 0 we set 
	// flagg to 1 and return 0 i.e. corrupt data    
    int flagg=0;
    for(int i=len-1;i>=0;i--)
    {
        if(copy[i]=='1')
        {
            flagg=1;
            break;
        }
    }
    if(flagg)
        return 0;
    else
        return 1;
}

int expected_sequence(char *buffer,int n,int expec)   // check wether the data received contains expeced sequence number only
{
    if(n<9)
        return 0;
    if(buffer[n-9]-'0'==expec) //matching the sequence number bit with the expected value
        return 1;
    return 0;
}

//generate and append the CRC to the string
char* CRC(char input[8000])    // calculates and append CRC at the last of the data
{
    int len = strlen(input);
    	// divisor is given polynomial x^8+x^2+x+1
    char divisor[10] = "100000111";
    int divlen= strlen(divisor);
    	//Appending divisorlength -1 '0' bits to input string
    for(int i=len;i<len+divlen-1;i++)
        input[i]='0';
    input[len+divlen-1]='\0';
    len+=divlen-1;

    //Divison starts here
    char copy[1000];
    strcpy(copy,input);
    for(int i=0;i<=len-divlen;i++)
    {
        if(copy[i]=='0')
            continue;
        for(int j=i;j<i+divlen;j++)
        {
            copy[j]=((copy[j]-48)^(divisor[j-i]-48))+48;//Cyclically taking xor of values
        }
    }
    
    	// Replacing the 0 bits at end of input with the remainder to generate the final codeword
    for(int i=len-1;i>=len-divlen+1;i--)
    {
        input[i]=copy[i];
    }

    return input;
}



void print_data(int n,char buffer[MAXLINE])    // convert data from binary to characters and print them
{
    char str3[8000]="";
    for(int i=0;i<n-9;i=i+8)
    {
    	// Taking 8 characters at a time and converting them to ASCII binary
        int ans=0;
        for(int j=i;j<i+8;j++)
        {
            ans=2*ans+(buffer[j]-'0');
        }
        char c=ans;
        char str2[2];
        str2[0]=c;
        str2[1]='\0';
        // Concatenating the cnverted character to string
        strcat(str3,str2);
    }
    printf("%s\n",str3 );
    fflush(stdout); 
}


// Driver code 
int main(int argc, char** argv) { 
		//Checking proper entry of input
    if(argc<2)
    {
        printf("Enter port number \n");
        exit(0);
    }
    int PORT=atoi(argv[1]);
    signal(SIGINT, sigintHandler);// signal handler for CTrl+c
    char buffer[MAXLINE]; 
    struct sockaddr_in servaddr; 
    printf("Please enter error probability\n");
    fflush(stdout); 
    float ber;
    scanf("%f",&ber);
    char junk;
    scanf("%c",&junk);
    int prob = ber*100;// probability is taken as 100*entered bit error rate
    // Filling client information
    int expec[max_clients];
    for(int i=0;i<max_clients;i++)
        expec[i]=0;
    int activity,max_sd;
    fd_set readfds;
    for(int i=0;i<max_clients;i++)
        client_socket[i]=0;
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {   // makes a socket
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,      // bind fd to the socket port number
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }   
    if (listen(sockfd, 3) < 0)  // listen failure
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 

    int len, n,new_socket,addrlen=sizeof(servaddr),sd;//Server now starts Waiting for connections 
    printf("Waiting for connections ..."); 
    fflush(stdout); 
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        max_sd = sockfd;
        for ( int i = 0 ; i < max_clients ; i++)   
        {   
            //socket descriptor  
            sd = client_socket[i];   
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET( sd , &readfds);  // adding socket to readfds list  
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL); // we will wait here until either data comes in any socket   
        if ((activity < 0))   
        {   
            printf("select error"); 
            fflush(stdout);   
        } 



        if (FD_ISSET(sockfd, &readfds))   // if sockfd socket causes the interrupt
        {   
        	// new socket is accept and added to the lis of client_socket
            if ((new_socket = accept(sockfd,  
                    (struct sockaddr *)&servaddr, (socklen_t*)&addrlen))<0)   
            {   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }   
             
            //inform user of socket number - used in send and receive commands  
            printf("New connection , socket fd is %d , ip is : %s , port : %d  \n",
                     new_socket , inet_ntoa(servaddr.sin_addr) , ntohs 
                  (servaddr.sin_port));  
            fflush(stdout);  
            //add new socket to array of sockets  
            for (int i = 0; i < max_clients; i++)   
            {   
                //if position is empty  
                if( client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;   
                    printf("Adding to list of sockets as %d\n" , i);   
                    fflush(stdout);          
                    break;   
                }   
            } 

        }
        for (int i = 0; i < max_clients; i++)     // client_soket[i] causes the interrupt
        {   
            sd = client_socket[i];   
                 
            if (FD_ISSET( sd , &readfds))   
            {   
                //Check if it was for closing , and also read the  
                //incoming message  
                n=read( sd , buffer, MAXLINE);
                if (n == 0)   
                {   
                    //Somebody disconnected , get his details and print  
                    getpeername(sd , (struct sockaddr*)&servaddr ,
                        (socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(servaddr.sin_addr) , ntohs(servaddr.sin_port));   
                    fflush(stdout);          
                    //Close the socket and mark as 0 in list for reuse  
                    close( sd );   
                    client_socket[i] = 0;
                    expec[i]=0;   
                }   
                     
                else 
                {   
                    //set the string terminating NULL byte on the end  
                    //of the data read 
                    // if everything is correct i.e recieved data and expected sequence and not_corrupted
                    if(checker(n) && not_corrupt(n,buffer) && expected_sequence(buffer,n,expec[i]))
                    {
                        getpeername(sd , (struct sockaddr*)&servaddr ,
                        (socklen_t*)&addrlen);
                        printf("HOST Message ip: %s Data: ",inet_ntoa(servaddr.sin_addr));
                        fflush(stdout); 
                        print_data(n,buffer);
                        char str3[8000]; 
                        if(expec[i]==0)
                            strcpy(str3,"00"); // bits : ACK + sequence number
                        else
                            strcpy(str3,"01");
                        char *hello=CRC(str3);
                        int rando = rand()%100 + 1;// generating a random number between 1 and 100
            							// if random number > probability , only then we will generate error in input keyword
                        printf("%d\n",rando);
                        fflush(stdout); 
                        if(rando <= prob)          // for giving external error
                        {
                            char origlast = hello[strlen(hello)-1];
                            hello[strlen(hello)-1]=((origlast-48)^1)+48;
                        } 
                        send(sd , hello , strlen(hello) , 0 );
                        expec[i]=1-expec[i];  
                    }
                    else   // send nack
                    {
                        char str3[8000];
                        if(expec[i]==0)
                            strcpy(str3,"10");  // bits : NACK + sequence number

                        else
                            strcpy(str3,"11");
                        char *hello=CRC(str3);  
                        int rando = rand()%100 + 1;
                        printf("%d\n",rando);
                        fflush(stdout); 
                        if(rando <= prob)    // for giving external error
                        {
                            char origlast = hello[strlen(hello)-1];
                            hello[strlen(hello)-1]=((origlast-48)^1)+48;
                        }         
                        send(sd , hello , strlen(hello) , 0 );
                    } 
                }   
            }   
        }   
    }
    return 0; 
}