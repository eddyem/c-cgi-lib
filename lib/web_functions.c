#include "web_functions.h"
const size_t MaxQsBufsize = MAX_MEMORY_FOR_QUERY;

//extern char **environ;
char *qs = NULL; // сюда помещается весь "нефильтрованный" запрос
static char *boundary = NULL;
static off_t boundaryLen = 0;
static char *contentType = NULL;
QueryString *QS = NULL;
size_t contentLength = 0;

size_t get_qs_len(){
	return QS->size;
}
size_t get_contentLength(){
	if(!qs) get_qs();
	return contentLength;
}

char *get_boundary(){ return boundary;}

void freeQS(){
	if(!QS) return;
	if(QS->fd) close(QS->fd);
	FREE(qs);
	FREE(QS);
}

// поиск needle в haystack без учета нулевых байтов в haystack
// строка считается оконченной на eptr
char *mystrstr(char *haystack, char *needle, char *eptr){
	if(!haystack || !needle) return NULL;
	char frstLtr = *needle, *ret = NULL;
	size_t l = strlen(needle);
	eptr -= l;
	do{
		if(*haystack != frstLtr) continue;
		if(!strncmp(haystack, needle, l)) ret = haystack;
	}while(++haystack < eptr && !ret);
	return ret;
}

// переносим qs на начало следующего куска мультизапроса
char *move_qs_to_next_boundary(){
	FNAME();
	static off_t off_begin = 0; // смещение, указывающее на начало очередного разделителя
	static bool last = FALSE;
	char *ptr;
	if(!QS->fd && !qs) return NULL;
	if(last || (size_t)off_begin >= contentLength) return NULL;
	DBG("boundary: %s", boundary);
	if(!boundary){ // запрос не multipart
		if(QS->str) return NULL; // запрос уже был обслужен
		if(!QS->fd){ // запрос - в буфере памяти
			QS->str = qs;
			DBG("qs: %s", QS->str);
			QS->size = contentLength;
			QS->qlen = contentLength;
		}else{ // запрос в файле
			QS->str = CHALLOC(MaxQsBufsize+1);
			if(!QS->str){freeQS(); die(MEMERR);}
			QS->size = read(QS->fd, QS->str, MaxQsBufsize);
			if(QS->size != MaxQsBufsize){ freeQS(); die(Q_CORRUPT); }
			QS->curpos = 0;
			QS->qlen = contentLength;
		}
	}else{ // multipart
		if(off_begin == 0) off_begin = boundaryLen + 2;
		if(!QS->fd){ // запрос - в буфере памяти
			ptr = qs + off_begin;
			if(ptr > qs + contentLength){last = TRUE; return NULL;}
			DBG("Off: %lld", off_begin);
			char *eptr = mystrstr(ptr, boundary, qs+contentLength);
			if(!eptr){DBG("ptr: %s", ptr);last = TRUE; return NULL;}
			off_begin = eptr - qs + boundaryLen + 2;
			DBG(" ");
			if(off_begin < 0 || (size_t)off_begin > contentLength){
				last = TRUE; return NULL;
			}
			if(*(eptr+boundaryLen) == '-') last = TRUE; // последний разделитель
			QS->qlen = QS->size = eptr - ptr - 2; // опускаем \r\n в конце последней строки
			DBG("LENGTH OF QUERY PART: %zd, POSITION: %zd", QS->qlen, ptr-qs);
			if(QS->size < 1) return NULL;
			QS->str = ptr;
		}else{ // запрос в файле
			off_t oldbeg = off_begin;
			off_t endpos = get_boundary_position(&off_begin, &last);
			DBG("endpos: %lld, off_beg: %lld (%lld)", endpos, off_begin, oldbeg);
			if(endpos == -1){ DBG("OOOPS"); last = TRUE; return NULL; }
			size_t qlen = (size_t)(endpos - oldbeg);
			QS->qlen = qlen;
			DBG("qlen: %zd", qlen);
			if(lseek(QS->fd, oldbeg, SEEK_SET) == -1){
				freeQS(); die(Q_CORRUPT);}
			if(qlen > MaxQsBufsize) qlen = MaxQsBufsize;
			DBG("qlen: %zd", qlen);
			FREE(qs);
			qs = CHALLOC(qlen+1);
			if(!qs){freeQS(); die(MEMERR);}
			QS->str = qs;
			QS->size = read(QS->fd, QS->str, qlen);
			if(QS->size != qlen){ freeQS(); die(Q_CORRUPT); }
			lseek(QS->fd, oldbeg, SEEK_SET);
			QS->curpos = oldbeg;
		}
	}
	#ifdef EBUG
	if((ptr = strchr(QS->str, '\n'))){
		char *ttt=CHALLOC(ptr-QS->str+1);
		strncpy(ttt, QS->str, ptr-QS->str);
		DBG("1st str: %s", ttt);
		FREE(ttt);
	}
	#endif
	DBG("fd=%d, curpos=%lld, qlen=%zd", QS->fd, QS->curpos, QS->qlen);
	return QS->str;
}

// положение ограничителя в файле, начиная с позиции from (вдруг несколько файлов?)
// если from - не NULL, туда записывается положение начала следующего куска данных
// (после разделителя)
off_t get_boundary_position(off_t *from, bool *last){
	FNAME();
	off_t ret = -1, offs = 0, ofrom = 0, oldpos;
	ssize_t ll;
	char *line = NULL;
	*last = FALSE;
	if(!boundary) return -1;
	size_t l = boundaryLen;
	if(from) ofrom = offs = *from;
	oldpos = lseek(QS->fd, 0, SEEK_CUR);
	if(oldpos == -1) return -1;
	if(lseek(QS->fd, ofrom, SEEK_SET) == -1) return -1;
	while( ((ll = mygetline(&line, QS->fd)) != -1) ){
		if(strncmp(boundary, line, l) == 0){
			ret = offs - 2;
			DBG("line: ||%s||", line);
			if((size_t)ll > l && line[l] == '-') // последний разделитель
				*last = TRUE;
			break;
		}
		offs = lseek(QS->fd, 0, SEEK_CUR);
		FREE(line);
	}
	if(from) *from = lseek(QS->fd, 0, SEEK_CUR);
	lseek(QS->fd, oldpos, SEEK_SET); // восстанавливаем предыдущую позицию
	if(ret < ofrom) ret = -1;
	DBG("DATA PORTION (%slast) ENDS AT: %lld", (*last?"":"not "), ret);
	return ret;
}

void read_query(){
	char *m;
	ssize_t n;
	QS = calloc(1, sizeof(QueryString));
	if(!QS) die(MEMERR);
	// если размер содержимого не больше MAX_MEMORY_FOR_QUERY, помещаем
	// его в память, иначе - в файл
	if((m = getenv("REQUEST_METHOD")) && strcasecmp(m, "POST") == 0){
		if(contentLength <= MaxQsBufsize){ // в буфер
			qs = CHALLOC(contentLength+1);
			if(!qs){freeQS(); die(MEMERR);}
			QS->size = read(0, qs, contentLength);
			if(QS->size != contentLength){freeQS(); die(Q_CORRUPT);}
		}else{ // в файл
			char *tmp_file = CHALLOC(512);
			snprintf(tmp_file, 511, "%s/newfile_XXXXXX", FILEDIR);
			mkstemp(tmp_file);
			if( (QS->fd = open(tmp_file, O_RDWR|O_CREAT)) == -1){
				FREE(QS); FREE(tmp_file); die(CANT_CREATE_FILE);
			}
			unlink(tmp_file);
			FREE(tmp_file);
			qs = CHALLOC(MaxQsBufsize+1);
			// пишем
			while((n = read(0, qs, MaxQsBufsize))){
				if( (n < 0) || (write(QS->fd, qs, n) != n) ){
					freeQS(); die(CANT_CREATE_FILE);
				}
			}
			FREE(qs);
			size_t l = lseek(QS->fd, 0, SEEK_END);
			DBG("l=%zd", l);
			if(l != contentLength){freeQS(); die(Q_CORRUPT);}
		}
		move_qs_to_next_boundary();
	}else{
#ifdef __GET__
		if( (qs = getenv("QUERY_STRING")) )
			QS->str = qs;
			QS->size = contentLength;
#endif
	}
	DBG("QS len = %d", QS->size);
	//DBG("qs=%s", QS->str);
}

char* get_qs(){
	FNAME();
	if(QS) return QS->str;
	/*char **ptr1 = environ;
	while(*ptr1){
		DBG("env: %s", *(ptr++));
	}*/
	char *ptr = getenv("CONTENT_TYPE");
	if(!ptr) return NULL;
	contentType = strdup(ptr);
	ptr = strchr(contentType, ';');
	if(ptr){
		*ptr++ = 0;
		boundary = get_param_from_string("boundary", ptr);
		if(boundary){
			int l = strlen(boundary)+3;
			char *tmp = CHALLOC(l);
			snprintf(tmp, l, "--%s", boundary);
			free(boundary);
			boundary = tmp;
			boundaryLen = strlen(boundary);
		}
	}
	DBG("Content-type=%s; boundary=%s", contentType, boundary);
	// на случай, если запрос больше размера буфера qs
	ptr = getenv("CONTENT_LENGTH");
	if(ptr) contentLength = atoll(ptr);
	else return NULL;
	if(contentLength > MAX_QSIZE) die(QS_TOO_LARGE);
	read_query();
	DBG("CONTENT_LENGTH=%zu", contentLength);
	if(QS) return QS->str;
	else return NULL;
}

char* get_hdr_name(char *hdr){
	if(!qs && !get_qs()) return NULL;
	QS->str[QS->size] = 0;
	return get_hdr_name_from_string(hdr, QS->str);
}

char* get_hdr_name_from_string(char *hdr, char *str){
	FNAME();
	if(!str) return NULL;
	char *ptr = str, *ret = NULL;
	int sl = strlen(hdr);
	while(ptr && *ptr){
		int i;
		if((i=strncasecmp(hdr, ptr, sl)) == 0) break;
		ptr = strchr(ptr, '\n');
		if(ptr && *ptr) ptr++;
	}
	if(ptr && (ptr = strchr(ptr, ':')) && *ptr && ++ptr && *ptr){
		char *ptr1=NULL, *ptr2=NULL; size_t l;
		while(*ptr == ' ') ptr++;
		ptr1 = strchr(ptr, ';');
		ptr2 = strchr(ptr, '\r');
		if(!ptr2) ptr2 = strchr(ptr, '\n');
		if(ptr2){
			if(!ptr1) ptr1 = ptr2;
			else if(ptr1 > ptr2) ptr1 = ptr2;
		}
		if(ptr1) l = (size_t)(ptr1 - ptr);
		else l = strlen(ptr);
		ret = CHALLOC(l+1);
		memcpy(ret, ptr, l);
	}
	DBG("hdr=%s, val=%s_=====", hdr, ret);
	return ret;
}

char* get_qs_param(char *param){
	if(!qs && !get_qs()) return NULL;
	return get_param_from_string(param, QS->str);
}

char* get_param_from_string(char *param, char *string){
	DBG("par=%s", param);
	char *tok, *val, *par, *str, *str0 = NULL, *ret = NULL;
	int l;
	if(!param || !string) return NULL;
	str0 = strdup(string);
	str = str0;
	tok = strtok(str, "& ;\n");
	do{
		if(strcasecmp(tok, param)==0){
			ret = strdup(" "); // переменная есть, значения нет
			break;
		}
		if((val = strchr(tok, '=')) == NULL) continue;
		*val++ = '\0';
		par = tok;
		if(strcasecmp(par, param)==0){
			if(strlen(val) > 0)
				ret = strdup(val);
			else
				ret = strdup(" "); // переменная есть, значения нет
			break;
		}
	}while((tok = strtok(NULL, "& ;\n"))!=NULL);
	FREE(str0);
	if(!ret) return NULL;
	l = strlen(ret);
	// выкидываем ненужные кавычки и \r
	if(l > 0 && *ret == '"'){
		str = strdup(ret+1);
		free(ret);
		ret = str;
		l--;
	}; l--;
	if(l > 0 && ret[l] == '\r'){ ret[l] = 0; l--; }
	if(l > 0 && ret[l] == '"') ret[l] = 0;
	DBG("param = %s, value = %s", param, ret);
	return ret;
}

char* get_cookie_val(char *param){
	FNAME();
	char *tok, *val, *par, *ret = NULL, *cookie, *tmp, *tmp0 = NULL;
	cookie = getenv("HTTP_COOKIE");
	if(!cookie){
		DBG("NO cookie");
		return NULL;
	}
	tmp = tmp0 = strdup(cookie);
	tok = strtok(tmp, "; \n");
	do{
		if((val = strchr(tok, '=')) == NULL) continue;
		*val++ = '\0';
		par = tok;
		if(strcasecmp(par, param)==0){
			if(strlen(val) > 0)
				ret = strdup(val);
			break;
		}
	}while((tok = strtok(NULL, "; \n"))!=NULL);
	FREE(tmp0);
	DBG("cookie: key=%s, val=%s", param, ret);
	return ret;
}

static char *olddomain = NULL, *olddir = NULL, *lang = NULL;
void setloc(){
	if(lang) return;
	lang = getenv("HTTP_ACCEPT_LANGUAGE");
	if(!lang) return;
	if(lang && (strncmp(lang, "ru", 2) == 0)){
		DBG("RU");
		setlocale(LC_ALL, "ru_RU.koi8-r");
		setlocale(LC_NUMERIC, "C");
	}else
		setlocale(LC_ALL, "C");
}
void set_locale(char *text_domain, char *localedir){
	setloc();
	if(!text_domain) text_domain = GETTEXT_PACKAGE;
	if(!localedir) localedir = LOCALEDIR;
	if( (olddomain = strdup(textdomain(NULL))) )
		olddir = strdup(bindtextdomain(olddomain, NULL));
	textdomain(text_domain);
	bindtextdomain(text_domain, localedir);
	DBG("MSG domain: %s, MSG dir: %s", text_domain, localedir);
	//bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
}

void restore_locale(){
	if(!olddomain) return;
	textdomain(olddomain);
	if(olddir) bindtextdomain(olddomain, olddir);
	FREE(olddomain);
	FREE(olddir);
}
