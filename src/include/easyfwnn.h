/* vim:set et sts=4: */
#ifndef __FWNN_H__
#define __FWNN_H

int fwnnserver_open();
int fwnnserver_close();
int fwnnserver_adddic(char *dicfilename);
int fwnnserver_kanren(uint8_t *yomi, uint8_t *kanren);

#endif
