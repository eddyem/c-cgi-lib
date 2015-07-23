#include <CGI_auth.h>

/*
 * скрипт для тестирования работы авторизации
 */


void printMsg(char *msg){
	printf("Content-type: text/html\n\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=koi8-r\">\n<html>\n");
	if(msg)
		printf("<body onLoad='parent.Pass.message(\"%s!\");'>\n%s</body></html>\n", msg, msg);
	else
		printf("<body onLoad='parent.Pass.onLoadFile();'></body></html>\n");
}

void printErr(int errNo){
	if(errNo){
		char *locerr = explain_error(errNo);
		printMsg(locerr);
	}else
		printMsg(NULL);
	if(errNo) fprintf(stderr, "Error %d\n", errNo);
	exit(errNo);
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))){
	char *qs, *qs_param = NULL, *msg = NULL;
	char *filename;
	int level;
	errFunction(printErr); // меняем функцию вывода ошибки
	printf("Access-Control-Allow-Origin: http://" SERVER_NAME "\nAcess-Control-Allow-Methods: POST\nContent-type:text/html\n\n");
	level = get_auth_level(FALSE);
	if((qs = get_qs())){
		do{
			if(qs_is_file(NULL, &filename)){
				save_file(NULL);
			}else{
				//printf("Content-type: text/html\n\n");
				if((qs_param = get_qs_param("test"))){
					printf("<b>auth_test: your auth level is %d</b><br>", level);
					printf("You clicked button test<br>\n");
					free(qs_param);
					size_t l = get_qs_len();
					if(get_contentLength() > l) qs[l] = 0;
					printf("All request: %s", qs);
				}else if((qs_param = get_qs_param("name"))){
					if(!strcmp(qs_param, "param") && get_data_beginning() != -1){
						char *param = get_qs();
						fprintf(stderr, "========================== param=%s  (%c)\n", param, *param);
						if(param && *param != '1'){
							msg = calloc(128, 1);
							snprintf(msg, 127, "You choose param %s", param);
							fprintf(stderr, "%s\n", msg);
						}
						free(qs_param);
					}
				}
			}
		}while( (qs = move_qs_to_next_boundary()) );
		if(!msg) printErr(0);
		else{
			char *tmp = malloc(128);
			snprintf(tmp, 127, ", file %s is loaded", filename);
			free(filename);
			strncat(msg, tmp, 127);
			free(tmp);
			printMsg(msg);
		}
	}else{
		//printf("Content-type: text/html\n\n");
		printf("<b>auth_test: your auth level is %d</b><br>", level);
	}
	return 0;
}

