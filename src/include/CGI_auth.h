#ifndef __AUTH_H__
#define __AUTH_H__
/*#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 666 // ��� usleep
#endif
#ifndef _LARGEFILE64_SOURCE
	#define _LARGEFILE64_SOURCE
#endif*/
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#ifndef _FILE_OFFSET_BITS
	#define _FILE_OFFSET_BITS 64
#endif


#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <libintl.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef DBNAME
	#define DBNAME "/var/www/SSL/cgi-bin/user-pass"
#endif

#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#define CHALLOC(x)	(char *)calloc(x, 1)
#define FREE(x)		do{free(x); x = NULL;}while(0)

// ������
enum{
	 OK
	,NO_QS				// "����������� ������ �������"
	,NO_LOGIN			// "�� ������� ��� ������������"
	,NO_PASSWD			// "�� ������ ������"
	,WRONG_PASSWD		// "�������� ���� ���-������"
	,BAD_URL			// "����������� ������������ ������� ��� �����������"
	,BAD_IP				// "�� ���� ���������� ��� IP-�����"
	,NO_DB				// "���������� ���� ������ �������������"
	,NO_REG				// "�� �� ����������������"
	,BAD_SERV			// "�� �� ���������������� �� ���� �������"
	// 10
	,SQL_ERR			// "������ ���� ������"
	,NO_LEVEL			// "�� ������ ������� �������"
	,BAD_LEVEL			// "������������ ������� �������"
	,LOGIN_TOO_LONG		// "����� ����� ������������ ��������� 128 ��������"
	,PASSWD_TOO_LONG	// "����� ������ ��������� 64 �������"
	,USER_EXISTS		// "������������ � ����� ������ ��� ����������"
	,NO_ID				// "����������� ������������� ������"
	,CANT_DELETE_ROOT	// "������ ������� ������������ root"
	,MEMERR				// "������ ��������� � ������"
	,Q_CORRUPT			// "������������ ������"
	// 20
	,CANT_CREATE_FILE	// "�� ���� ������� ����"
	,QS_TOO_LARGE		// "������ ������� ������� �����"
	,FS_TOO_LARGE		// "������ ����� ������� �����"
	,QS_ISNT_A_FILE		// "������ �� �������� ���������� � �����"
	,CANT_SAVE_FILE		// "�� ���� ��������� ����"
	,FILE_EXISTS		// "���� ����������"
	,UNKNOWN_ERR		// "����������� ������"
};

// ��������� ������ �������
char* get_qs();
// ������������ ������ ������� (� ��������, ����� � ���� ������� ���, ����� ���������� ����������)
void freeQS();
// ��������� ����� ������ �������
size_t get_qs_len();
// ��������� ����� �������
size_t get_contentLength();
// ��������� �������� ������������
char *get_boundary();
// "��������� qs �� ������ ���������� ����� �������������"
char *move_qs_to_next_boundary();
// ��������� �������� �� ������ ������ � multipart-������ ������� (� ��������� �����)
// QS �������������� ��������������� ������� (curpos - ������ �����, qlen - ��� �����)
off_t get_data_beginning();
// ��������� ����� ��������� ������ � ���������� hdr (��� multipart/form-data)
// ��������, ��� ������ Content-Disposition: form-data; name=... ������ form-data
char* get_hdr_name(char *hdr);
// �� ��, �� ��� ��������� str, � �� qs
char* get_hdr_name_from_string(char *hdr, char *str);
// ��������� ��������� �� ������ ������� == get_param_from_string(param, qs)
char* get_qs_param(char *param);
// �� �� �����, �� �� ������ string
char* get_param_from_string(char *param, char *string);
// ��������� ���������� ��������� �� ������ � ����� errcode
char *explain_error(int errcode);
/* ������ � ���������� �� ������ errcode
 * !!! ��������� �� ������� cgi_die !!!
 * ������� �������� �� ��������� ����� ��� ������ ������� errFunction
 */
extern void (*die)(int);
void errFunction(void (*fn)(int));
//void die(int errcode);
// ��������� ������ ��������������
int get_auth_level(bool dieflag);
// ���������� ������ ������� q_str � �� db
char *SQL_request(char *dbname, char *q_str);
// ���������� ������ str ����� salt
char *sha(char *str, char *salt);
/*
 * ��� ��������� ������� ���������� 0 � ������ �����
 * ��� ������� ��������� ������� �� ���������:
 *  	���� �� ==  TRUE, ��������� ����������� � �������
 * 		���� �� == FALSE, ���������� ��� ������
 */
// �������������� ������������
int autentification(bool dieflag);
// �����
int clear_auth(bool dieflag);
// ���������� ������������
int useradd(bool dieflag);
// �������� ������������
int userdel(bool dieflag);
// ��������� ������������
int usermod(bool dieflag);
// ����������� ������ �������������
int lsusers();


// ��������� �������� ��������� param �� ���
char* get_cookie_val(char *param);

/*
 * ������� ��� ������ � �������
 */
// ��������: ���� �� � ������ ������� �������� "filename" (�.�. ���������� �� ����)
// ���� file_mime/filename �� NULL, � ��� ��������� ��� � ��� ����� (�� ������ ��� FREE!)
bool qs_is_file(char **file_mime, char **filename);
// ������������� ������ (��������� ��������� � �� �� ������ inp)
void unhexdump(char *inp);
// ������������ ������ inp � outp, ���� flag==true, ����� �� ���������� (����)
void hexdump(char *inp, char* outp, bool flag);
// ��������� ����, ���������� �� POST-���������, � ���������� dest
void save_file(char *dest);
/*
 * ������ � �������
 */
// ��������� ������
void set_locale(char *text_domain, char *localedir);
// �������������� ���������� ������
void restore_locale();
#endif // __AUTH_H__
