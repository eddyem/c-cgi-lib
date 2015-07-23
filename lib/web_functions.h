#ifndef __COOKIES_H__
#define __COOKIES_H__
#include <CGI_auth.h>

// режим отладки, -DEBUG
#ifdef EBUG
	#define RED			"\033[1;32;41m"
	#define GREEN		"\033[5;30;42m"
	#define OLDCOLOR	"\033[0;0;0m"
	#define FNAME() fprintf(stderr, "\n%s (%s, line %d)\n", __func__, __FILE__, __LINE__)
	#define DBG(...) do{fprintf(stderr, "%s (%s, line %d): ", __func__, __FILE__, __LINE__); \
					fprintf(stderr, __VA_ARGS__);			\
					fprintf(stderr, "\n");} while(0)
	#define __GET__
#else
	#define FNAME()	 do{}while(0)
	#define DBG(...) do{}while(0)
#endif //EBUG

#define _(String)				gettext(String)
#define gettext_noop(String)	String
#define N_(String)				gettext_noop(String)

#ifndef MAX_MEMORY_FOR_QUERY
	#define MAX_MEMORY_FOR_QUERY 1048576 // 1МБ
#endif
#ifndef FILEDIR
	#define FILEDIR "/tmp"
#endif
#ifndef MAX_QSIZE // максимальный размер запроса
	#define MAX_QSIZE (1LL << 30) // 1ГБ
#endif
#ifndef MAX_FSIZE // максимальный размер файла
	#define MAX_FSIZE (1LL << 30) // 1ГБ
#endif
// веб-запрос
typedef struct{
	char *str;		// указатель на запрос или его текущий кусок между разделителями
	size_t size;	// strlen(str) или MaxQsBufsize, если весь запрос в буфер не влез
	int fd;			// файловый дескриптор, куда помещается запрос, если он слишком велик
	off_t curpos;	// текущая позиция в файле, если fd != 0
	size_t qlen;	// длина текущего куска (от curpos до [положение разделителя]-2)
}QueryString;

extern QueryString *QS;
extern char *qs;

char *move_qs_to_next_boundary();
off_t get_boundary_position(off_t *from, bool *last);
extern ssize_t mygetline(char **buffer, int fd);

#endif // __COOKIES_H__
