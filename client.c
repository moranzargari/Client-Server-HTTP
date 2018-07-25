
//--------------------------------------Libraries---------------------------------------------//
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>

//--------------------------------------defines----------------------------------------------//
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define HEAD "HEAD"
#define GET "GET"
#define RESPONSE_SIZE 4069
#define TIMEBUF_SIZE 128
#define case_0 0
#define case_1 1
#define case_2 2
#define case_3 3
#define case_4 4
#define case_5 5
#define FAIL -1
#define SUCCESS 0
//--------------------------------------structs----------------------------------------------//
typedef struct{

	char *req;
	char *path;
	char *host;
	int port;
	int sizeofport;
	int sizeofreq;
}request;
//--------------------------------------declaration of functions---------------------------------------------//
void killRequest(request *r,int flag);
char* Time(char *timestr);
void wrongCommand(request *r,int flag);
int chkString(char *str,int flag);
void Url(char *url,request **r);
char* modifyTime(int* timearg);
int findPort(char* host,int *sizeofport);
char* createRequest(request *r,char *timebuf);
char* input(char* argv[],int argc,request *r);
void urlExists(int argc,char *argv[]);
void error(char *err,request *r,int flag,char* httpreq);
void chkFlags(int flgH,int flgUrl,int flgD,int curcase,request *r);
int getSizeRequest(request *r,char *timebuf);
void freeAll(request *r,char* httpreq);
void connectToServer(request *r,char* httpreq);
//==============================================================================MAIN===============================================================================================================//
int main(int argc,char *argv[])
{
	if(argc>5 || argc<2)//wrong amount of arguments
		wrongCommand(NULL,-1);
	urlExists(argc,argv);//check if url is exists
	request *r=(request*)calloc(1,sizeof(request));//this struct will hold arguments of the request
	if(r==NULL)
		error("calloc",r,case_0,NULL);
	char* httpreq= input(argv,argc,r);//create the request.
	connectToServer(r,httpreq);

freeAll(r,httpreq);
return SUCCESS;
}
//=======================================================================================END OF MAIN===================================================================================================//

//----------------------------------------------------------FUNCTIONS-----------------------------------------------------------------//

//--------------------------------------function connectToServer--------------------------------------//

void connectToServer(request *r,char* httpreq)//this function handle the connection to the requested web server from the input.
{
	int sockfd,rc,received = 0;
	struct hostent *server;
	struct sockaddr_in serv_addr;
	unsigned response[RESPONSE_SIZE];
	
	if((sockfd=socket(PF_INET,SOCK_STREAM,0))==FAIL)//creating new socket.
		error("socket",r,case_1,httpreq);

	server = gethostbyname(r->host);//using DNS to convert the host name to an IP
	if(server==NULL)
	{
		close(sockfd);
		freeAll(r,httpreq);
		herror("gethostbyname");
		exit(1);
	}

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(r->port);//add the correct port
	bcopy((char*)server->h_addr,(char*) &serv_addr.sin_addr.s_addr, server->h_length);//insert the IP address to the struct variable

	if((connect(sockfd,(const struct sockaddr*)&serv_addr, sizeof(serv_addr))==FAIL))//connecting to the web server using sockfd
	{
		close(sockfd);
		error("connect",r,case_1,httpreq);
	}

	printf("HTTP request=\n%s\nLEN=%d\n",httpreq,r->sizeofreq); 
	if(write(sockfd,httpreq,strlen(httpreq)+1)!=(strlen(httpreq)+1))//send request to the server
	{
		close(sockfd);
		error("write",r,case_1,httpreq);
	}
	memset(response,'\0',RESPONSE_SIZE);
	do {//reading server response
		
		rc = read(sockfd,response,sizeof(response));
		if (rc < 0)
		{
			close(sockfd);
		   	error("read",r,case_1,httpreq);
		}
		else if(rc>0)
		{
			received+=rc;//in each iteration add the amount of bytes that received so far.
			if(write(1,response,rc)!=rc)//write response bytes to stdout 
			{
				close(sockfd);
				error("write",r,case_1,httpreq);
			}
			memset(response,'\0',RESPONSE_SIZE);
		}
		else
		    break;
	    } while (1);
 
    printf("\n Total received response bytes:%d\n",received);
close(sockfd);
}

//--------------------------------------function urlExists--------------------------------------//

void urlExists(int argc,char *argv[])//this method check the entire input to avoid non url input.
{
	int i,size=0;
    for(i=0;i<argc;i++)//calculate each string inside argv size.
        size+= strlen(argv[i]);
	i=0;
	char *tmp=(char*)calloc(size+1,sizeof(char));
	if(tmp==NULL)
	{
		perror("calloc");
		exit(1);
	}
	while(i<argc)//concatnate all arguments to one string
	{
		strcat(tmp,argv[i]);
		i++;
	}
	if(strstr(tmp,"http://")==NULL)
	{
		free(tmp);
		wrongCommand(NULL,-1);
	}
free(tmp);
}
//--------------------------------------function input---------------------------------------------//

char* input(char* argv[],int argc,request *r)//this method is takeing care of the input ,saparate each part and return the request.
{
	int i=1,flgH=-1,flgUrl=-1,flgD=-1;
	char *timebuf=NULL;

	r->req=(char*)calloc((strlen(HEAD)+1),sizeof(char));
	if(r->req==NULL)
		error("calloc",r,case_4,NULL);
	
	while(i<argc)
	{
		if(strcmp(argv[i],"-h")==0)//in case the input is "-h"
		{
			chkFlags(flgH,flgUrl,flgD,case_0,r);
			strcpy(r->req,HEAD);
			flgH=1;
		}
		else if(strcmp(argv[i],"-d")==0)//in case the input is "-d"
		{
			chkFlags(flgH,flgUrl,flgD,case_1,r);
			if(flgH!=1)
				strcpy(r->req,GET);
			if(i+1!=argc)	
				i++;// check if the next input is type of time.
			char timestr[strlen(argv[i])];
			strcpy(timestr,argv[i]);
			timebuf=Time(timestr);
			if(timebuf==NULL && flgUrl==1)
				wrongCommand(r,case_1);
			else if(timebuf==NULL && flgUrl!=1)
				wrongCommand(r,case_4);
			flgD=1;
		}//otherwise.
		else
		{
			chkFlags(flgH,flgUrl,flgD,case_2,r);
			if(flgH!=1)
				strcpy(r->req,GET);
			char url[strlen(argv[i])];
			strcpy(url,argv[i]);
			Url(url,&r);
			flgUrl=1;	
		}
		i++;
	}
return createRequest(r,timebuf);
}
//--------------------------------------function Time---------------------------------------------//

char* Time(char *timestr)//this method check if the time input is valid ,return the calculated time on failure return NULL.
{
	if(chkString(timestr,case_2)==FAIL || chkString(timestr,case_3)==FAIL)
		return NULL;

	int count=0,*timearg;
	char *temp=timestr;
	timearg = calloc(3,sizeof(int));
	if(timearg==NULL)
	{
		perror("calloc");
		return NULL;
	}
	temp= strtok(timestr,":");
	while(temp!=NULL)
	{
		if(strcmp(temp,"")==0 || chkString(temp,case_1)==FAIL)
		{
			free(timearg);
			return NULL;
		}
		timearg[count]=atoi(temp);
		temp = strtok(NULL,":");
		count++;
	}
return modifyTime(timearg);
}
//--------------------------------------function Url---------------------------------------------//

void Url(char *url,request **r)//this method check if the url is valid and keep the host port and path.
{
	if(chkString(url,case_4)==FAIL)
		wrongCommand((*r),case_4);
	char *token,*rest;

	strtok_r(url, "/",&rest);
	rest++;
	if(chkString(rest,case_5)==FAIL)
		wrongCommand((*r),case_4);
	
	(*r)->host= strtok_r(rest,"/",&rest);
	(*r)->port = findPort((*r)->host,&(*r)->sizeofport);
	if((*r)->port==FAIL)
		wrongCommand((*r),case_4);

	token=strtok((*r)->host,":");
	(*r)->host = (char*) calloc((strlen(token)+1),sizeof(char));
	if((*r)->host==NULL)
		error("calloc",(*r),case_4,NULL);
	
	strcpy((*r)->host,token);
	(*r)->path = (char*) calloc((strlen(rest)+1),sizeof(char));
	if((*r)->path==NULL)
		error("calloc",(*r),case_3,NULL);
	strcpy((*r)->path,rest);
}
//--------------------------------------function createRequest---------------------------------------------//

char* createRequest(request *r,char *timebuf)//this method concatenate strings and return the correct request format.
{
	int size=getSizeRequest(r,timebuf);
	char *str=(char*)calloc(size,sizeof(char));
	if(str==NULL)
		error("calloc",r,case_1,NULL);
	
	sprintf(str,"%s /%s HTTP/1.1\r\nHost: %s",r->req,r->path,r->host);
	if(r->port==80)
		strcat(str,"\r\n");
	else
	{
		char tmp[r->sizeofport];
		sprintf(tmp,":%d\r\n",r->port);
		strcat(str,tmp);
	}
	if(timebuf!=NULL)
	{
		strcat(str,"Connection: Close\r\nIf-Modified-Since: ");
		strcat(str,timebuf);
	}
	else
		strcat(str,"Connection: Close");
	strcat(str,"\r\n\r\n");
	r->sizeofreq=strlen(str);
free(timebuf);
return str;
}
//--------------------------------------function modifyTime---------------------------------------------//

char* modifyTime(int* timearg)//this method calculate the time since it was modified.
{
	char *temp=(char*)calloc(TIMEBUF_SIZE,sizeof(char));
	if(temp==NULL)
	{
		perror("calloc");
		return NULL;
	}
	time_t now;
	char timebuf[TIMEBUF_SIZE];
	now = time(NULL);
	now=now-(timearg[case_0]*24*3600+timearg[case_1]*3600+timearg[case_2]*60);
	strftime(timebuf,sizeof(timebuf), RFC1123FMT, gmtime(&now));
	strcpy(temp,timebuf);
	free(timearg);
return temp;
}
//--------------------------------------function findPort---------------------------------------------//

int findPort(char* host,int *sizeofport)//this function extract the port from str and return's the correct port.
{
	char *tmpport;
	if(strstr(host,":")!=NULL)
	{
		tmpport = strchr(host,':');
		tmpport++;
		if(chkString(tmpport,case_1)==FAIL)
			return FAIL;
		*sizeofport=strlen(tmpport);
	     return atoi(tmpport);
	}
	else
		return 80;
}
//--------------------------------------function getSizeRequest---------------------------------------------//

int getSizeRequest(request *r,char *timebuf)//this function return current size of the request.
{
	int count=0;
	count+=strlen(r->host);
	count+=strlen(r->path);
	count+=strlen(r->req);
	count+=sizeof(r->port);
	if(timebuf!=NULL)
	{
		count+=strlen(timebuf);
		count+=72;
	}
	else
		count+=50;
return count;
}
//--------------------------------------function chkString---------------------------------------------//

int chkString(char *str,int flag)//this method taking care of all strings failures.
{
	int i;
	char temp[strlen(str)];
	switch(flag)
	{
		case case_1:
			if(str[0]=='-')
				i=1;
			else
				i=0;
			for(i;i<strlen(str);i++)
			{
				if(!isdigit(str[i]))
					return FAIL;
			}
			break;
		case case_2:
			strcpy(temp,str);
			char *run=temp;
			int count=0;
			while(*run) if (*run++ == ':') ++count;
			if(count!=2)
				return FAIL;
			break;	
		case case_3:
			if(strstr(str, "::")!=NULL)
				return FAIL;
			break;
		case case_4:
			if(strstr(str,"http://")==NULL)
				return FAIL;
			if(memcmp(str,"http://",7)!=0)
				return FAIL;
			if(strstr(str,"http:///")!=NULL)
				return FAIL;
			break;
		case case_5:
			if(strstr(str,"/")==NULL)
				return FAIL;
			if(strstr(str,":/")!=NULL)
				return FAIL;
			break;	
	}
return SUCCESS;
}
//--------------------------------------function chkFlags---------------------------------------------//

void chkFlags(int flgH,int flgUrl,int flgD,int curcase,request *r)//this method handle's wrong input situations.
{
	switch(curcase)	
	{
		case case_0:
			if(flgH==1 && flgUrl==1)
				wrongCommand(r,case_1);
			else if(flgH==1)
				wrongCommand(r,case_4);
			break;
		case case_1:
			if(flgD==1 && flgUrl==1)
				wrongCommand(r,case_1);
			else if(flgD==1)
				wrongCommand(r,case_4);
			break;
		case case_2:
			if(flgUrl==1)
				wrongCommand(r,case_1);
			break;
	}
}
//--------------------------------------function killRequest---------------------------------------------//

void killRequest(request *r,int flag)//this method free the current allocated memory so far.
{
	switch(flag)
	{
		case case_1:
			free(r->path);
			free(r->host);
			free(r->req);
			break;
		case case_2:
			free(r->path);
			free(r->req);
			break;
		case case_3:
			free(r->host);
			free(r->req);
			break;
		case case_4:
			free(r->req);
			break;
	}
free(r);
}
//--------------------------------------function error---------------------------------------------//

void error(char *err,request *r,int flag,char* httpreq)//taking care of all the errors and free allocated memory.
{
	if(httpreq!=NULL)
		free(httpreq);
	killRequest(r,flag);
	perror(err);
	exit(1);
}
//--------------------------------------function wrongCommand--------------------------------------//

void wrongCommand(request *r,int flag)//this method handle's wrong command and free's current allocated memory.
{
	if(r!=NULL)
		killRequest(r,flag);
	fprintf(stderr,"usage: client [-h] [-d <time-interval>] <URL>\n");
	exit(1);
}
//--------------------------------------function freeAll--------------------------------------//

void freeAll(request *r,char* httpreq)//this method free all allocated memory.
{
	killRequest(r,case_1);
	free(httpreq);
}

