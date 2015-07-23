#include <CGI_auth.h>

/*
 * скрипт для авторизации или выхода из системы
 */

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))){
	char *qs_param = NULL;
	int l;
	printf("Access-Control-Allow-Origin: http://" SERVER_NAME "\nAcess-Control-Allow-Methods: POST\nContent-type:text/html\n\n");
	if(!get_qs()) die(NO_QS);
	if(get_qs_param("login")) // аутентификация
		return autentification(TRUE);
	if(get_qs_param("exit")){ // выход
		int r = clear_auth(FALSE);
		if(r) explain_error(r);
		return r;
	}
	if((qs_param = get_qs_param("admin"))){  // администрирование
		if((l = get_auth_level(FALSE))){ // пользователь - не рут
			if(l > 0){
				printf("You are not root\n");
				die(WRONG_PASSWD);
			}
			else
				die(l);
		}
		if(strcmp(qs_param, "useradd") == 0) // добавление пользователя
			return useradd(TRUE);
		if(strcmp(qs_param, "userdel") == 0) // удаление пользователя
			return userdel(TRUE);
		if(strcmp(qs_param, "usermod") == 0) // редактирование пользователя
			return usermod(TRUE);
		if(strcmp(qs_param, "lsusers") == 0) // отображение списка пользователей
			return lsusers(TRUE);
	}
	die(NO_QS);
}

