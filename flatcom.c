/*!																www.yoics.com			
 *---------------------------------------------------------------------------
 *! \file flatcom.c
 *  \brief Yoics camera.flat communication interface.  
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
 *
*/

#include "mytypes.h"
#include "config.h"


#if UCLINUX
/*
	communication with IPCam by HTTP
	Return: -1, network error
			0, success
*/

int NetConnect(const char *pcServer, unsigned short usPort, int iMillSecTimeout, int *iOut_fd)
{
	int fd;
	int iFl;
	struct timeval tv;
	fd_set fdsRead;
	fd_set fdsWrite;

	struct hostent* sHostent = NULL;
	struct hostent* sHost = NULL;
	unsigned long ulAddr;
	struct sockaddr_in sin;
	char **ppc;

	if (iOut_fd == NULL || pcServer == NULL || iMillSecTimeout <= 0)
	{
		fprintf(stderr, "YOICS_CONFIG:NetConnect:Illegal parameters while call NetConnect!\n");
		return -1;
	}
	*iOut_fd = -1;

	if ((sHostent = gethostbyname(pcServer)) == NULL)
	{
		fprintf(stderr, "YOICS_CONFIG:NetConnect:Failed to revolve host!\n");
		return -1;
	}

	tv.tv_sec = iMillSecTimeout / 1000;
	tv.tv_usec = (iMillSecTimeout - 1000 * tv.tv_sec) * 1000;

	for (ppc=sHostent->h_addr_list; *ppc; ppc++)
	{
		/* create socket */
		fd = socket(PF_INET,SOCK_STREAM,0);
		if (fd < 0)
		{
			fprintf(stderr, "YOICS_CONFIG:NetConnect:Can not create socket!\n");
			return -1;
		}

		/* set socket to O_NDELAY */
		iFl = fcntl(fd, F_GETFL, 0);
		if (fcntl(fd, F_SETFL, iFl | O_NDELAY) != 0)
		{
			fprintf(stderr, "YOICS_CONFIG:NetConnect:Can not set socket fd to O_NDELAY mode.\n");
			close(fd);
			return -1;
		}

		FD_ZERO(&fdsRead);
		FD_SET(fd, &fdsRead);
		FD_ZERO(&fdsWrite);
		FD_SET(fd, &fdsWrite);

		/* Connect to server */
		sin.sin_family = AF_INET;
		sin.sin_port = htons(usPort);	//smtp port number
		sin.sin_addr.s_addr = *(unsigned long*)*ppc;

		connect(fd,(struct sockaddr *)&sin,sizeof(sin));

		if (select(fd+1, &fdsRead, &fdsWrite, NULL, &tv) > 0)
		{
			if (FD_ISSET(fd, &fdsWrite) || FD_ISSET(fd, &fdsRead))
			{
				int iErrorCode;
				int iErrorCodeLen = sizeof(iErrorCode);
				if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &iErrorCode, &iErrorCodeLen) == 0)
				{
					if (iErrorCode == 0)
					{
						*iOut_fd = fd;
						fcntl(fd, F_SETFL, iFl);
						return 0;
					}
				}
			}
		}

//		fprintf(stderr, "MCTEST:NetConnect:Can not connect %s in %d millsecond.\n", pcServer, iMillSecTimeout);
		close(fd);
	}

	return -1;
}


/*
	communication with IPCam by HTTP
	Return: -1, network error
			0, success
*/
int SendRequest(const char *pcServer,unsigned short usPort,	const char *pcPath,	int flag, const char *buf,
	int len)
{
	char *pcBuf;
	int fd;
	int iLen;
	int rt;
	char *p1;
	int first=1;
	char *base;
	char *str2 = "User = ";

	if ((pcBuf = (char *)malloc(4096)) == NULL) return -1;
	if (NetConnect(pcServer, usPort, 5000, &fd) != 0)
	{
		printf("MCTEST:SendRequest:NetConnect error\n");
		free(pcBuf);
		return -1;
	}
	iLen = sprintf(pcBuf, "GET %s HTTP/1.1\r\n\r\n", pcPath);

	rt = write(fd, pcBuf, iLen);
	if (rt == iLen)
	{
		while ((rt = read(fd, pcBuf, 4096)) > 0)
		{
			pcBuf[rt]='\0';
			//write(1, pcBuf, rt);
			if(flag==1)
			{
				if(first==1)
				{
					first=0;
				
					p1=strstr(pcBuf,"200");
				
					if(p1==NULL)
						break;

					printf("ok response 200\n");
				}
				p1=strstr(pcBuf,str2);
				if(p1!=NULL)
				{
					printf("found user - len is %d\n",len);
					char *p2,*p3;
					p2=p1+strlen(str2);
					p2=strtok(p2,"\n");
					strncpy((char*)buf,p2, len /*p3-p2*/);
					buf[len-1]=0;
					//printf("result %s -- %s\n",buf,p2);
					break;
				}	
			}	
		}
		rt = iLen;
	}
	close(fd);
	free(pcBuf);
	return (rt==iLen?0:-1);
}


/*
	change modem user name
*/
int SetModemName(const char *Cname)
{
	char	ac[256];
	int		ret;
	int		port = VPshort(0x8000-4);

	printf("setting modem name to %s on %d\n",Cname,port);

	sprintf(ac, "/Modem.cgi?User=%s&Action=Set",Cname);
	if(-1==(ret=SendRequest("127.0.0.1", port?port:80, ac,0,NULL,0)))
	{
		port = VPshort(0x8000-2);
		ret=SendRequest("127.0.0.1", port?port:80, ac,0,NULL,0);
	}
	return(ret);
}


/*
	get camera name by HTTP(localhost) 
*/
int GetModemName(char *Cname,U16 len )
{
	int	ret;

	int port = VPshort(0x8000-4);
	if(-1==(ret=SendRequest("127.0.0.1", port?port:80, "/Modem.cgi?Action=GetSetting",1,Cname,len)))
	{
		port = VPshort(0x8000-2);
		ret=SendRequest("127.0.0.1", port?port:80, "/Modem.cgi?Action=GetSetting",1,Cname,len);
	}
	return(ret);
}
#endif



