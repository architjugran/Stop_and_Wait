// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h> 
#define MAXLINE 9000 
  
int sockfd;

// A function to check if n is greater than 0
int checker(int n)
{
    if(n>0)
        return 1;
    else
        return 0;
}

//check if the data recieved is corrupt or not through CRC.
int not_corrupt(int n,char received[MAXLINE]) // received data is in char received[]
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
			copy[j]=((copy[j]-48)^(divisor[j-i]-48))+48; // Taking xor of all bits 
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

//this function checks if sequence number of ack is correct. If last is zero, we expect one and vice versa. 
int correct_acknowledgement(char *buffer,int seq)
{
    if(buffer[0]=='0' && buffer[1]-'0'==seq)
    	return 1;
    if(buffer[0]=='1' && buffer[1]-'0'==(1-seq))
    	return 1;
    return 0;
}

//close client properly on pressing Ctrl+C, freeing the socket too. 
void sigintHandler(int sig_num) 
{ 
    /* Reset handler to catch SIGINT next time. 
       Refer http://exitn.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler); 
    close(sockfd);
    printf("Client closed\n");
    fflush(stdout); 
    exit(0);
} 

//convert string to bits to send the bits directly instead of string as data
char* bits(char str[1000])
{
	int len = strlen(str);
	char *str2=malloc(sizeof(char)*8000); 
	for(int i=0;i<len;i++)
	{
		// Basically, converting each character's ASCII number to binary and concatenating in result
		int a=str[i]; 
		char str3[9]="";
		for(int j=0;j<8;j++)
		{
			if(a%2==0)
				strcat(str3,"0");
			else
				strcat(str3,"1");
			a=a/2;
		}
		str3[8]='\0';

		//Reversing the string to obtain correct binary sequence
	    for(int j=0;j<4;j++)
	    {
	    	char a=str3[j];
	    	str3[j]=str3[7-j];
	    	str3[7-j]=a;
	    }
	    //Concatenating characters binary to final string str3
	    strcat(str2,str3);
	}
	return str2;
}

//generate and append the CRC to the string
char* CRC(char input[8000])
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


// Driver code 
int main(int argc,char **argv) { 

	//Checking proper entry of input
    if(argc<3)
    {
        printf("Enter IP address and port number \n");
        exit(0);
    }
    int PORT=atoi(argv[2]);
    printf("%d \n",PORT);
    int seq=1;
    srand(time(0)); //  seed of random numbers 
    char buffer[MAXLINE]; 
    struct sockaddr_in     servaddr; 
    signal(SIGINT, sigintHandler); // signal handler for CTrl+c
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    memset(&servaddr, 0, sizeof(servaddr)); 
    float ber;
    printf("Please enter error probability\n");
    scanf("%f",&ber);
    char junk;
    scanf("%c",&junk);
    int prob = ber*100; // probability is taken as 100*entered bit error rate
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr)<=0)  //Invalid address/ Address not supported
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) //Connection Failed
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 
    int n, len; 
    while(1)
    {
        char str[1000]; 
        seq=1-seq; // initially seq=0
        scanf("%[^\n]%*c", str);  
        int flaggg=0;
        char* str2=bits(str);
        if(seq==0) // adding current sequence number to input
        	strcat(str2,"0");
        else
        	strcat(str2,"1");
        fflush(stdout);
        char *str4;
        str4=CRC(str2); // adding CRC to input
        char origlast = str4[strlen(str4)-1];
        while(1) // When NACK happens or timeout occurs, data is retransmitted with timer value reset
        {
            int rando = rand()%100 + 1; // generating a random number between 1 and 100
            							// if random number > probability , only then we will generate error in input keyword
            printf("%d\n",rando);
             fflush(stdout); 
            if(rando <= prob)
            {
                str4[strlen(str4)-1]=((origlast-48)^1)+48;
            } 
            else
                str4[strlen(str4)-1]=origlast;
            send(sockfd,str4,strlen(str4),0);
            clock_t before = clock(); // original value of time
            clock_t last=before;
            while(1) // When corrupt data is received from server, this loop is executed again
            {
            	// Timer value is updated using before value and current clock value = last
                float time_left=((last-before) * 1000 / CLOCKS_PER_SEC);
                time_left=999000-(time_left*1000);
                if(time_left<=0)
                {
                	// if timeout break
                    flaggg=0;
                    break;
                }

                // Declaring rfds list
                fd_set rfds;
                struct timeval tv; 
                int retval;
                FD_ZERO(&rfds); // initalize rfds as empty
                FD_SET(sockfd,&rfds); // insert sockfd socket into rfds
                tv.tv_sec=0; 
                tv.tv_usec=time_left; // remaining timer value inserted in rfds list
                fflush(stdout);
                retval=select(sockfd+1,&rfds,NULL,NULL,&tv); // we will wait here until either data comes in any socket or timeout
                fflush(stdout);
                if (retval == -1) // error in select
                {
                    perror("select()");
                    exit(EXIT_FAILURE); 
                }
                else if (retval==0) // No data within one second
                {
                    printf("No data within one second.\n"); 
                    fflush(stdout); 
                    flaggg=0;
                    break;
                }
                else
                {
                    int z=sleep(0.1); 
                    n=read(sockfd,buffer,MAXLINE);
                    if(checker(n)) // checking if connection closed by server or not
                    { 
                        if(not_corrupt(n,buffer)) // checking if data received from server is corrupted or not
                        {
                            if(correct_acknowledgement(buffer,seq)) // checking if ack is correct or not
                            { // then break into outermost loop and take next input
                                flaggg=1;
                                break;
                            }
                            else
                            {
                                printf("NACK received \n"); // nack received, wait into the 2nd loop
                                fflush(stdout); 
                                flaggg=0;
                                break;
                            }
                        }
                        else
                        {
                            printf("Corrupt data recieved\n");// go into 3rd loop from outside from here
                            fflush(stdout); 
                        }
                    }
                    else
                    {
                        printf("Server closed \n"); // When server has closed connection , just terminate the client too
                        fflush(stdout); 
                        close(sockfd);
                        fflush(stdout); 
                        exit(0);
                    }
                }
                last=clock(); // updating the last value to update the time_left value at top of loop and check for timeout
            }
            if(flaggg==1) 
                break;
        }
        printf("Message sent and recieved \n"); // inside outermost loop , successful delivery, then take next input
        fflush(stdout); 
    }
  
    close(sockfd); 
    return 0; 
} 
