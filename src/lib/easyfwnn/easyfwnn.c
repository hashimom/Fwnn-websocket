/* Copyright (c) 2014-2015 Masahiko Hashimoto <hashimom@geeko.jp>
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
#include <stdint.h>
#include <iconv.h>
#include "wnn/jslib.h"

#include "easyfwnn.h"

#define FZK_FILE		"pubdic/full.fzk"
#define KIHON_DIC		"pubdic/kihon.dic"
#define SETTO_DIC		"pubdic/setsuji.dic"
#define CHIMEI_DIC	"pubdic/chimei.dic"
#define JINMEI_DIC	"pubdic/jinmei.dic"
#define KOYUU_DIC		"pubdic/koyuu.dic"
#define BIO_DIC		"pubdic/bio.dic"
#define PC_DIC		"pubdic/computer.dic"
#define SYMBOL_DIC	"pubdic/symbol.dic"

#define MAX_CONV_STRLEN	(4096)
#define CONV_UTF82EUC	(0)
#define CONV_EUC2UTF8	(1)

static WNN_JSERVER_ID *jserver;
static WNN_ENV *wnnenv;
static struct wnn_ret_buf wnnbuf= {0, NULL};

static void set_wnn_env_pram()
{
	struct wnn_param pa;
	pa.n= 2;		/* n_bun */
	pa.nsho = 10;	/* nshobun */
	pa.p1 = 2;		/* hindoval */
	pa.p2 = 40;	/* lenval */
	pa.p3 = 0;		/* jirival */
	pa.p4 = 100;	/* flagval */
	pa.p5 = 5;		/* jishoval */
	pa.p6 = 1;		/* sbn_val */
	pa.p7 = 15;	/* dbn_len_val */
	pa.p8 = -20;	/* sbn_cnt_val */
	pa.p9 = 0;		/* kan_len_val */

	js_param_set(wnnenv, &pa);
}

static void exec_iconv(const char *instr, const char *outstr, int ch)
{
	iconv_t cd;
	size_t src_len = strlen(instr);
	size_t dest_len = MAX_CONV_STRLEN - 1;
	
	if (ch == CONV_UTF82EUC)
		cd = iconv_open("EUC-JP", "UTF-8");
	else
		cd = iconv_open("UTF-8", "EUC-JP");
	
	iconv(cd, (char **)&instr, &src_len, (char **)&outstr, &dest_len);
	iconv_close(cd);
}

static void strtows(w_char *u, unsigned char *e)
{
	int x;
	for(;*e;){
		x= *e++;
		if(x & 0x80)
			x = (x << 8)  | *e++;
		*u++= x;
	}
	*u=0;
}

static int putws(unsigned short *s, char *outstr)
{
	int ret = 0;
	
	while(*s) {
		outstr[ret] = *s >> 8;
		outstr[ret + 1] =  *s;
		ret = ret + 2;
		s++;
	}
	return ret;
}

int fwnnserver_open()
{
	int ret = -1;
	int fzkfile;

	jserver = js_open(NULL, 0);
	if (jserver != NULL) {
		wnnenv = js_connect(jserver, "kana");
		ret = js_isconnect(wnnenv);
		if (ret == 0) {
			wnnbuf.buf = (char *)malloc((unsigned)(wnnbuf.size = 0));
			set_wnn_env_pram();
			
			fzkfile = js_file_read(wnnenv, FZK_FILE);
			js_fuzokugo_set(wnnenv, fzkfile);
			
			fwnnserver_adddic(KIHON_DIC);
			fwnnserver_adddic(SETTO_DIC);
			fwnnserver_adddic(CHIMEI_DIC);
			fwnnserver_adddic(JINMEI_DIC);
			fwnnserver_adddic(KOYUU_DIC);
			fwnnserver_adddic(BIO_DIC);
			fwnnserver_adddic(PC_DIC);
			fwnnserver_adddic(SYMBOL_DIC);
		}
	}
	
	return ret;
}

int fwnnserver_close()
{
	int ret = -1;
	
	if (js_disconnect(wnnenv) == 0) {
		ret = js_close(jserver);
		if (wnnbuf.buf)
			free(wnnbuf.buf);
	}
	return ret;
}

int fwnnserver_adddic(char *dicfilename)
{
	int dicno = -1;
	
	dicno = js_file_read(wnnenv, dicfilename);
	if (dicno != -1) {
		js_dic_add(wnnenv, dicno, -1,
				WNN_DIC_ADD_NOR,1,WNN_DIC_RDONLY, 
				WNN_DIC_RDONLY, NULL, NULL);
	}
	
	return dicno;
}

int fwnnserver_kanren(uint8_t *yomi, uint8_t *kanren)
{
	// とりあえずサイズ決め打ち
	char yomi_euc[MAX_CONV_STRLEN];
	w_char upstrings[MAX_CONV_STRLEN];
	int count = 0;
	int i, tmpbuf_len;
	char kanstr_tmp[MAX_CONV_STRLEN];
	char kanstr_utf8[MAX_CONV_STRLEN];
	struct wnn_sho_bunsetsu  *sbn;
	struct wnn_dai_bunsetsu *dlist;
	
	memset(yomi_euc, 0, MAX_CONV_STRLEN);
	memset(upstrings, 0, MAX_CONV_STRLEN);
	
	exec_iconv(yomi, yomi_euc, CONV_UTF82EUC);
	strtows(upstrings, yomi_euc);
	
	count = js_kanren(wnnenv, upstrings, WNN_ALL_HINSI, NULL,
			WNN_VECT_KANREN, WNN_VECT_NO, WNN_VECT_BUNSETSU,&wnnbuf);

	if (count > 1) {
		dlist = (struct wnn_dai_bunsetsu *)wnnbuf.buf;
		for ( ; count > 0; dlist++, count --) {
			sbn = dlist->sbn;
			for (i = dlist->sbncnt; i > 0; i--) {
				/* 単文節ごとに解析 */
				memset(kanstr_tmp, '\0', MAX_CONV_STRLEN);
				memset(kanstr_utf8, '\0', MAX_CONV_STRLEN);
				tmpbuf_len = putws(sbn->kanji, kanstr_tmp);
				tmpbuf_len = putws(sbn->fuzoku, kanstr_tmp + tmpbuf_len);

				/* 文字コード変換して格納 */
				exec_iconv(kanstr_tmp, kanstr_utf8, CONV_EUC2UTF8);
				strncat(kanren, kanstr_utf8, strlen(kanstr_utf8));
				sbn++;
			}
		}
	}
	else {
		/* 文節数が1以下のためそのまま返す */
		strncpy(kanren, yomi, strlen(yomi));
	}

	return(strlen(kanren));
}


