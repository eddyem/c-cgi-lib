/*
 * в приложении, которое должно работать с аутентификацией,
 * из библиотеки используется только int get_auth_level(),
 * однако, можно использовать и другие функции (для работы со
 * строками запроса и т.п.)
 */

#include "CGI_auth.h"
#include "web_functions.h"
#include "file_functions.h"
#include <sys/time.h>	// для time
#include <math.h>		// для floor

#define SQLexec(query) SQL_request(db_path, query)

const int Ntry_max = 100; // кол-во попыток с паузами по 50мс, в течение которых
// будем пытаться открыть заблокированную базу

static char *db_path = DBNAME;
/*
 * atoi с выводом ошибки
 */
static int atolevl(char *str){
	char *eptr = NULL;
	long ret;
	errno = 0;
	ret = strtol(str, &eptr, 10);
	if(errno == ERANGE || ret >= INT_MAX || ret < 0 || eptr == str ||
			(errno != 0 && ret == 0))
		die(BAD_LEVEL);
	return (int)ret;
}

/*
 * Сообщения об ошибках
 */
static char *ErrMessages[] = {
	// "OK"
	"OK",
	// "Отсутствует строка запроса"
	N_("No query string"),
	// "Не указано имя пользователя"
	N_("No user name"),
	// "Не указан пароль"
	N_("No password"),
	// "Неверная пара имя-пароль"
	N_("Wrong login/password"),
	// "Отсутствует наименование сервиса для регистрации"
	N_("Bad service URL"),
	// "Не могу определить ваш IP-адрес"
	N_("Can't define your IP"),
	// "Повреждена база данных пользователей"
	N_("Users' database corrupted"),
	// "Вы не зарегистрированы"
	N_("You aren't registered"),
	// "Вы не зарегистрированы на этом сервисе"
	N_("You aren't registered in this service"),
	// "Ошибка базы данных"
	N_("Database error"),
	// "Не указан уровень доступа"
	N_("No level"),
	// "Недопустимый уровень доступа"
	N_("Bad level"),
	// "Длина имени пользователя превышает 128 символов"
	N_("User's name length is more than 128 symbols"),
	// "Длина пароля превышает 64 символа"
	N_("User's password length is more than 64 symbols"),
	// "Пользователь с таким именем уже существует"
	N_("Login busy"),
	// "Отсутствует идентификатор сессии"
	N_("No session ID"),
	// "Нельзя удалить пользователя root"
	N_("You can't delete user root"),
	// "Ошибка обращения к памяти"
	N_("Memory error"),
	// "Поврежденный запрос"
	N_("Corrupted query"),
	// "Не могу создать файл"
	N_("Can't create file"),
	// "Размер запроса слишком велик"
	N_("Query size is too large"),
	// "Размер файла слишком велик"
	N_("File size is too large"),
	// "Запрос не содержит информации о файле"
	N_("Query has no information about a file"),
	// "Не могу сохранить файл"
	N_("Can't save file"),
	// "Файл существует"
	N_("File exists"),
	// "Неизвестная ошибка"
	N_("Unknown error"),
};

/*
 * Преобразование сообщения об ошибке errcode в соответствии с локалью
 */
char *explain_error(int errcode){
	char *msg;
	set_locale(NULL, NULL);
	errcode = abs(errcode);
	msg = _(ErrMessages[errcode]);
	restore_locale();
	return msg;
}

/*
 * Функция вывода ошибки по умолчанию
 */
void cgi_die(int errcode){
	// "Ошибка! %s.\n"
	printf(_("Error! %s.\n"), explain_error(errcode));
	fprintf(stderr, "Error %d\n", errcode);
	exit(errcode);
}
// Указатель на функцию вывода ошибки
void (*die)(int) = cgi_die;

/*
 * Переопределение стандартной функции вывода ошибки
 * на функцию пользователя
 */
void errFunction(void (*fn)(int)){
	die = fn;
}

// Буфер для ответа sqlite
static char *SQLbuf = NULL;
/*
 * Функция-отклик sqlite, преобразующая массив строк ответа
 * в одну строку с разделением ответов точкой с запятой
 */
static int callback(void *NotUsed __attribute__((unused)),
					int argc,
					char **argv,
					char **azColName __attribute__((unused))){
	if(!*argv) return 1;
	int i;
	if(SQLbuf){
		SQLbuf = realloc(SQLbuf, strlen(SQLbuf)+2);
		strcat(SQLbuf, "\n");
	}
	for(i = 0; i < argc; i++){
		if(SQLbuf){
			SQLbuf = realloc(SQLbuf, strlen(SQLbuf)+strlen(*argv)+3);
			if(i) strcat(SQLbuf, ";");
			strcat(SQLbuf, *argv++);
		}
		else
			SQLbuf = strdup(*argv++);
	}
	return 0;
}
/*
typedef struct UnlockNotification UnlockNotification;
struct UnlockNotification{
	int fired;				// True after unlock event has occured
	pthread_cond_t cond;	// Condition variable to wait on
	pthread_mutex_t mutex;	// Mutex to protect structure
};
static void unlock_notify_cb(void **apArg, int nArg){
	int i;
	for(i=0; i<nArg; i++){
		UnlockNotification *p = (UnlockNotification *)apArg[i];
		pthread_mutex_lock(&p->mutex);
		p->fired = 1;
		pthread_cond_signal(&p->cond);
		pthread_mutex_unlock(&p->mutex);
	}
}
static int wait_for_unlock_notify(sqlite3 *db){
	int rc;
	UnlockNotification un;
	// Initialize the UnlockNotification structure.
	un.fired = 0;
	pthread_mutex_init(&un.mutex, 0);
	pthread_cond_init(&un.cond, 0);
	// Register for an unlock-notify callback.
	rc = sqlite3_unlock_notify(db, unlock_notify_cb, (void *)&un);
	if( rc!=SQLITE_LOCKED && rc!=SQLITE_OK )
			die(SQL_ERR);
	if(rc==SQLITE_OK){
		pthread_mutex_lock(&un.mutex);
		if(!un.fired)
			pthread_cond_wait(&un.cond, &un.mutex);
		pthread_mutex_unlock(&un.mutex);
	}
	pthread_cond_destroy(&un.cond);
	pthread_mutex_destroy(&un.mutex);
	return rc;
}
*/
/*
 * запрос q_str к базе dbname
 * возвращает NULL, либо динамически выделенную область памяти с ответом
 * !!! Возвращаемое удалять free() !!!
 */
char *SQL_request(char *dbname, char *q_str){
	sqlite3 *db;
	char *zErrMsg = NULL, *ret = NULL;
	int rc = sqlite3_open(dbname, &db), Ntry = 0;
	if(rc) die(NO_DB);
	free(SQLbuf); SQLbuf = NULL;
	//sprintf(sqlstr, ".timeout 5000;\n %s", q_str);
	//rc = wait_for_unlock_notify(db);
	//DBG("UNLOCK: rc=%d", rc);
	do{
		rc = sqlite3_exec(db, q_str, callback, NULL, &zErrMsg);
		if(rc){
			if((rc == SQLITE_BUSY || rc == SQLITE_LOCKED) && Ntry < Ntry_max){ // база заблокирована другим вызовом
				Ntry++;
				DBG("ooops, DB is busy, try number %d", Ntry);
				usleep(5000); // ждем 5мс
				continue; // пробуем снова
			}
			fprintf(stderr, "SQL error: %s, %d\n", zErrMsg, rc);
			die(SQL_ERR);
		}
		else if(SQLbuf)
			ret = strdup(SQLbuf);
		break;
	}while(Ntry < Ntry_max);
	sqlite3_close(db);
	//free(sqlstr);
	DBG("db=%s, reqest=%s, answer=___%s___",dbname, q_str, ret);
	free(SQLbuf); SQLbuf = NULL;
	return ret;
}

/*
 * возвращает хеш пароля и уровень доступа для сессии по логину login,
 * либо NULL
 * level в случае ошибки равен 0
 * !!! Возвращаемое удалять free() !!!
 */
static char *get_passwd(char *login, int *level, char **URL){
	char *q_str = (char*)calloc(512, 1), *passw=NULL, *tmp=NULL;
	snprintf(q_str, 512, "select pass from users where login=\"%s\";", login);
	if((passw = SQLexec(q_str))){
		snprintf(q_str, 512, "select level from users where login=\"%s\";", login);
		if(level){
			if(!(tmp = SQLexec(q_str))) *level = -NO_REG;
			else *level = atolevl(tmp);
			free(tmp);
		}
		if(URL){
			snprintf(q_str, 512, "select URL from users where login=\"%s\";", login);
			if(!(tmp = SQLexec(q_str))) *URL = NULL;
			else *URL = strdup(tmp);
			free(tmp);
		}
	}
	free(q_str);
	return passw;
}

/*
 * Для заданного url возвращает путь к сервису (для кук),
 * либо NULL (в случае ошибки)
 * Флаг sequre устанавливается в TRUE, если запрос по https
 * !!! Возвращаемое удалять free() !!!
 */
char *getpath(char *url, bool *sequre){
	DBG("url: %s", url);
	char *ptr, *ptr1;
	ptr = strchr(url, '/'); // ищем первый / в http://...
	if(!ptr) return NULL;
	int n = (int)(ptr - url);
	DBG("n=%d, url=%s", n, url);
	if(n == 6 && strncmp(url, "https", 5) == 0) *sequre = TRUE;
	else *sequre = FALSE;
	ptr1 = strchr(ptr+1, '/'); // ищем второй слеш
	if(!ptr1) return NULL;
	ptr = strchr(ptr1+1, '/'); // ищем слеш - окончание имени домена
	if(!ptr){
		ptr1 = strdup("/");
	}else
		ptr1 = strdup(ptr);
	return ptr1;
}

/*
 * sha-кодирование строки str солью salt
 * !!! Возвращаемое удалять free() !!!
 */
char *sha(char *str, char *salt){
	FNAME();
	char *tmp = NULL, *crypt_str = NULL;
	tmp = crypt(str, salt);
	crypt_str = strdup(strrchr(tmp, '$') + 1);
	// tmp удалять не надо, т.к. функция crypt эту область выделяет статически
	return crypt_str;
}

/* инициализация генератора псевдослучайных чисел */
static void urandom_ini(){
	double tt, mx = (double)LONG_MAX;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tt = tv.tv_sec*1e6 + ((double)tv.tv_usec);
	srand48((long)(tt - mx * floor(tt/mx)));
}

/* В следующих функциях в зависимости от аргумента dieflag
 * в случае ошибки выполняется функция die, если dieflag == TRUE,
 * либо функция возвращает код ошибки со знаком "-", если dieflag == FALSE
 */
int rtn;
#define die_or_err(arg)	do{if(dieflag) die(arg); else {rtn = -arg; goto endoffn;}}while(0)

// аутентификация пользователя (возвращает 0 в случае удачи)
int autentification(bool dieflag){
	FNAME();
	DBG("db_path=%s",db_path);
	char *IP, *URL=NULL, *URL_ORI=NULL, *id=NULL,
		*login=NULL, *pass_raw=NULL, *pass_ori=NULL,
		*pass=NULL, *key=NULL, *salt=NULL, *tmp=NULL;
	rtn = OK;
	char *q_str = (char*)calloc(512, 1);
	bool sequre;
	salt = calloc(128, 1);
	IP = getenv("REMOTE_ADDR");
	if(!IP) die_or_err(BAD_IP);
	URL = get_qs_param("URL");
	id = get_qs_param("ID");
	if(!id) die_or_err(NO_ID);
	if(!(login = get_qs_param("login"))) die_or_err(NO_LOGIN);
	if(!(pass_raw = get_qs_param("passwd"))) die_or_err(NO_PASSWD);
	pass = sha(pass_raw, "$6$pass-enc$");
	if(!(pass_ori = get_passwd(login, &rtn, &URL_ORI))) die_or_err(WRONG_PASSWD);
	DBG("login: %s, level: %d, URL: %s, pass: %s, pass_real: %s",
		login, rtn, URL, pass, pass_ori);
	if(strcmp(pass_ori, pass) || rtn < 0) die_or_err(WRONG_PASSWD);
	if(strcmp(URL_ORI, "/") && rtn){ // пользователю разрешен доступ только к одному сервису
		if(!URL || !strstr(URL_ORI, URL)) // в перечне URL_ORI текущий URL не найден
			die_or_err(BAD_URL);
	}
	urandom_ini();
//	snprintf(salt, 127, "$5$%s$", IP);
//	id = sha(login, salt);
	snprintf(salt, 127, "$5$.%ld.$", lrand48());
	key = sha(id, salt);
	snprintf(q_str, 511, "delete from keyid where time < julianday('now')-366;");
	tmp = SQLexec(q_str);
	free(tmp); tmp = NULL;
	snprintf(q_str, 511, "delete from keyid where key=\"%s\";", key);
	tmp = SQLexec(q_str);
	free(tmp);
	tmp = getpath(URL, &sequre);
	if(!tmp) die_or_err(BAD_URL);
	printf("KEY=%s; path=%s%s", key, tmp, (sequre ? "; secure\n" : "\n"));
	free(tmp); tmp = NULL;
	snprintf(q_str, 511, "insert into keyid values(\"%s\",\"%s\",\"%s\",julianday('now'));", key, login, id);
	tmp = SQLexec(q_str);
endoffn:
	free(URL); free(URL_ORI); free(id);
	free(login); free(pass_raw); free(pass_ori); free(pass);
	free(key); free(salt); free(tmp);
	return rtn;
}

// выход (возвращает 0 в случае удачи)
int clear_auth(bool dieflag){
	FNAME();
	rtn = OK;
	char *id, *q_str=NULL;
	id = getenv("HTTP_REFERER");
	if(!id) die_or_err(NO_ID);
	q_str = (char*)calloc(511, 1);
	snprintf(q_str, 511, "delete from keyid where id=\"%s\";", id);
	SQLexec(q_str);
	printf("Exit");
endoffn:
	free(q_str);
	return rtn;
}

/*
 * возвращает уровень доступа пользователя к данному ресурсу,
 * либо -код ошибки
 */
int get_auth_level(bool dieflag){
	FNAME();
	char *URL_ORI=NULL, *URL, *login=NULL, *key=NULL, *q_str=NULL, *p=NULL;
	rtn = -NO_REG;
	do{
		URL = getenv("REQUEST_URI");
		if(!URL){
			rtn = -BAD_URL;
			break;
		}
		key = get_cookie_val("key");
		if(!key){
			rtn = -BAD_SERV;
			break;
		}
		q_str = (char*)calloc(512, 1);
		snprintf(q_str, 511, "select login from keyid where key=\"%s\"", key);
		if((login = SQLexec(q_str))){
			if(!(p = get_passwd(login, &rtn, &URL_ORI)))
				die_or_err(NO_REG);
			DBG("URL=%s, URL_ORI=%s", URL, URL_ORI);
			if(strcmp(URL_ORI, "/") && rtn){ // пользователю разрешен доступ только к одному сервису
				if(!strstr(URL_ORI, URL)) // в перечне URL_ORI текущий URL не найден
					die_or_err(BAD_SERV);
			}
		}else
			die_or_err(NO_REG);
	}while(0);
endoffn:
	free(URL_ORI); free(login); free(key); free(q_str); free(p);
	return rtn;
}

/*
 * АДМИНИСТРИРОВАНИЕ
 */
// добавление пользователя
int useradd(bool dieflag){
	char *login=NULL, *pass_raw=NULL, *pass=NULL, *q_str=NULL, *levl=NULL, *URL=NULL;
	int level;
	rtn = OK;
	get_qs();
	if(!(login = get_qs_param("user")))
		die_or_err(NO_LOGIN);
	if((pass = get_passwd(login, NULL, NULL)))
		die_or_err(USER_EXISTS);
	free(pass); pass = NULL;
	if(!(pass_raw = get_qs_param("passwd")))
		die_or_err(NO_PASSWD);
	if(!(levl = get_qs_param("level")))
		die_or_err(NO_LEVEL);
	if(!(URL = get_qs_param("URL")))
		URL = strdup("/");
	if(strlen(login) > 128)
		die_or_err(LOGIN_TOO_LONG);
	if(strlen(pass_raw) > 64)
		die_or_err(PASSWD_TOO_LONG);
	level = atolevl(levl);
	pass = sha(pass_raw, "$6$pass-enc$");
	q_str = (char*)calloc(1024, 1);
	snprintf(q_str, 1023, "insert into users values(\"%s\",\"%s\",%d, \"%s\");", login, pass, level, URL);
	SQLexec(q_str);
endoffn:
	free(login); free(pass_raw); free(pass); free(q_str); free(levl); free(URL);
	return rtn;
}

// удаление пользователя
int userdel(bool dieflag){
	char *login=NULL, *q_str = (char*)calloc(512, 1);
	rtn = OK;
	if(!(login = get_qs_param("user")))
		die_or_err(NO_LOGIN);
	if(!strcmp(login, "root"))
		die_or_err(CANT_DELETE_ROOT);
	snprintf(q_str, 511, "delete from users where login=\"%s\";", login);
	SQLexec(q_str);
endoffn:
	free(q_str);
	free(login);
	return rtn;
}

// изменение пользователя
int usermod(bool dieflag){
	char *login=NULL, *pass_raw=NULL, *levl=NULL, *pass=NULL,
		*URL=NULL, *q_str=NULL;
	int level;
	rtn = OK;
	if(!(login = get_qs_param("user")))
		die_or_err(NO_LOGIN);
	pass_raw = get_qs_param("passwd");
	if(pass_raw && *pass_raw != ' ')
		pass = sha(pass_raw, "$6$pass-enc$");
	else
		pass = get_passwd(login, &level, &URL);
	levl = get_qs_param("URL");
	if(levl){
		free(URL); URL = levl;
		levl = NULL;
	}
	levl = get_qs_param("level");
	if(levl) level = atolevl(levl);
	q_str = (char*)calloc(1024, 1);
	snprintf(q_str, 511, "delete from users where login=\"%s\";", login);
	SQLexec(q_str);
	snprintf(q_str, 1023, "insert into users values(\"%s\",\"%s\",%d,\"%s\");", login, pass, level, URL);
	SQLexec(q_str);
endoffn:
	free(login); free(pass_raw); free(levl); free(pass); free(q_str); free(URL);
	return rtn;
}

int lsusers(){
	char *q_str = NULL, *list = NULL;
	//char *plist[] = {"login", "level", "URL"};
	//int i;
	q_str = calloc(512, 1);
	snprintf(q_str, 511, "select login,level,URL from users order by pass;");
	list = SQLexec(q_str);
	printf("%s\n\n",list);
	free(list);
	free(q_str);
	return OK;
}
