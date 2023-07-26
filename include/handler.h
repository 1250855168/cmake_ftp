
#ifndef __HANDLER_H__
#define __HANDLER_H__

void handle_connection(int conn);

void handle_cmd_ls(int conn);

void handle_cmd_get(int conn, char *filename);

void handle_ftp_cmd(int conn, unsigned char *cmd, int len);

#endif