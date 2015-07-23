#ifndef __AUTH_H__
#define __AUTH_H__
/*#ifndef _XOPEN_SOURCE
	#define _XOPEN_SOURCE 666 // для usleep
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

// ошибки
enum{
	 OK
	,NO_QS				// "Отсутствует строка запроса"
	,NO_LOGIN			// "Не указано имя пользователя"
	,NO_PASSWD			// "Не указан пароль"
	,WRONG_PASSWD		// "Неверная пара имя-пароль"
	,BAD_URL			// "Отсутствует наименование сервиса для регистрации"
	,BAD_IP				// "Не могу определить ваш IP-адрес"
	,NO_DB				// "Повреждена база данных пользователей"
	,NO_REG				// "Вы не зарегистрированы"
	,BAD_SERV			// "Вы не зарегистрированы на этом сервисе"
	// 10
	,SQL_ERR			// "Ошибка базы данных"
	,NO_LEVEL			// "Не указан уровень доступа"
	,BAD_LEVEL			// "Недопустимый уровень доступа"
	,LOGIN_TOO_LONG		// "Длина имени пользователя превышает 128 символов"
	,PASSWD_TOO_LONG	// "Длина пароля превышает 64 символа"
	,USER_EXISTS		// "Пользователь с таким именем уже существует"
	,NO_ID				// "Отсутствует идентификатор сессии"
	,CANT_DELETE_ROOT	// "Нельзя удалить пользователя root"
	,MEMERR				// "Ошибка обращения к памяти"
	,Q_CORRUPT			// "Поврежденный запрос"
	// 20
	,CANT_CREATE_FILE	// "Не могу создать файл"
	,QS_TOO_LARGE		// "Размер запроса слишком велик"
	,FS_TOO_LARGE		// "Размер файла слишком велик"
	,QS_ISNT_A_FILE		// "Запрос не содержит информации о файле"
	,CANT_SAVE_FILE		// "Не могу сохранить файл"
	,FILE_EXISTS		// "Файл существует"
	,UNKNOWN_ERR		// "Неизвестная ошибка"
};

// получение строки запроса
char* get_qs();
// освобождение строки запроса (в принципе, нужды в этой функции нет, кроме служебного применения)
void freeQS();
// получение длины строки запроса
size_t get_qs_len();
// получение длины запроса
size_t get_contentLength();
// получение значения ограничителя
char *get_boundary();
// "переносим qs на начало следующего куска мультизапроса"
char *move_qs_to_next_boundary();
// получение смещения на начало данных в multipart-строке запроса (в очередном куске)
// QS модифицируется соответствующим образом (curpos - начало файла, qlen - его длина)
off_t get_data_beginning();
// получение имени параметра строки с заголовком hdr (для multipart/form-data)
// например, для строки Content-Disposition: form-data; name=... вернет form-data
char* get_hdr_name(char *hdr);
// то же, но для указателя str, а не qs
char* get_hdr_name_from_string(char *hdr, char *str);
// получение параметра из строки запроса == get_param_from_string(param, qs)
char* get_qs_param(char *param);
// то же самое, но из строки string
char* get_param_from_string(char *param, char *string);
// получение текстового сообщения об ошибке с кодом errcode
char *explain_error(int errcode);
/* смерть с сообщением об ошибке errcode
 * !!! Указатель на функцию cgi_die !!!
 * сменить действие по умолчанию можно при помощи функции errFunction
 */
extern void (*die)(int);
void errFunction(void (*fn)(int));
//void die(int errcode);
// получение уровня аутентификации
int get_auth_level(bool dieflag);
// выполнение строки запроса q_str к БД db
char *SQL_request(char *dbname, char *q_str);
// шифрование строки str солью salt
char *sha(char *str, char *salt);
/*
 * ВСЕ СЛЕДУЮЩИЕ ФУНКЦИИ ВОЗВРАЩАЮТ 0 В СЛУЧАЕ УДАЧИ
 * ПРИ НЕУДАЧЕ поведение зависит от АРГУМЕНТА:
 *  	если он ==  TRUE, ПРОГРАММА ЗАВЕРШАЕТСЯ С ОШИБКОЙ
 * 		если он == FALSE, ВОЗВРАЩАЕТ КОД ОШИБКИ
 */
// аутентификация пользователя
int autentification(bool dieflag);
// выход
int clear_auth(bool dieflag);
// добавление пользователя
int useradd(bool dieflag);
// удаление пользователя
int userdel(bool dieflag);
// изменение пользователя
int usermod(bool dieflag);
// отображение списка пользователей
int lsusers();


// получение значения параметра param из кук
char* get_cookie_val(char *param);

/*
 * Функции для работы с файлами
 */
// проверка: есть ли в строке запроса параметр "filename" (т.е. передается ли файл)
// если file_mime/filename не NULL, в них заносится тип и имя файла (не забыть про FREE!)
bool qs_is_file(char **file_mime, char **filename);
// раскодировать строку (результат заносится в ту же строку inp)
void unhexdump(char *inp);
// закодировать строку inp в outp, если flag==true, слеши не кодируются (путь)
void hexdump(char *inp, char* outp, bool flag);
// сохраняет файл, полученный из POST-пересылки, в директорию dest
void save_file(char *dest);
/*
 * Работа с локалью
 */
// установка локали
void set_locale(char *text_domain, char *localedir);
// восстановление предыдущей локали
void restore_locale();
#endif // __AUTH_H__
