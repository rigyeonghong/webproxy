/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.1 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include <signal.h>

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) 
{
  /* 듣기 식별자, 연결 식별자 선언 */
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr; 

  /* argc 확인 */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  /* 듣기 식별자 오픈 */ 
  listenfd = Open_listenfd(argv[1]);

  /* 무한 루프에서 client로부터 연결 요청 기다림 */
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  /* 반복적으로 연결 요청 접수 */
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port); /* 도메인 이름과 연결된 클라이언트의 포트 출력*/
    doit(connfd);   /* 트랜잭션 수행 */
    Close(connfd);  /* server 연결 식별자 닫음 */
  }
}


/*
 * doit - HTTP 트랜잭션 처리
 */
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* 요청 라인과 헤더 읽음 */
  Rio_readinitb(&rio, fd);           /* Rio_read 초기화 */
  Rio_readlineb(&rio, buf, MAXLINE); /* buf에 읽은 데이터 맨위의 한 줄 읽음 */
  printf("Request line:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); /* request line에서 method, uri, version 값 추출 */

  /* method가 GET, HEAD가 아닌 다른 것이 들어오면 */
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")){ 
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  /* tiny는 요청된 헤더 읽고 무시 */
  read_requesthdrs(&rio);

  /* 요청된 uri 해석 */
  is_static = parse_uri(uri, filename, cgiargs);
  
  /* stat -> 지정된 파일에 대한 상태 정보를 가져와 buf 인수로 가리킨 메모리 영역에 배치. 성공하면 0 / 없으면 -1 return */ 
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static) { /* 정적 콘텐츠 서빙 */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    printf("%s", filename);
    serve_static(fd, filename, sbuf.st_size, method);
    
  }
  else{ /* 동적 콘텐츠 서빙 */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, method);
  }
}


/*
 * clienterror - 클라이언트 에러 메세지 출력
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* HTTP 응답 바디 생성 */
  sprintf(body, "<html><title>Tiny Error</title>"); /* sprintf(): 형식화된 데이터를 버퍼로 출력 */
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* HTTP 응답 출력 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}


/*
 * read_requesthdrs - request header 읽기 
 */
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}


/*
 * parse_uri - uri 문자열 검사
 */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  // strstr 함수: string1에서 string2의 첫 번째 표시를 찾는다.
  /* 정적 컨텐츠 */
  if (!strstr(uri, "cgi-bin")) { /* 실행파일의 홈 디렉토리인 /cgi-bin이 아닐 경우 */
    // strcpy 함수: string2를 string1에서 지정한 위치로 복사한다.
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    // strcat 함수: string2를 string1에 연결하고 널 문자로 결과 스트링을 종료.
    strcat(filename, uri); 
    printf("%s\n", filename);

    /* 최소한의 URL을 갖은 경우 - 특정 기본 홈페이지("home.html")로 확장 */ 
    if (uri[strlen(uri)-1] == '/') {
      strcat(filename, "home.html");
    }
    return 1;
  }
  /* 동적 컨텐츠 */
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


/*
 * serve_static - 정적 콘텐츠 서브
 */
void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* client에 보내는 응답 헤더 */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); /* 응답 라인 */
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf); /* 응답 헤더 */
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); 
  Rio_writen(fd, buf, strlen(buf));

  /* 응답 헤더 출력 */
  printf("Response headers:\n");
  printf("%s", buf);

  /* HEAD method는 응답 바디 출력 안함 */
  if (!strcasecmp(method, "HEAD")){
    return;
  }

  /* client에 응답 바디 전송 */ 
  srcfd = Open(filename, O_RDONLY, 0); /* file open 가능여부 확인*/

  /* 요청 파일을 가상 메모리 영역으로 매핑하는 Mmap() 대신 malloc */
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  srcp = (int *)malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd); /* srcfd 식별자는 더이상 필요없어 닫음 */
  Rio_writen(fd, srcp, filesize); 
  /* 매핑된 가상 메모리 주소 반환하는 Munmap 대신 free*/
  // Munmap(srcp, filesize);
  free(srcp);

}


/*
 * get_filetype - 파일 이름에서 파일 형식(확장자) 가져오기
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


/*
 * serve_dynamic - 동적 콘텐츠 서브
 */
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* HTTP 응답의 첫 부분 return */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* 자식 분기 */
  if (Fork() == 0) { 
    /* CGI 변수 설정 */
    setenv("QUERY_STRING", cgiargs, 1);
    setenv("REQUEST_METHOD", method, 1);
    Dup2(fd, STDOUT_FILENO); /* client로 stdout을 redirect */
    Execve(filename, emptylist, environ); /* CGI program 실행 */
  }
  Wait(NULL); /* 부모가 자식 기다리고 처리 */
}