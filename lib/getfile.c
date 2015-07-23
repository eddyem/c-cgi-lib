#include "file_functions.h"

/*
 * оптимизировать эту функцию: читать сразу куском bufportion,
 * а потом уже в буфере искать \n или 0
 * если не обнаружено - считывать следующий буфер
 *
 * !!! И КОНТРОЛИРОВАТЬ МАКСИМАЛЬНЫЙ РАЗМЕР СЧИТАННОЙ "СТРОКИ" !!!
 */
ssize_t mygetline(char **buffer, int fd){
	size_t bufportion = 16000, bufsz = bufportion, i = 0;
	free(*buffer); *buffer = NULL;
	char *ptr;
	char *buf;
	buf = malloc(bufsz);
	if(!buf){freeQS(); die(MEMERR);}
	ptr = buf;
	if(read(fd, ptr, 1) != 1){ free(buf); return -1; }
	do{
		if(*ptr == '\n' || *ptr == 0) break;
		ptr++;
		if(++i >= bufsz){
			bufsz += bufportion;
			buf = realloc(buf, bufsz);
			if(!buf){freeQS(); die(MEMERR);}
			ptr = &buf[i];
		}
	}while(read(fd, ptr, 1) == 1);
	*ptr = 0;
	if(ptr > buf && *(ptr-1) == '\r') *(ptr-1) = 0;
	*buffer = strdup(buf);
	free(buf);
	return strlen(*buffer);
}
void hexdump(char *inp, char* outp, bool flag){
	char *i_ptr = inp, *o_ptr = outp;
	unsigned char ch;
	strcpy(outp, "");
	while(*i_ptr){
		ch = *i_ptr++;
		if(ch == '/' && flag){ *o_ptr++ = '/';}
		else
		if(ch > 31 ){
			sprintf(o_ptr, "%%%x", ch);
			o_ptr += 3;
		}
	}
	*o_ptr = 0;
}
void unhexdump(char *inp){
	char tmp[512], *o_ptr = inp, *tok;
	unsigned char ch;
	unsigned int a;
	strncpy(tmp, inp, 512);
	tok = strtok(tmp, "%");
	do{
		sscanf(tok, "%x", &a);
		ch = a;
		*o_ptr++ = ch;
	}while((tok = strtok(NULL, "%")));
	*o_ptr = 0;
}

// замена пробелов в имени файла подчеркиваниями
void remove_spaces(char *name){
	char *ptr = name;
	while((ptr = strchr(ptr, ' ')))
		*ptr++ = '_';
}

// получение смещения на начало данных в multipart-строке запроса (в очередном куске)
// QS модифицируется соответствующим образом (curpos - начало файла, qlen - его длина)
off_t get_data_beginning(){
	DBG("fd=%d, curpos=%lld, qlen=%zd", QS->fd, QS->curpos, QS->qlen);
	if((!qs && !get_qs()) || (QS->fd && !QS->curpos)) return -1;
	off_t offs = 0;
	char *ctpos = strstr(QS->str, "\r\n\r\n");
	if(!ctpos) return -1;
	offs = ctpos - QS->str + 4;
	if(offs > 0 && offs < QS->qlen){
		QS->str = ctpos + 4;
		DBG("Found data");
		if(QS->fd) QS->curpos += offs;
		QS->size -= offs;
		QS->qlen -= offs;
	}else return -1;
/*	char *ctpos = strcasestr(QS->str, "Content-Type");
	if(ctpos) ctpos = strchr(ctpos, '\r');
	if(ctpos) offs = ctpos - QS->str + 4;
	if(offs > 0 && offs < QS->qlen){
		QS->str = ctpos + 4;
		DBG("Found file data");
		if(QS->fd) QS->curpos += offs;
		QS->size -= offs;
		QS->qlen -= offs;
	}else return -1;
*/
	DBG("offs: %lld, curpos: %lld", offs, QS->curpos);
	return QS->curpos;
}

// проверяем, является ли очередной кусок запроса контейнером для файла
// если file_mime/filename не NULL, в них заносится тип и имя файла (не забыть про FREE!)
bool qs_is_file(char **file_mime, char **filename){
	FNAME();
	bool QSisFILE = FALSE;
	char *f_mime = NULL, *f_name = NULL;
	char *cont_disp = get_hdr_name("Content-Disposition");
	do{
		if(!cont_disp || strcasecmp(cont_disp, "form-data")) break;
		if(! (f_mime = get_hdr_name("Content-Type")) ) break;
		if(! (f_name = get_qs_param("filename")) ) break;
		DBG("filename: %s, mime: %s, cd: %s", f_name, f_mime, cont_disp);
		if(f_name) QSisFILE = TRUE;
	}while(0);
	FREE(cont_disp);
	if(!QSisFILE) FREE(f_mime);
	if(file_mime) *file_mime = (f_mime) ? f_mime : NULL;
	else FREE(f_mime);
	if(filename) *filename = (f_name) ? f_name : NULL;
	else FREE(f_name);
	DBG("QSisFILE=%d", QSisFILE);
	return QSisFILE;
}

/*
 * Сохраняем файл в текущей позиции составного запроса (или обычного запроса)
 * Если файлов несколько, для сохранения всех необходимо вызывать несколько раз
 * move_qs_to_next_boundary(), сохраняя каждый файл
 * В случае ошибки функция "вылетает" на die
 */
void save_file(char *dest){
	FNAME();
	char *file_dest=NULL, *file_mime=NULL, *file=NULL;
	int f_file;
	size_t n;
	static const char *destdir = FILEDIR;
	off_t fsize, file_beg;
	if(!qs_is_file(&file_mime, &file)) die(QS_ISNT_A_FILE);
	if( (file_beg = get_data_beginning()) == -1 ){
		FREE(file_mime); FREE(file);
		die(QS_ISNT_A_FILE);
	}
	if(QS->qlen > MAX_FSIZE){
		FREE(file_mime); FREE(file);
		die(FS_TOO_LARGE);
	}
	DBG("File size: %zd", QS->qlen);
	file_dest = CHALLOC(1024);
	if(dest)
		sprintf(file_dest, "%s/%s/%s", destdir, dest, file);
	else
		sprintf(file_dest, "%s/%s", destdir, file);
	DBG("file_dest = %s, destdir=%s", file_dest,dest);
	struct stat st;
	if(stat(file_dest, &st) == 0){
		FREE(file_dest); FREE(file_mime); FREE(file);
		die(FILE_EXISTS);
	}
	if((f_file = open(file_dest, O_RDWR|O_CREAT)) == -1){
		FREE(file_dest); FREE(file_mime); FREE(file);
		DBG("Can't open: %s", strerror(errno));
		die(CANT_SAVE_FILE);
	}
	if(chmod(file_dest, 00666) == -1){
		unlink(file_dest);
		DBG("Can't chmod");
		FREE(file_dest); FREE(file_mime); FREE(file);
		die(CANT_SAVE_FILE);
	}
	fsize = QS->qlen;
	if(QS->fd > 0){ // запрос содержится в файле
		char *buf = CHALLOC(16384);
		DBG("curpos: %lld", QS->curpos);
		if(lseek(QS->fd, QS->curpos, SEEK_SET) == -1){
			FREE(file_dest); FREE(file_mime); FREE(file);
			FREE(buf); unlink(file_dest);
			DBG("Can't lseek");
			die(CANT_SAVE_FILE);
		}
		while((n = read(QS->fd, buf, 16383))){
			if(n > fsize) n = fsize; // чтобы не залезть в чужую область
			if(write(f_file, buf, n) == -1){
				FREE(file_dest); FREE(file_mime); FREE(file);
				FREE(buf); unlink(file_dest);
				DBG("Can't write");
				die(CANT_SAVE_FILE);
			}
			fsize -= n;
			if(fsize < 1) break;
		}
		FREE(buf);
	}else{ // запрос - в памяти
		if(write(f_file, QS->str, fsize) != fsize){
			FREE(file_dest); FREE(file_mime); FREE(file);
			unlink(file_dest);
			DBG("Can't write / bad size");
			die(CANT_SAVE_FILE);
		}
	}
	DBG("file %s saved", file);
	FREE(file_mime); FREE(file);
	if((fsize = lseek(f_file, 0, SEEK_END)) != QS->qlen){
		DBG("bad file size (%lld instead of %zd)", fsize, QS->qlen );
		unlink(file_dest);
		FREE(file_dest);
		die(CANT_SAVE_FILE);
	}
	FREE(file_dest);
	close(f_file);
}

