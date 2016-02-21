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
#include "websocket.h"
#include "izumooyashiro.h"
#include "aoi.h"

#define BUF_LEN 0xFFFF

static enum wsState wsstate = WS_STATE_OPENING;
static struct handshake hs;
static AOI_STR *aoistr;


int izm_webSock_RcvSnd(int acptfd)
{
	int ret = 0;
	uint8_t gBuffer[BUF_LEN];
	uint8_t rcvdata[BUF_LEN];
	uint8_t *indata = NULL;
	uint8_t *convString = NULL;
	size_t indataSize = 0;
	size_t rcvlen;
	size_t frameSize = BUF_LEN;
	enum wsFrameType frameType = WS_INCOMPLETE_FRAME;

	/* cwebsocketのサンプルを参考に実装。ちょっと可読性悪いので後で直す。 */

	memset(gBuffer, 0, BUF_LEN);
	memset(rcvdata, 0, BUF_LEN);

	/* receive & parse */
	rcvlen = recv(acptfd, rcvdata, BUF_LEN, 0);
	if (rcvlen > 0) {
		if (wsstate == WS_STATE_OPENING) {
			printf("Izumo Websock Handshake phase.\n");
			frameType = wsParseHandshake(rcvdata, rcvlen, &hs);
			indataSize = rcvlen;
		}
		else {
			frameType = wsParseInputFrame(rcvdata, rcvlen, &indata, &indataSize);
		}
	}
	else {
		ret = -1;
		if (rcvlen == 0) {
			/* graceful close, if rcvlen is 0. */
			ret = 1;
			wsstate = WS_STATE_OPENING; /* 暫定 */
		}
		close(acptfd);
		return(ret);
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
			send(acptfd, gBuffer, frameSize, 0);
			return(0);
		}
		else {
			wsMakeFrame(NULL, 0, gBuffer, &frameSize, WS_CLOSING_FRAME);
			if (send(acptfd, gBuffer, frameSize, 0) < 0)
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
			/* izumo open ※本来ならオープン処理の戻りを確認してリターンすべきだがとりあえず暫定 */
			fwnnserver_open();
			
			aoistr = aoi_new();

			frameSize = BUF_LEN;
			memset(gBuffer, 0, BUF_LEN);
			wsGetHandshakeAnswer(&hs, gBuffer, &frameSize);
			freeHandshake(&hs);
			if (send(acptfd, gBuffer, frameSize, 0) < 0) {
				fwnnserver_close();
				return(-1);
			}
			wsstate = WS_STATE_NORMAL;
		}
	}
	else {
		if (frameType == WS_CLOSING_FRAME) {
			if (wsstate == WS_STATE_CLOSING) {
				return(0);
			}
			else {
				/* izumo close */
				fwnnserver_close();

				frameSize = BUF_LEN;
				memset(gBuffer, 0, BUF_LEN);
				wsMakeFrame(NULL, 0, gBuffer, &frameSize, WS_CLOSING_FRAME);
				send(acptfd, gBuffer, frameSize, 0);
				return(0);
			}
		}
		/* WS_TEXT_FRAME　通信 */
		else if (frameType == WS_TEXT_FRAME) {
			aoi_input_str(aoistr, indata);
			convString = fwnnserver_kanren(aoistr->workstr);
			frameSize = BUF_LEN;
			memset(gBuffer, 0, BUF_LEN);
			wsMakeFrame(convString, strlen(convString), gBuffer, &frameSize, WS_TEXT_FRAME);
			send(acptfd, gBuffer, frameSize, 0);
        }
    }

	return(0);
}
