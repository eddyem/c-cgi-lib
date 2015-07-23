#include <CGI_auth.h>

/*
 * ������ ��� ����������� ��� ������ �� �������
 */

int main(int argc __attribute__((unused)), char **argv __attribute__((unused))){
	char *qs_param = NULL;
	int l;
	printf("Access-Control-Allow-Origin: http://" SERVER_NAME "\nAcess-Control-Allow-Methods: POST\nContent-type:text/html\n\n");
	if(!get_qs()) die(NO_QS);
	if(get_qs_param("login")) // ��������������
		return autentification(TRUE);
	if(get_qs_param("exit")){ // �����
		int r = clear_auth(FALSE);
		if(r) explain_error(r);
		return r;
	}
	if((qs_param = get_qs_param("admin"))){  // �����������������
		if((l = get_auth_level(FALSE))){ // ������������ - �� ���
			if(l > 0){
				printf("You are not root\n");
				die(WRONG_PASSWD);
			}
			else
				die(l);
		}
		if(strcmp(qs_param, "useradd") == 0) // ���������� ������������
			return useradd(TRUE);
		if(strcmp(qs_param, "userdel") == 0) // �������� ������������
			return userdel(TRUE);
		if(strcmp(qs_param, "usermod") == 0) // �������������� ������������
			return usermod(TRUE);
		if(strcmp(qs_param, "lsusers") == 0) // ����������� ������ �������������
			return lsusers(TRUE);
	}
	die(NO_QS);
}

