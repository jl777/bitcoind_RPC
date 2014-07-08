//
//  main.c
//  bitcoind_RPC
//
//  Created by jimbo laptop on 7/8/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <pthread.h>
#include "cJSON.h"
#include <curl/curl.h>

#define issue_NXT(curl_handle,cmdstr) bitcoind_RPC(curl_handle,"NXT",cmdstr,0,0,0)
#define NXTSERVER "http://127.0.0.1:7876/nxt?requestType"
char *bitcoind_RPC(CURL *curl_handle,char *debugstr,char *url,char *userpass,char *command,char *params);


int32_t set_current_NXTblock(int32_t *isrescanp,CURL *curl_handle,char *blockidstr)
{
    int32_t numblocks = 0;//,numunlocked = 0;
    cJSON *blockjson,*json;
    char cmd[256],*retstr;
    sprintf(cmd,"%s=getState",NXTSERVER);
    *isrescanp = 0;
    blockidstr[0] = 0;
    retstr = issue_NXT(0,cmd);
    if ( retstr != 0 )
    {
        if ( (json= cJSON_Parse(retstr)) != 0 )
        {
            numblocks = (int32_t)get_cJSON_int(json,"numberOfBlocks");
            *isrescanp = (int32_t)get_cJSON_int(json,"isScanning");
            blockjson = cJSON_GetObjectItem(json,"lastBlock");
            copy_cJSON(blockidstr,blockjson);
            free_json(json);
        }
        free(retstr);
    }
    return(numblocks);
}

void set_prev_NXTblock(CURL *curl_handle,int32_t *height,int32_t *timestamp,char *prevblock,char *blockidstr)
{
    char cmd[4096],*jsonstr;
    int32_t errcode;
    cJSON *prevjson,*json;
    prevblock[0] = 0;
    if ( timestamp != 0 )
        *timestamp = 0;
    if ( height != 0 )
        *height = 0;
    sprintf(cmd,"%s=getBlock&block=%s",NXTSERVER,blockidstr);
    jsonstr = issue_NXT(curl_handle,cmd);
    if ( jsonstr != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            errcode = (int32_t)get_cJSON_int(json,"errorCode");
            if ( errcode == 0 )
            {
                if ( timestamp != 0 )
                    *timestamp = (int32_t)get_cJSON_int(json,"timestamp");
                if ( height != 0 )
                    *height = (int32_t)get_cJSON_int(json,"height");
                prevjson = cJSON_GetObjectItem(json,"previousBlock");
                if ( prevjson != 0 )
                    copy_cJSON(prevblock,prevjson);
            } else printf("%s\n",cJSON_Print(json));
            free_json(json);
        }
        free(jsonstr);
    }
}

void *curl_loop(void *arg)
{
    int32_t bigloops=0,numblocks,rescanning,height,timestamp,threadid,*threadidp = arg;
    char blockidstr[64],prevblock[64];
    threadid = *threadidp;
    while ( 1 )
    {
        numblocks = set_current_NXTblock(&rescanning,0,blockidstr);
        printf("bigloop.%d threadid.%d: numblocks.%d scan back from (%s) to 0\n",bigloops,threadid,numblocks,blockidstr);
        do
        {
            set_prev_NXTblock(0,&height,&timestamp,prevblock,blockidstr);
            if ( (height % 1000) == 0 )
                printf("threadid.%d: height.%d timestamp.%d (%s) prev.(%s)\n",threadid,height,timestamp,blockidstr,prevblock);
            strcpy(blockidstr,prevblock);
        }
        while ( blockidstr[0] != 0 && height != 0 );
        bigloops++;
    }
    return(0);
}

// gets errors like:
// curl_easy_perform() failed: Couldn't connect to server NXT.(http://127.0.0.1:7876/nxt?requestType=getBlock&block=8510055566383176347 (null) (null))
// unless sleeps are enabled in bitcoind_RPC.c
// if ( 1 && laststart+pause > milliseconds() ) // horrible hack for bitcoind "Couldn't connect to server"
//  usleep(pause*1000);
// errors happen even with just one thread, but with more than one thread, the errors happen a lot
//
// you need to have NXT core installed
// wget https://bitbucket.org/JeanLucPicard/nxt/downloads/nxt-client-1.1.6.zip
// unzip nxt-client-1.1.6.zip
// cd nxt
// ./run.sh &


int main(int argc, const char * argv[])
{
    static int threadids[1];
    int i;
    for (i=0; i<(int)(sizeof(threadids)/sizeof(*threadids)); i++)
    {
        threadids[i] = i;
        if ( pthread_create(malloc(sizeof(pthread_t)),NULL,curl_loop,&threadids[i]) != 0 )
            printf("ERROR curl_loop\n");
    }
    while ( 1 )
        sleep(60);
}

