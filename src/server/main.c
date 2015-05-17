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
#include <string.h>
#include <netdb.h>
#include <syslog.h>
#include "izumooyashiro.h"

/* 仮（config.hを作成するまで） */
#define IR_SERVICE_NAME "izumo-fwnn"
#define IR_DEFAULT_PORT 8860

enum {
	SOCK_OTHER_ERROR = -2,
	SOCK_BIND_ERROR,
	SOCK_OK, /* This is '0'. (dummy) */
};

static int inetSockfd = -1;
static int acptSockfd = -1;

/* char gDbgMode = 0; */
/* 後で直す */
char gDbgMode = 1;


static int izm_daemon()
{
	int ret = -1;

	switch (fork()) {
	case -1:
		return(ret);

	case 0:
		break;

	default:
		exit(0);
	}

	/* ここでsyslog設定、及び子プロセスのユーザを変更する（未実装） */

	return(getpid());
}

static int izm_open_inet_socket()
{
	struct sockaddr_in insock;
	struct servent *sp;
	int retry = 0, oldflags;
	int status = SOCK_OTHER_ERROR;

	inetSockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (inetSockfd >= 0) {

#ifdef SO_REUSEADDR
		int one = 1;
		setsockopt(inetSockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));
#endif
		bzero((char *)&insock, sizeof(insock));
		insock.sin_family = AF_INET;
		insock.sin_addr.s_addr = INADDR_ANY;

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

static int izm_accept()
{
	int clilen;
	struct sockaddr_in client;

	clilen = sizeof(client);
	acptSockfd = accept(inetSockfd, (struct sockaddr *)&client, &clilen);
	if (acptSockfd < 0)
		perror("accept");

	return(acptSockfd);
}

int main(int argc, char *argv[])
{
	int optno, status, con;
	int acptfd;

	while ((optno = getopt(argc, argv, "d") != -1)) {
		switch (optno) {
		case 'd':
			/* debug mode */
			gDbgMode = 1;
			break;
		}
	}

	printf("Hello, kasuga.\n");

	/* become daemon, if not debug mode */
	if (gDbgMode == 0)
		izm_daemon();

	status = izm_open_inet_socket();

	/* accept処理はマルチスレッド対応時に何とかしたい */
	while (status >= 0) {
		con = -1;
		acptfd = izm_accept();
		if (acptfd >= 0)
			con = 0;

		while (con == 0) {
			con = izm_webSock_RcvSnd(acptfd);
		}
	}

	printf("Goodbye, kasuga.\n");

	return(0);
}
