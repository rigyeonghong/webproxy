#include <stdio.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

#include "csapp.h"
#include <stdlib.h>

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int send_request(char *uri, char *port, int fd);

char dest[MAXLINE];
// char port[MAXLINE];

int main(int argc, char **argv) 
{
  // 듣기 식별자, 연결 식별자 선언
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 듣기 식별자 할당
  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    // printf("<------------proxy server request------------->\n");
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    Close(connfd);
  }

}

int send_request(char *uri, char *port, int fd){

  int clientfd;
  char buf[MAXLINE], proxy_res[MAX_OBJECT_SIZE];
  rio_t rio;
  clientfd = Open_clientfd(dest, port);
  // printf("%d\n", clientfd);

  Rio_readinitb(&rio, clientfd);
  sprintf(buf, "GET %s HTTP/1.0\r\n", uri);
  sprintf(buf, "%sConnection: keep-alive\r\n", buf);
  sprintf(buf, "%sCache-Control: max-age=0\r\n", buf);
  sprintf(buf, "%sUpgrade-Insecure-Requests: 1\r\n", buf);
  sprintf(buf, "%sUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36\r\n", buf);
  sprintf(buf, "%sAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9", buf);
  sprintf(buf, "%sAccept-Encoding: gzip, deflate\r\n", buf);
  sprintf(buf, "%sAccept-Language: ko-KR,ko;q=0.9,en-US;q=0.8,en;q=0.7\r\n\r\n", buf);

  Rio_writen(clientfd, buf, strlen(buf));
  // printf("---------------------------------------\n");
  printf("%s", buf);
  Rio_readnb(&rio, proxy_res, MAX_OBJECT_SIZE);
  // printf("%s", proxy_res);

  Rio_writen(fd, proxy_res, MAX_OBJECT_SIZE);


  Close(clientfd);
  // exit(0);

}


void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], new_uri[MAXLINE], port[MAXLINE], tmp[MAXLINE], tmp2[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;
  char *p, *q, *r;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);

  printf("Request line:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  sscanf(strstr(uri, "http://"), "http://%s", tmp);
  
  
  if((p = strchr(tmp, ':')) != NULL){
    *p = '\0';
    sscanf(tmp, "%s", dest);
    sscanf(p+1, "%s", tmp2);
    // printf("%s\n", dest);
    // printf("%s\n", tmp2);

    q = strchr(tmp2, '/');
    *q = '\0';
    sscanf(tmp2, "%s", port);
    // printf("%s\n", port);
    *q = '/';
    sscanf(q, "%s", new_uri);
    // printf("%s\n", new_uri);

    // if(strlen(new_uri) == 0){
    //   new_uri[0] = '/';
    // }
    // printf("%s\n", new_uri);
    
  }

  // printf("%s\n", dest);
  // printf("%s\n", port);
  

  // if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")){
  //   clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
  //   return;
  // }

  // read_requesthdrs(&rio);
  // printf("사용자의 목적지 %s\n", dest);

  send_request(new_uri, port, fd);

  // // /* Parse URI from GET request */
  // is_static = parse_uri(uri, filename, cgiargs);
  

  // if (stat(filename, &sbuf) < 0) {
  //   clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
  //   return;
  // }


  // /* Serve static content */
  // if (is_static) {
  //   if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
  //     clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
  //     return;
  //   }
  //   serve_static(fd, filename, sbuf.st_size, method);
    
  // }
  // /* Serve dynamic content */
  // else{
  //   printf("이 컨텐츠는 동적 컨텐츠 입니다.\n");
  //   if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
  //     clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
  //     return;
  //   }
  //   serve_dynamic(fd, filename, cgiargs, method);
  // }
}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));

}

// request header를 읽는다.
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE], destWithPort[MAXLINE];
  char *p, *p2;

  Rio_readlineb(rp, buf, MAXLINE);
  int cnt = 0;
  while(strcmp(buf, "\r\n")) {
    cnt ++;
    Rio_readlineb(rp, buf, MAXLINE);
    if (p = strstr(&buf, "Referer")){
      printf("%s", p);
      sscanf(p, "Referer: http://%s", dest);
      p2 = strchr(dest, ':');
      *p2 = '\0';
      sscanf(dest, "%s", dest);
      // sscanf(p2+1, "%s", port);
      printf("%s", dest);
      // printf("%s", port);
    }
    printf("%s", buf);
  }

  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin")) {
    
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);

    if (uri[strlen(uri)-1] == '/') {
      strcat(filename, "proxy.html");
    }

    return 1;
  }
  else{

    ptr = index(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }else{
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);

    return 0;
  }

}

void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];


  /* Send response headers to client */
  get_filetype(filename, filetype);
  // response line
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // response header
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));

  printf("Response headers:\n");
  printf("%s", buf);

  if (!strcasecmp(method, "HEAD")){
    return;
  }

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);

  srcp = (int *)malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);

}



/*
get_filetype - Derive file type from filename
*/
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename, ".mp4")) {
    strcpy(filetype, "video/mp4");
  }
  else {
    strcpy(filetype, "text/plain");
  }
}



void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));


  if (Fork() == 0) { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    setenv("REQUEST_METHOD", method, 1);
    Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }

  Wait(NULL); /* Parent waits for and reaps child */
}
