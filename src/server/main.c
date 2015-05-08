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
/* #include <sys/socket.h> */
#include "izumooyashiro.h"

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

int main(int argc, char *argv[])
{
	int optno, status;

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
	if (status >= 0)
		izm_accept();

	while (status >= 0) {
		status = izm_webSock_RcvSnd();
	}

	printf("Goodbye, kasuga.\n");

	return(0);
}
