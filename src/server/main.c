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
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

char gDbgMode = 0;

enum {
	SOCK_OTHER_ERROR = -2,
	SOCK_BIND_ERROR,
	SOCK_OK, /* This is '0'. (dummy) */
};

/* 仮 */
#define IR_SERVICE_NAME "izumo-fwnn"
#define IR_DEFAULT_PORT 8860


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

#if 0 /* 未実装 */
static int open_inet_socket()
{
	struct sockaddr_in insock;
	struct servent *sp;
	int retry = 0, request, oldflags;
	int status = SOCK_OTHER_ERROR;

	request = socket(AF_INET, SOCK_STREAM, 0);
	if (request  < 0) {
		ir_debug( Dmsg(5,"Warning: INET socket for server failed.\n"));
	}
	else {

#ifdef SO_REUSEADDR
		int one = 1;
		setsockopt(request, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));
#endif
		bzero ((char *)&insock, sizeof (insock));
		insock.sin_family = AF_INET;
		insock.sin_addr.s_addr = htonl(INADDR_ANY);

		/* ポート番号を取得 */
		sp = getservbyname(IR_SERVICE_NAME ,"tcp");
		if (sp != NULL) {
			insock.sin_port = sp->s_port;
		}
		else {
			insock.sin_port = htons(IR_DEFAULT_PORT);
		}
		ir_debug( Dmsg(5, "INET PORT NO:[%d]\n",insock.sin_port));

		if (bind(request, (struct sockaddr *)&insock, sizeof(insock)) < 0) {
			ir_debug( Dmsg(5,"Warning: Server could not bind.\n"));
			status = SOCK_BIND_ERROR;
			close(request);
			goto last;
		}

		if (listen (request, 5) < 0) {
			ir_debug( Dmsg(5,"Warning: Server could not listen.\n"));
			close(request);
			goto last;
		}

		status = request;
	}

last:
	return(status);
}
#endif

int main(int argc, char *argv[])
{
	int optno;

	while ((optno = getopt(argc, argv, "d::") != -1)) {
		switch (optno) {
		case 'd':
			/* debug mode */
			gDbgMode = 1;
			break;
		}
	}

	/* become daemon, if not debug mode */
	if (gDbgMode == 0)
		izm_daemon();


	printf("Hello, kasuga.\n");

	return(0);
}
