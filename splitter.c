/*!																www.yoics.com			
 *---------------------------------------------------------------------------
 *! \file splitter.c
 *  \brief Yoics 9100a stream spliter.  
 *																			
 *---------------------------------------------------------------------------
 * Version                                                                  -
 *		0.1 Original Version Dec 7, 2006									-        
 *
 *---------------------------------------------------------------------------    
 *                                                             				-
 * Copyright (C) 2006, Yoics Inc, www.yoics.com								-
 *                                                                         	-
 * $Date: mwj 2006/12/08 20:35:55 $
 *
 *---------------------------------------------------------------------------
 *
 * Notes:
 *
 *  This program is based off of my original PHP stream splitter daemon
 *	located at: 
 *	http://www.mycal.net/projects/9100a/9100d.txt (c)mycal_labs 2006
 *  and also the Simple Image Grabber for the 9100 that is
 *	Copyright (C) 2005 - Mike Johnson - www.mycal.net
 *
 *
*/

#include "mytypes.h"
#include "config.h"

#define MAX_READ_LEN	4096

#define TEMPFILE		"/usr/yoics.jpg"
#define DESTFILE		"/usr/yoics"
#define DEST_PORT		80
#define DEST_IP			0x0100007f;				/*localhost*/

typedef struct JPG_
{
	U16		soi;					//0xffd8
	U8		filler[12];
	U8		channel;
}JPG;


U8				buffer[MAX_READ_LEN];
U8				buffer1[MAX_READ_LEN];

U8				boundry[128];
U8				tstr[64];

//
//
//
U16	get_second_count(void)
{
	// grab a time value and return a 16 bit 10ms count used for timestamping - 1000=1second
	struct timeb	timebuffer;
	U16				ticks;
  
	ftime( &timebuffer );
	
	//time(&t);

	// convert to seconds 
	ticks=0;
	ticks=(U16)(timebuffer.time);

	return(ticks);

}



//
// binary search (quick hack) find a needle(len) in haystack(len) -max 65535 length with register optimization
//
// Returns positive index to start of string if found
//
U8 *
binarysearch(U8 *needle,U16 needle_len, U8 *haystack, U16 haystacklen)
{
	U8			match;
	int			i;
	int			c=0;
	int			first_needle=needle[0];
 
    while(1)
    {
        // Match the first char first

        if(first_needle==haystack[c])
        {
            //
            // Check if there is enough haystack left to process search
            //
            if((haystacklen-c)<needle_len)
                return 0; //return -($haystacklen-$c);

            // Now match reset of string
            match=1;
            for(i=1;i<needle_len;i++)
            {
                if(needle[i]==haystack[c+i])
                {
                    continue;
                }
                else
                {
                    match=0;
                    break;//$i=$needle_len;
                }
            }
            if(match)
                return(&haystack[c]);
        }
        c++;


        if(c>=haystacklen)
            break;
    }
    return 0;
}


//
// If anything needs to be initialized before using the network, put it here.
//
S16
network_init()
{
#ifdef WIN32

	WSADATA w;								/* Used to open Windows connection */
	/* Open windows connection */
	if (WSAStartup(0x0101, &w) != 0)
	{
		fprintf(stderr, "Could not open Windows connection.\n");
	    printf("**** Could not initialize Winsock.\n");
		exit(0);
	}	

#endif
return(0);
}

//
//
//
//
//
//


int get_last_error()
{
#ifdef WIN32
	return(WSAGetLastError());
#endif

#ifdef LINUX
	return(errno);
#endif
}


/*
S16
dump(FILE tfile,U8 *buffer,U16 start,U16 len)
{
	U8	*tstr;

	fwrite(&buffer[start],1,len,tfile);

	
}  
*/



//
// Fix connect to be non blocking...
//
SOCKET
tcp_connect()
{
	int						ret;
	SOCKET					sd;
	struct sockaddr_in		server;				/* Information about the client */
	IPADDR					target;

	

	printf("-connecting to target..\n");

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd==-1)
	{
		printf("..failed\n");
		return(0);
	}

    //
    // Set receive buffer size to 4K!!!!
    // 
   // s=4096;
   ///setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char*) &s, sizeof(s));

	//
	// Connect to localhost port 80
	//
	//target.ipb1=10;
	//target.ipb2=10;
	//target.ipb3=11;
	//target.ipb4=15;
	target.ipb1=127;
	target.ipb2=0;
	target.ipb3=0;
	target.ipb4=1;
	server.sin_port				=htons(DEST_PORT);
	server.sin_addr.s_addr		=target.ip32;
	server.sin_family			= AF_INET;					// host byte order
	//
	ret=connect(sd,(struct sockaddr*)&server,sizeof(struct sockaddr_in));


	//set_sock_nonblock(sd); // ++

	if(ret==-1)
	{
		ret=get_last_error();
		printf("error in connect error= %d\n",ret);
		closesocket(sd);
		return(0);
	}

	printf("***..connected socket %x\n",sd);
	return(sd);
}


int
startup(U8 *buffer, U8 *boundry)
{
	int	ok=0;
	int bound=0,i;
	U8	*subst,*t;

	subst=strtok(buffer,"\n");
	
	do
	{
		// if 2 \r\n's are encountered, this is the end
		if((i=strlen(subst))==0)
			break;;
		if(i==1)
			break;

		printf("line-%d=%s\n",i,subst);
		
		if(strlen(subst)==0)
		{
			// do nothing
		}
		if(0==strcmp(subst,"HTTP/1.0 200 OK\r"))
		{
			printf("OK\n");
			ok=1;
		}
		if(0==strncmp(subst,"Content-Type:",strlen("Content-Type:")))
		{
		    if ( (t=strchr(subst, '=')) )
			{
				strcpy(boundry,&t[1]);
				printf("boundry found = %s\n",boundry);
				bound=1;
			}
		}
	}while((subst=strtok(NULL,"\n")));


	if(bound && ok)
		return(1);

	return(0);
}


int
read_all(SOCKET sd, U8 *buffer, U16 size)
{
	int totread=0
		,nread;

	do
	{
		nread= recv(sd, &(buffer[totread]), size - totread, 0);
		if(nread<=0)
			return-1;
		totread+= nread;
	}	
	while(totread != size);


	return(totread);

}



void
dump_pics(SOCKET sd)
{


	U8				*wp;
	U8				channel;
	int				i,j,go=0,first,len,ret;
	FILE			*tfile;
	JPG				*jpg;
	char			tmp[] = TEMPFILE;
	U8				frames=0;
	U16				timestart=0;

	//
	// Issue A get to the server
	//
	wp="GET /GetData.cgi HTTP/1.1\r\n\r\n";
	i=send(sd,wp,strlen(wp),0);

	if(i<=0)
	{
		printf("send fail %d\n",get_last_error());
		return;
	}
	//
	// Read return code
	//
	i=recv(sd,buffer1,512,0);
	if(i<=0)
	{
		printf("error %d\n",get_last_error());
		return;
	}
	else
	{
		printf("receive read %d bytes\n",i);
		wp=strstr(buffer1,"\r\n\r\n")+4;
		j=wp-buffer1;
		printf("index is %d\n",j);
		//
		//  start up the connection
		//
		if(startup(buffer1,boundry))
		{
			if(i!=j)
				memcpy(buffer,wp,i-j);

			//i=readit(sd,&buffer[i-j],&buffer[i-j]); //			i=recv(sd,&buffer[300],MAX_READ_LEN-(300),0);

			//i=recv(sd,&buffer[i-j],MAX_READ_LEN-(i-j),MSG_WAITALL);

			i=read_all(sd, &buffer[i-j], (U16)(MAX_READ_LEN-(i-j)));
			if(i<=0)
			{
				printf("read file err=%d\n",get_last_error());
				return;
			}	
			printf("read %d\n",i);
			go=1;
			first=1;
			//
			// File
			//
			if(NULL == (tfile = fopen(TEMPFILE, "wb")) )
			{
				printf("tempfile fail %s\n",TEMPFILE);
				return;
			}

		}
		else
		{
			printf("cannot find boundry\n");
			go=0;
		}
		//
		// Now just read forever and write files  (the loop code should be an a callable function)
		//
		timestart=get_second_count();
		while((sd) && (go))
		{
			//
			// Search for the boundry
			//
			//wp=strstr(buffer,boundry);
			wp=binarysearch(boundry,(U16)strlen(boundry), buffer,(U16)(MAX_READ_LEN-250) );

			if(wp)
			{
				if(first)
				{
					first=0;
				}
				else
				{
					// flush and close file
					len=(wp-buffer)-2;
					fwrite(&buffer[0],1,len,tfile);
					fclose(tfile);
					//
					//sched_yield();
					//
					// Rename file
					//
					sprintf(tstr,DESTFILE"%d.jpg",channel);
					ret=rename(tmp, tstr);
					if(-1==ret)
					{
						printf("error in rename error= %d\n",get_last_error());
						//sleep(1);
					}
					frames++;

				}//if
				//
				// Setup for the next image, Find the start of the image, then index in and
				// extract the channel number out of the image and save it.
				//
				while(*wp!=0xff)
					wp++;
				//
				// We should be at the start of the jpg
				//
				//printf("start byte=%x %x %x\n",wp[0],wp[1],wp[2]);			
				jpg=(JPG*)wp;
				//
				// Extract channel number
				//
				channel=jpg->channel;
				//printf("channel number = %d\n",channel);
				//
				// open new file
				//
				if(NULL == (tfile = fopen(TEMPFILE, "wb")) )
					return;
				//
				// Write out data
				//
				i=MAX_READ_LEN-(wp-buffer);
				if(i<=300)
				{
					//printf("*********\n");
				}
				//
				// Write the rest out
				//
				fwrite(wp,1,i,tfile);
				i=0;
				//
				// copy over the last 300 bytes
				//
				//memcpy(buffer,&buffer[MAX_READ_LEN-301],300);
			}
			else
			{
				// Write the data except for the last 300 bytes, copy this to the beginning of the next buffer
				i=300;
				fwrite(&buffer[0],1,MAX_READ_LEN-i,tfile);
				memcpy(buffer,&buffer[MAX_READ_LEN-i/*-1*/],300);

				//if (wp)
				//	printf("on edge wp\n");
			}//if(wp)
			//
			// Read in another buffer
			//
			//i=recv(sd,&buffer[300],MAX_READ_LEN-(300),MSG_WAITALL);
			i=read_all(sd, &buffer[i], (U16)(MAX_READ_LEN-(i)));
			if(i<=0)
			{
				printf("read fail 2 err=%d",get_last_error());
				return;
			}
			//
			// printout FPS
			//
			if(timestart!=(get_second_count()/10))
			{
				printf("Running at %dFPS\n",frames/10);
				frames=0;
				timestart=get_second_count()/10;
			}
		

		}// end while
	}//eindif
}



main()
{
	SOCKET			sd;

	network_init();

	printf("9100a Video Server Stream Splitter Startup V0.3 Alpha\n");
	printf("--(c)2006 Yoics Inc. All Rights Reserved.\n");

	//wait 5 seconds
	sleep(10);
	printf("Starting up...\n");
	//
	// Authenticate against stored key
	//
	// secret + MAC = "purchased key" - use SHA1 but key will be only 16 bytes (make them think MD5)
	//
	// Loop Forever
	while(1)
	{
		sd=tcp_connect();

		if(sd)
		{
			printf("dumppics\n");
			dump_pics(sd);
			closesocket(sd);
		}
		else
		{
			printf("TCP connect Fail\n");
		}
#ifdef LINUX
		printf("sleep\n");
		sleep(5);			// Sleep before restart of socket
#else
		Sleep(5000);
#endif
	}
}




