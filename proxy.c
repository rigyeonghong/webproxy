#include <stdio.h>
#include "csapp.h"
#include <stdlib.h>
#include "queue.c"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";


void doit(int fd);
void send_request(char *uri, int fd);
void *thread(void *vargp);

char dest[MAXLINE];
Queue queue;


int main(int argc, char **argv) 
{
  int listenfd, *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;
  
  InitQueue(&queue);//큐 초기화

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("<-----------------start of proxy server request------------------>\n");
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    Pthread_create(&tid, NULL, thread, connfdp);
    
  }

}

void *thread(void *vargp)
{
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  Free(vargp);
  doit(connfd);
  Close(connfd);
  printf("<-----------------end of entire request------------------>\n");
  return NULL;
}

void send_request(char *uri, int fd){

  int clientfd;
  char buf[MAXLINE], proxy_res[MAX_OBJECT_SIZE], tmp[MAX_OBJECT_SIZE], tmp2[MAX_OBJECT_SIZE], port[MAX_OBJECT_SIZE], new_uri[MAX_OBJECT_SIZE];
  rio_t rio;
  char *p, *q;

  // uri 파싱
  sscanf(strstr(uri, "http://"), "http://%s", tmp);
  if((p = strchr(tmp, ':')) != NULL){
    *p = '\0';
    sscanf(tmp, "%s", dest);
    sscanf(p+1, "%s", tmp2);

    q = strchr(tmp2, '/');
    *q = '\0';
    sscanf(tmp2, "%s", port);
    *q = '/';
    sscanf(q, "%s", new_uri);
    
  }

  // proxy - tiny
  clientfd = Open_clientfd(dest, port);

  // request header
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
  printf("%s", buf);
  Rio_readnb(&rio, proxy_res, MAX_OBJECT_SIZE);

  Rio_writen(fd, proxy_res, MAX_OBJECT_SIZE);
  Close(clientfd);


  if (queue.count > 10){
    Dequeue(&queue);
    printf("------------------dequeue %d\n", queue.count);
  }

  Enqueue(&queue, uri, &proxy_res);
  printf("----------------enqueue %d\n", queue.count);


}

// fd : telnet - proxy
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;
  

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);

  printf("Request line:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  

  Node *cache = queue.front;
  while (cache != NULL){
    
    if(strcmp(cache->request_line, uri) == 0){
      printf("--------------------cache hit!!\n");
      Rio_writen(fd, cache->response, MAX_OBJECT_SIZE);
      return;
    }
    cache = cache->next;
  }
  
  send_request(&uri, fd);
}