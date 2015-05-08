/* Copyright (c) 2015 Masahiko Hashimoto <hashimom@geeko.jp>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <syslog.h>
/* #include <sys/socket.h> */
#include "websocket.h"
#include "izumooyashiro.h"

enum {
	SOCK_OTHER_ERROR = -2,
	SOCK_BIND_ERROR,
	SOCK_OK, /* This is '0'. (dummy) */
};

#define BUF_LEN 0xFFFF



static int inetSockfd = -1;
static int acptSockfd = -1;
static enum wsState wsstate = WS_STATE_OPENING;
static struct handshake hs;


/* 仮（config.hを作成するまで） */
#define IR_SERVICE_NAME "izumo-fwnn"
#define IR_DEFAULT_PORT 8860

int izm_open_inet_socket()
{
	struct sockaddr_in insock;
	struct servent *sp;
	int retry = 0, oldflags;
	int status = SOCK_OTHER_ERROR;

	inetSockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (inetSockfd  < 0) {
		/* error */
	}
	else {

#ifdef SO_REUSEADDR
		int one = 1;
		setsockopt(inetSockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));
#endif
		bzero((char *)&insock, sizeof(insock));
		insock.sin_family = AF_INET;
		insock.sin_addr.s_addr = htonl(INADDR_ANY);

		/* ポート番号を取得 */
		sp = getservbyname(IR_SERVICE_NAME ,"tcp");
		if (sp != NULL)
			insock.sin_port = sp->s_port;
		else
			insock.sin_port = htons(IR_DEFAULT_PORT);

		if (bind(inetSockfd, (struct sockaddr *)&insock, sizeof(insock)) < 0) {
			status = SOCK_BIND_ERROR;
			close(inetSockfd);
			goto last;
		}

		if (listen (inetSockfd, 5) < 0) {
			close(inetSockfd);
			goto last;
		}

		/* いずれはfdではなくセッションIDを返すようにする*/
		status = inetSockfd;
	}

last:
	return(status);
}

int izm_accept()
{
	int clilen;
	struct sockaddr_in client;

	clilen = sizeof(client);
	acptSockfd = accept(inetSockfd, (struct sockaddr *)&client, &clilen);
	if (acptSockfd < 0) {
		perror("accept");
		return(-1);
	}
	return(0);
}


int izm_webSock_RcvSnd()
{
	uint8_t gBuffer[BUF_LEN];
	uint8_t rcvdata[BUF_LEN];
	uint8_t *indata = NULL;
	size_t indataSize = 0;
	size_t rcvlen;
	size_t frameSize = BUF_LEN;
	enum wsFrameType frameType = WS_INCOMPLETE_FRAME;;

	/* cwebsocketのサンプルを参考に実装。ちょっと可読性悪いので後で直す。 */

	memset(gBuffer, 0, BUF_LEN);
	memset(rcvdata, 0, BUF_LEN);

	/* receive & parse */
	rcvlen = recv(acptSockfd, rcvdata, BUF_LEN, 0);
	if (wsstate == WS_STATE_OPENING) {
		printf("Izumo Websock Handshake phase.\n");
		frameType = wsParseHandshake(rcvdata, rcvlen, &hs);
		indataSize = rcvlen;
	}
	else {
		frameType = wsParseInputFrame(rcvdata, rcvlen, &indata, &indataSize);
    }

	/* Error */
	if ((frameType == WS_INCOMPLETE_FRAME && indataSize >= BUF_LEN) || frameType == WS_ERROR_FRAME) {
		if (frameType == WS_INCOMPLETE_FRAME)
			printf("Warning: buffer too small.\n");
		else
			printf("Error: in incoming frame.\n");

		frameSize = BUF_LEN;
		memset(gBuffer, 0, BUF_LEN);
		if (wsstate == WS_STATE_OPENING) {
			frameSize = sprintf((char *)gBuffer,
					"HTTP/1.1 400 Bad Request\r\n"
					"%s%s\r\n\r\n",
					versionField,
					version);
			send(acptSockfd, gBuffer, frameSize, 0);
			return(0);
		}
		else {
			wsMakeFrame(NULL, 0, gBuffer, &frameSize, WS_CLOSING_FRAME);
			if (send(acptSockfd, gBuffer, frameSize, 0) < 0)
				return(0);
			wsstate = WS_STATE_CLOSING;
        }
    }

	/* make frame */
	if (wsstate == WS_STATE_OPENING) {
		if (frameType != WS_OPENING_FRAME) {
			printf("Warning: Cant Handshake IzumoWebSock.\n");
			return(-1);
		}
		if (frameType == WS_OPENING_FRAME) {
#if 0 /* ここでURLをパースするらしい。が、現状特に利用していないのでパス */
			// if resource is right, generate answer handshake and send it
			if (strcmp(hs.resource, "/echo") != 0) {
				frameSize = sprintf((char *)gBuffer, "HTTP/1.1 404 Not Found\r\n\r\n");
				send(acptsock, gBuffer, frameSize, 0);
				return(-1);
			}
#endif
			frameSize = BUF_LEN;
			memset(gBuffer, 0, BUF_LEN);
			wsGetHandshakeAnswer(&hs, gBuffer, &frameSize);
			freeHandshake(&hs);
			if (send(acptSockfd, gBuffer, frameSize, 0) < 0)
				return(0);
			wsstate = WS_STATE_NORMAL;

			/* izumo open */

		}
	}
	else {
		if (frameType == WS_CLOSING_FRAME) {
			if (wsstate == WS_STATE_CLOSING) {
				return(0);
			}
			else {
				frameSize = BUF_LEN;
				memset(gBuffer, 0, BUF_LEN);
				wsMakeFrame(NULL, 0, gBuffer, &frameSize, WS_CLOSING_FRAME);
				send(acptSockfd, gBuffer, frameSize, 0);
				return(0);
			}
		}
		/* WS_TEXT_FRAME　通信 */
		else if (frameType == WS_TEXT_FRAME) {
			/* この辺りに変換処理を追加する */
			uint8_t *recievedString = NULL;
			recievedString = malloc(indataSize+1);
			if (recievedString != NULL) {
				memcpy(recievedString, indata, indataSize);
				recievedString[indataSize] = 0;

				frameSize = BUF_LEN;
				memset(gBuffer, 0, BUF_LEN);
				wsMakeFrame(recievedString, indataSize, gBuffer, &frameSize, WS_TEXT_FRAME);
				free(recievedString);
				send(acptSockfd, gBuffer, frameSize, 0);
			}
        }
    }

	return(0);
}
