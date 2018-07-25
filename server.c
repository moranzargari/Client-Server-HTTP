//header files--c library
#include <stdio.h>
#include "threadpool.h"
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>



//-------------------------------------------Defines------------------------------------------//
#define BODYFORMAT "<HTML><HEAD><TITLE>@</TITLE></HEAD><BODY><H4>@</H4>#</BODY></HTML>"
#define BODYDIRC "<table CELLSPACING=8><tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>"

#define BODY400 "Bad Request."
#define BODY404 "File not found."
#define BODY403 "Access denied."
#define BODY302 "Directories must end with a slash."
#define BODY501 "Method is not supported."
#define BODY500 "Some server side error."

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define CODE400 400
#define CODE404 404
#define CODE403 403
#define CODE302 302
#define CODE501 501
#define CODE500 500
#define FILECODE 1
#define DIRCONTENT -1
#define FILECODE2 2
#define MAXLEN 4000
#define TIMEBUF_SIZE 128
#define ENTITY 500
#define FAIL -1
#define SUCCESS 0

#ifndef server_c
#define server_c

//--------------------------------------------Struct----------------------------------------//
typedef struct{

	unsigned long bodySize;
	char line[MAXLEN];
	char *dir;
	char* msg;
	char date[TIMEBUF_SIZE];
	char* location;
	char* ContentType;
	char LModified[TIMEBUF_SIZE];	
	char* body;

}RESP;

//------------------------------Declaration of functions------------------------------------//
void hError(char *err);
void extractLine(char* req,RESP *rsp);
int negofunction(void* num);
int checkLine(RESP *rsp);
int countWords(char* line);
char* extractPath(char* line);
int isDirectory(char *dir);
char* response(RESP *rsp,int status);
char *get_mime_type(char *name);
int handleStatus(RESP *rsp,int status);
int readFromFile(int sockfd,RESP *rsp);
char* returnErrorBody(char *s,char* msg,char* msg2,RESP *rsp);
char* createDirContent(RESP *rsp);
int searchIndexHtml(char *dir);
int checkArgv(int argc,char* argv[]);
void freeAll(RESP *rsp,char* rec);
int EntityList(struct dirent **d,char* str,char* tmpdir,RESP *rsp,int i);
int OK(RESP *rsp,int status);
void modifyDate(RESP *rsp);
char* lastModified(char* dir);
int getFileSize(char* dir,int flg);
int readPermission(char* dir);
void spaces(char *dst, const char *src);


//=========================================MAIN==================================================//
int main(int argc,char *argv[])// ./server <port> <pool-size> <max-number-of-request>
{
	if(checkArgv(argc,argv)==FAIL)//check input
		hError("Usage: server <port> <pool-size> <max-number-of-request>");

	int i=0,sockfd,fdArr[atoi(argv[3])];

	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;
	socklen_t clilen=atoi(argv[3]);
	
	if((sockfd=socket(PF_INET,SOCK_STREAM,0))==FAIL)//create new socket
		hError("socket");

	serv_addr.sin_family = PF_INET;
	serv_addr.sin_addr.s_addr =htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if(bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr))<0)
		hError("bind");
	listen(sockfd,5);//listen to 5 requests each time.
	threadpool* pool=create_threadpool(atoi(argv[2]));//create threadpool
	
	while(i<atoi(argv[3]))
	{		
		if((fdArr[i]=accept(sockfd,(struct sockaddr*)&client_addr,&clilen))<0)//accept client request.
		{
			close(sockfd);
			free(pool->threads);
			free(pool);
			hError("accept");
		}
		dispatch(pool,negofunction,(void*)& fdArr[i]);//thread handle the request.
		i++;
	}
	destroy_threadpool(pool);
	close(sockfd);
return SUCCESS;
}

//============================================================================FUNCTIONS=========================================================================================================//

//----------------------------------------------------checkArgv------------------------------------------------------//
int checkArgv(int argc,char* argv[])//this method check the input.
{
	if(argc<4 || argc>4)
		return FAIL;
	int i,j=1;
	char *str;
	while(j<argc)
	{
		str=argv[j];
		for(i=0;i<strlen(str);i++)
		{
			if(!isdigit(str[i]))
				return FAIL;
		}
		j++;
	}
return SUCCESS;   
}
//----------------------------------------------------negofunction------------------------------------------------------//
int negofunction(void* num)//this is the main negotiation function, the function read request and write response.
{
	int sockfd=*((int*)num),rc,status=0;
	char buffer[MAXLEN];
	RESP *rsp;

	do {//read request
		rc = read(sockfd,buffer,sizeof(buffer));
		if (rc < 0)
		{
			close(sockfd);	
			status=CODE500;
		}
		else if(rc==0)
			return FAIL;
		else
		    break;
	    } while (1);
	
		rsp=(RESP*)calloc(1,sizeof(RESP));//allocate memory for the struct
		if(rsp==NULL)
			status=CODE500;

		extractLine(buffer,rsp);//extract only the first line from the request.
		if(status==0)
			status=checkLine(rsp);

		if(handleStatus(rsp,status)==FAIL){handleStatus(rsp,CODE500);}

		char* rec=response(rsp,status);

		if(rec==NULL){freeAll(rsp,rec);return FAIL;}

		while(write(sockfd,rec,strlen(rec))!=(strlen(rec)))//write response to client.
		{
			close(sockfd);
			hError("write");
		}
		if(status==FILECODE || status==FILECODE2)
			if(readFromFile(sockfd,rsp)==FAIL){close(sockfd);}
	
	freeAll(rsp,rec);   
	close(sockfd);
return SUCCESS;
}
//----------------------------------------------------checkLine------------------------------------------------------//
int checkLine(RESP *rsp)//this method check the request first line and return the status of the request.
{
	struct stat buf;
	int status=0,n;

	rsp->dir=extractPath(rsp->line);
	if(rsp->dir==NULL){return FAIL;}

	if(countWords(rsp->line)>2 || countWords(rsp->line)<2)
		status=CODE400;
	else if(strstr(rsp->line,"HTTP/1.1")==NULL && strstr(rsp->line,"HTTP/1.0")==NULL)
		status=CODE400;
	else if(strstr(rsp->line,"GET")==NULL)
		status=CODE501;
	else
	{
		if(readPermission(rsp->dir)==FAIL){return CODE403;}
		n=stat(rsp->dir,&buf);
		if(n==FAIL)//in case the path dose not exists
		{
			if(errno==ENOENT){status=CODE404;}
			else{status=CODE500;}
		}	
		else
		{	
			
			if(S_ISDIR(buf.st_mode))
				status=isDirectory(rsp->dir);
			else
			{
				if(S_ISREG(buf.st_mode))
					status=FILECODE;
				else{status=CODE403;}	
			}
		}	
	} 
return status;
}

//----------------------------------------------------handleStatus------------------------------------------------------//
int handleStatus(RESP *rsp,int status)//this method receive's request status and define the correct response.
{
	int flg=0;
	if(status==FAIL){flg=FAIL;}
	modifyDate(rsp);
	rsp->ContentType=get_mime_type(".html");
	switch(status)
	{
		case CODE400:
			rsp->msg="400 Bad Request";
			rsp->body=returnErrorBody(BODYFORMAT,rsp->msg,BODY400,rsp);
			if(rsp->body==NULL){flg=FAIL;}
			break;
		case CODE404:
   			rsp->msg="404 Not Found";
			rsp->body=returnErrorBody(BODYFORMAT,rsp->msg,BODY404,rsp);
			if(rsp->body==NULL){flg=FAIL;}
			break;
		case CODE403:
   			rsp->msg="403 Forbidden";
			rsp->body=returnErrorBody(BODYFORMAT,rsp->msg,BODY403,rsp);
			if(rsp->body==NULL){flg=FAIL;}
			break;
		case CODE302:
   			rsp->msg="302 Found";
			rsp->body=returnErrorBody(BODYFORMAT,rsp->msg,BODY302,rsp);
			rsp->location=rsp->dir;
			if(rsp->body==NULL){flg=FAIL;}
			break;
		case CODE500:
			rsp->msg="500 Internal Server Error";
			rsp->body=returnErrorBody(BODYFORMAT,rsp->msg,BODY500,rsp);
			if(rsp->body==NULL){flg=FAIL;}
			break;
		case CODE501:
			rsp->msg="501 Not supported";
			rsp->body=returnErrorBody(BODYFORMAT,rsp->msg,BODY501,rsp);
			if(rsp->body==NULL){flg=FAIL;}
			break;
		default:
			flg=OK(rsp,status);
			break;
	}
return flg;
}

//----------------------------------------------------response------------------------------------------------------//
char* response(RESP *rsp,int status)//this method create the entire response headers+data.
{
	char *rec=(char*)calloc(MAXLEN,sizeof(char));
	char *str=(char*)calloc(MAXLEN+1,sizeof(char));
	if(rec==NULL || str==NULL){return NULL;}
	long size;

	size=sprintf(str,"HTTP/1.1 %s\r\nServer: webserver/1.1\r\nDate: %s\r\n",rsp->msg,rsp->date);
	if(status==CODE302)
	{
		size+=sprintf(rec,"Location: %s/ \r\n",rsp->dir);
		strcat(str,rec);
	}
	if(rsp->ContentType!=NULL)
	{
		size+=sprintf(rec,"Content-Type: %s\r\nContent-Length: %ld\r\n",rsp->ContentType,rsp->bodySize);
		strcat(str,rec);
	}
	else
	{
		size+=sprintf(rec,"Content-Length: %ld\r\n",rsp->bodySize);
		strcat(str,rec);
	}
	if(status==FILECODE || status==DIRCONTENT || status==FILECODE2)
	{
		size+=sprintf(rec,"Last-Modified: %s\r\n",rsp->LModified);
		strcat(str,rec);
	}
	size+=sprintf(rec,"%sConnection: close\r\n\r\n",str);
	if(status!=FILECODE && status!=FILECODE2)
	{
		rec=(char*)realloc(rec,strlen(rsp->body)+size+1);
		if(rec==NULL){return NULL;}
		strcat(rec,rsp->body);
	}
free(str);
return rec;
}

//----------------------------------------------------readFromFile------------------------------------------------------//
int readFromFile(int sockfd,RESP *rsp)//this method read file data and write the file data to the client.
{
	int fd,rd,wt;	
	char buffer[1024];
        if((fd = open(rsp->dir, O_RDONLY)) == FAIL){return FAIL;}

	while((rd=read(fd,buffer,sizeof(buffer)>0)))
	{
		while(rd>0)
		{
			if((wt=write(sockfd,buffer,rd))>0){rd-=wt;}
			else if(wt==0){close(sockfd);return FAIL;}
			else {
				if(errno==EINTR){continue;}
				close(sockfd);
				return FAIL;
			}
		}
	}
return SUCCESS;
}

//----------------------------------------------------modifyTime------------------------------------------------------//
char* lastModified(char* dir)//this method return the file last modified date and time.
{
	char* date=(char*)calloc(TIMEBUF_SIZE+1,sizeof(char));
	if(date==NULL){return NULL;}

	 struct stat attrib;
	 stat(dir, &attrib);
	 strftime(date,TIMEBUF_SIZE,RFC1123FMT,localtime(&(attrib.st_ctime)));
return date;
}

//----------------------------------------------------modifyDate------------------------------------------------------//
void modifyDate(RESP *rsp)//this method return the current date and time.
{
	time_t now;
	now = time(NULL);
	strftime(rsp->date,TIMEBUF_SIZE, RFC1123FMT, gmtime(&now));
}

//----------------------------------------------------extractLine------------------------------------------------------//
void extractLine(char* req,RESP *rsp)//this method extract the request first line.
{
	char line[MAXLEN];
	strcpy(line,strtok(req,"\r"));
	strcpy(rsp->line,line);
}

//----------------------------------------------------countWords------------------------------------------------------//
int countWords(char* line)//this method check request string for errors.
{
	int count=0,i=0;
	while(line[i]!='\0')
	{
		if(line[i]==' ')
		{
			count++;
			while(line[i+1]==' '){i++;}	
		}	
		i++;
	}
return count;
}

//----------------------------------------------------extractPath------------------------------------------------------//
char* extractPath(char* line)//this method extract the directory/file path frome the request first line.
{
	char *rest,*token,temp[MAXLEN];
	strcpy(temp,line);
	token=strtok_r(temp," ",&rest);
	token=strtok(rest," ");
	char* dir=(char*)calloc(MAXLEN+1,sizeof(char));
	if(dir==NULL){return NULL;}
	strcpy(dir,token);
	strcpy(temp,".");
	strcat(temp,dir);
	spaces(dir, temp);
return dir;
}

//----------------------------------------------------isDirectory------------------------------------------------------//
int isDirectory(char *dir)//this method handle all directory paths.
{
	char endWith= dir[strlen(dir)-1];
	if(endWith!='/'){return CODE302;}
	else
	{
		int x=searchIndexHtml(dir);
		if(x==1){return FILECODE2;}
		else if(x==FAIL){return CODE500;}
		else if(x==CODE403){return CODE403;}
		else {return DIRCONTENT;}		
	}	
return SUCCESS;
}

//----------------------------------------------------searchIndexHtml------------------------------------------------------//
int searchIndexHtml(char *dir)//this method check if there is a file name "index.html" inside the current directory path.
{
	DIR *directory;
	struct dirent *d;

	directory=opendir(dir);
	if(directory!=NULL)
	{
		char path[MAXLEN+1];
		strcpy(path,dir);
		strcat(path,"index.html");
		while((d=readdir(directory))!=NULL)
		{
			if(strcmp(d->d_name,"index.html")==0 && readPermission(path)!=FAIL){closedir(directory);return 1;}
			else if(strcmp(d->d_name,"index.html")==0 && readPermission(path)==FAIL){closedir(directory);return CODE403;}//check file "index.html" permmisions.
		}
	closedir(directory);
	}
	else {return FAIL;}

return SUCCESS;
}

//----------------------------------------------------get_mime_type------------------------------------------------------//
char *get_mime_type(char *name) {//this method return the file content type.
char *ext = strrchr(name, '.');
if (!ext) return NULL;
if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
if (strcmp(ext, ".gif") == 0) return "image/gif";
if (strcmp(ext, ".png") == 0) return "image/png";
if (strcmp(ext, ".css") == 0) return "text/css";
if (strcmp(ext, ".au") == 0) return "audio/basic";
if (strcmp(ext, ".wav") == 0) return "audio/wav";
if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
return NULL;
} 

//----------------------------------------------------returnErrorBody------------------------------------------------------//
char* returnErrorBody(char *s,char* msg,char* msg2,RESP *rsp)//this method replace string by another string--> build the errors respond body.
{
	int count = 3;
	const char *t;
	int size=strlen(msg)+strlen(msg2);
	int len=(strlen(s)+1) + (size-1)*count + 1;

    char *res = (char*)calloc(len,sizeof(char));
	if(res==NULL)
		return NULL;
    char *ptr = res;
    for(t=s; *t; t++) {
        if(*t == '@') {
            memcpy(ptr, msg, strlen(msg));
            ptr += strlen(msg);
		}
		else if(*t == '#'){
		   memcpy(ptr, msg2, strlen(msg2));
            ptr += strlen(msg2);
		}
        else {
            *ptr++ = *t;
        }
    }
    *ptr = 0;
    rsp->bodySize=strlen(res);
    return res;
}

//----------------------------------------------------createDirContent------------------------------------------------------//
char* createDirContent(RESP *rsp)//this method build the the "dir content" format body.
{
	struct dirent **d;
	int n,i;
	char *str,*tmpdir;
	
	n=scandir(rsp->dir,&d,NULL,alphasort);
	if(n<0)
		return NULL;
	else
	{
		tmpdir=(char*)calloc(strlen(rsp->dir)+1,sizeof(char));
		str=(char*)calloc(strlen(BODYDIRC)+(n*ENTITY)+1,sizeof(char));
		if(str==NULL || tmpdir==NULL)
			return NULL;
		memcpy(str,BODYDIRC,strlen(BODYDIRC));
		for(i=0;i<n;i++)
		{		
			strcpy(tmpdir,rsp->dir);
			tmpdir=(char*)realloc(tmpdir,strlen(tmpdir)+strlen(d[i]->d_name)+1);
			if(str==NULL || tmpdir==NULL || EntityList(d,str,tmpdir,rsp,i)==FAIL)
			{
				free(tmpdir);
				free(str);
				return NULL;
			}
			free(d[i]);
		}
	free(d);
	}		
strcat(str,"</table><HR><ADDRESS>webserver/1.1</ADDRESS>");
free(tmpdir);
rsp->bodySize=strlen(str);
return str;
}

//----------------------------------------------------hError------------------------------------------------------//
void hError(char *err)//taking care of all the errors and exit.
{
	perror(err);
	exit(1);
}

//----------------------------------------------------freeAll------------------------------------------------------//
void freeAll(RESP *rsp,char* rec)//this method free all allocated memmory after the thread finish the work.
{
	free(rec);
	free(rsp->body);
	free(rsp->dir);
	free(rsp);
}

//----------------------------------------------------EntityList------------------------------------------------------//
int EntityList(struct dirent **d,char* str,char* tmpdir,RESP *rsp,int i)//this method called by the function "createDirContent" to build the entitys table
{
	char *t=(char*)calloc(strlen(str)+1,sizeof(char));
	if(t==NULL){return FAIL;}

	strcat(tmpdir,d[i]->d_name);
	sprintf(t,"<tr><td><A HREF=\"%s\">%s</A></td>",d[i]->d_name,d[i]->d_name);
	strcat(str,t);
	char* time=lastModified(tmpdir);
	if(time==NULL){return FAIL;}
	sprintf(t,"<td>%s</td>",time);
	strcat(str,t);
	if(d[i]->d_type == DT_REG)
	{
		if(getFileSize(tmpdir,0)==FAIL){return FAIL;}
		sprintf(t,"<td>%d</td></tr>",getFileSize(tmpdir,0));
		strcat(str,t);
	}
	else
		strcat(str,"<td></td></tr>");	
free(t);
free(time);
return SUCCESS;
}

//----------------------------------------------------OK------------------------------------------------------//
int OK(RESP *rsp,int status)//this method handle all 200 ok responses .
{
	rsp->msg="200 OK";
	char* time=lastModified(rsp->dir);
	if(time==NULL){return FAIL;}
	strcpy(rsp->LModified,time);

	if(status==DIRCONTENT)
	{
		char str[MAXLEN];
		strcpy(str,"Index of ");
		strcat(str,rsp->dir);
		char* dircont=createDirContent(rsp);
		if(dircont==NULL){return FAIL;}
		rsp->body=returnErrorBody(BODYFORMAT,str,dircont,rsp);
		if(rsp->body==NULL){free(time);free(dircont);return FAIL;}
		free(dircont);
	}
	else
	{
		if(status==FILECODE)	
			rsp->ContentType=get_mime_type(rsp->dir);
		else if(status==FILECODE2)
			strcat(rsp->dir,"index.html");
		if((rsp->bodySize=getFileSize(rsp->dir,1))==FAIL){return FAIL;}
	}
	
free(time);
return SUCCESS;
}

//----------------------------------------------------getFileSize------------------------------------------------------//
int getFileSize(char* dir,int flg)//this method return the wanted file size using the correct flag.
{
	unsigned long size;
	if(flg==0)
	{
		struct stat buf;
		stat(dir,&buf);
		size=buf.st_size;
	}
	else
	{
		FILE *f;	
 		if((f = fopen(dir, "rb")) == NULL){return FAIL;}	
		if(fseek (f, 0, SEEK_END)!=0){return FAIL;}
		size = ftell (f);
		if(fseek (f, 0, SEEK_SET)!=0){return FAIL;}
		fclose(f);
	}
return size;
}
//----------------------------------------------------readPermission------------------------------------------------------//
int readPermission(char* dir)//this method receive a path and check for execution permmisions and read permmisions.
{
	char tmp[MAXLEN+1];
	int i,count=0;
	struct stat s;

	for(i=0;i<strlen(dir);i++)
		if(dir[i]=='/'){count++;}
	for(i=0;i<strlen(dir);i++)
	{
		if(dir[i]=='/')
		{
			count--;
			if(count==1)
			{
				strncpy(tmp,dir,strlen(dir));
				tmp[strlen(dir)]='\0';
			}
			else
			{
				strncpy(tmp,dir,i+1);
				tmp[i+1]='\0';
			}
			stat(tmp,&s);
			if(S_ISDIR(s.st_mode)){
				if(!(S_IXOTH & s.st_mode) || !(S_IXGRP & s.st_mode) || !(S_IXUSR & s.st_mode)){return FAIL;}
			}
			else{
				if(!(S_IROTH & s.st_mode) || !(S_IRGRP & s.st_mode) || !(S_IRUSR & s.st_mode)){return FAIL;}
			}	
		}
	}
return SUCCESS;
}

//----------------------------------------------------spaces------------------------------------------------------//
void spaces(char *dst, const char *src)
{
        char a, b;
        while (*src) {
                if ((*src == '%') &&
                    ((a = src[1]) && (b = src[2])) &&
                    (isxdigit(a) && isxdigit(b))) {
                        if (a >= 'a')
                                a -= 'a'-'A';
                        if (a >= 'A')
                                a -= ('A' - 10);
                        else
                                a -= '0';
                        if (b >= 'a')
                                b -= 'a'-'A';
                        if (b >= 'A')
                                b -= ('A' - 10);
                        else
                                b -= '0';
                        *dst++ = 16*a+b;
                        src+=3;
                } else if (*src == '+') {
                        *dst++ = ' ';
                        src++;
                } else {
                        *dst++ = *src++;
                }
        }
        *dst++ = '\0';
}
#endif





