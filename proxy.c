#include <stdio.h>
#include "queue.c"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// #define MAX_THREAD_CNT 2


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

#include "csapp.h"
#include <stdlib.h>

void doit(int fd);
void send_request(char *uri, int fd);
void *thread(void *vargp);

char dest[MAXLINE];
Queue queue;



int main(int argc, char **argv) 
{
  // 듣기 식별자, 연결 식별자 선언
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

  // 듣기 식별자 할당
  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));
    // connfd 번호가 나온다.
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("<------------proxy server request------------->\n");
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    // peer thread 만들기
    // Pthread_create 에서 thread 호출해서 connfd를 연결한 peer thread생성
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


  // 캐싱
  // 여기의 uri 저장.
  if (queue.count > 10){
    Dequeue(&queue);
    printf("------------------dequeue %d\n", queue.count);
  }
  // printf("%s\n", uri);
  // printf("%s\n", proxy_res);

  Enqueue(&queue, uri, &proxy_res);
  printf("----------------enqueue %d\n", queue.count);
  


}


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

  // 이미 받아본 요청이면, 캐시에서 response를 바로 보낸다.
  // 큐를 검사해서 request_line 이 같은지 검사
  // 같으면 불러와서 return
  // 같지 않으면 send_request.
  
  Node *p = queue.front;
  while (p != NULL){
    
    printf("^^^^^^^^^^^^^^^^^^^%s\n", (p->request_line));
    if(!strcmp(p->request_line, &uri)){
      printf("--------------------cache hit!!\n");
      Rio_writen(fd, p->response, MAX_OBJECT_SIZE);
      Close(fd);
      return;
    }
    p = p->next;
  }
  
  send_request(&uri, fd);
}

// 캐시 히트 안남
// 지금 이 상태로 했더니 seg폴트 안남.
// request_line uri랑 비교 how?

// not found 왜 나는지 모름
// 