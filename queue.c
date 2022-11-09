#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"

/* 권장 최대 캐시 및 객체 크기 */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* 노드 정의 */
typedef struct Node{
    struct Node *next;
    char request_line[MAXLINE];
    char response[MAXLINE];
}Node;


/* 큐 구조체 정의 */
typedef struct Queue{
    Node *front; /* 맨 앞(꺼낼 위치) */
    Node *rear;  /* 맨 뒤(넣을 위치) */
    int count;   /* 보관 개수 */
}Queue;

/* 함수 선언 */
void InitQueue(Queue *queue);
int IsEmpty(Queue *queue);   
void Enqueue(Queue *queue, char *request_line, char *response);
void Dequeue(Queue *queue);

/*
 * InitQueue - 큐 초기화
 */
void InitQueue(Queue *queue){
    queue->front = queue->rear = NULL; /* front와 rear를 NULL로 설정 */
    queue->count = 0;
}

/*
 * IsEmpty - 큐 초기화
 */
int IsEmpty(Queue *queue){
    return queue->count == 0;
}

/*
 * Enqueue - 큐에 넣기
 */
void Enqueue(Queue *queue, char *request_line, char *response){
    Node *now = (Node *)calloc(1, MAX_OBJECT_SIZE); 
    strcpy(now->request_line, request_line); /* 데이터 복사 */
    strcpy(now->response, response);
    now->next = NULL;
    if (IsEmpty(queue)){        /* 큐가 비었을 때 */
        queue->front = now;     /* 맨 앞을 now로 설정 */        
    }else{                      /* 큐가 비어있지 않을 때 */
        queue->rear->next = now;/* 맨 뒤의 다음을 now로 설정 */
    }
    queue->rear = now;          /* 맨 뒤를 now로 설정 */ 
    queue->count++;             /* 보관 개수 1 증가 */
}

/*
 * Dequeue - 큐에서 빼기
 */
void Dequeue(Queue *queue){
    int re = 0;
    Node *now;
    if (IsEmpty(queue)){ /* 큐가 비었을 때 */
        return re;
    }
    now = queue->front;      /* 맨 앞의 노드를 now에 기억 */
    queue->front = now->next;/* 맨 앞은 now의 다음 노드로 설정 */
    free(now);
    queue->count--;          /* 보관 개수 1 감소 */
    return;
}