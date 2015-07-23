// move this file to the root html directory
// change const's EXURL & PASSURL
/*
 * �������������:
 * Pass.OnAuth(fn): ��������� ������� fn ����������� ����� ����� ������� ��������������,
 * Pass.OnExit(fn): ��������� ������� fn ����������� ����� ����� ������,
 * Pass.checkcookie(): ���������, ���� �� ���� � ������ KEY - ���� ���, ���������� false,
 * Pass.submit(): �������� �����/������
 * Pass.OnLoad(fn): ��������� �������, ����������� ��� �������� ����� �� ������
 * 		Pass.onLoadFile() ��������� �� (����������� �������� �� �������� iframe'�)
 * Pass.getcookie(arg): �������� �������, �����������, ���� ����, ���� ��������������
 * 		� ���������� ��� ��������� (���� ��� ����) ��� ���������� ��� ��� ���� �������,
 * Pass.message(mesg, [color]): ������� ��������� mesg ������ color (��� �������)
 */
Pass = function(){
const CGI_PATH="https://"+window.location.hostname+"/cgi-bin/auth";
var msgTmout, reqnmbr=0;
var bgdiv=null, frm=null;
var c_path=new Array(), URL=new Array();
var fn_on_auth_OK=null; // ������������ �������, ������������� ����� ��������������
var fn_on_auth_EX=null; // ������������ �������, ������������� ����� ������
var fn_on_fileLoad=null;// �������, ������������� ��� �������� ����� �� ������
/*
 * ���������� �������
 */
function OnAuth(fn){
	fn_on_auth_OK = fn;
}
function OnExit(fn){
	fn_on_auth_EX = fn;
}
function OnLoad(fn){
	fn_on_fileLoad = fn;
}
function onLoadFile(){
	if(fn_on_fileLoad) fn_on_fileLoad();
}
function checkcookie(){
	var txt = document.cookie;
	if(!$('inout')) addInOutBtn();
	if(txt.length==0 || txt.indexOf('KEY')<0){
		$("inout").innerHTML = "�����";
		return false;
	}
	else{
		$("inout").innerHTML = "�����";
		return true;
	}
}
/*	��� ���������� - ��� ������� ��������,
	������ �������� - ���. "��������"
*/
function getcookie(){
	function getpath(str){
		var a = str.split('//');
		if(a.length == 1) return str;
		a = a[1].split("/");
		a[0] = '';
		return a.join('/');
	}
	var i;
	if(URL.length == 0){
		URL[0] = document.location.href;
		for(i = 0; i < getcookie.arguments.length; i++){
			URL[URL.length] = getcookie.arguments[i];
			c_path[c_path.length] = getpath(getcookie.arguments[i]);
		}
	}else reqnmbr = 0;
	if(!checkcookie()) addFGdiv();
	else if(fn_on_auth_OK) fn_on_auth_OK();
}
function subm(id){
	var str, str1, i;
	var login = $("login").value;
	var pass = $("passwd").value;
	if(id){
		$(id).focus();
		if(id == "passwd"){
			$("passwd").value = "";
			return;
		}
	}
	if(login == "" || pass == "") return;
	var str = "login=" + login + " passwd=" + pass;
	for(i = 0; i < URL.length; i++){
		str1 = str + " URL=" + URL[i] + " ID=" + URL[0];
		sendrequest(str1, onAuthOK);
	}
}
/*
 * ��������� �������
 */
function $(id){ return document.getElementById(id);
}
function sendrequest(req_STR, _onOK){
	var request = new XMLHttpRequest(), timeout_id;
	request.open("POST", CGI_PATH, true);
	request.setRequestHeader("Accept-Charset", "koi8-r");
	request.overrideMimeType("multipart/form-data; charset=koi8-r");
	request.onreadystatechange=function(){
		if (request.readyState == 4){
			if (request.status == 200){
				clearTimeout(timeout_id);
				_onOK(request);
			}
			else{
				clearTimeout(timeout_id);
				ch_status("������ �������� �������. ���������� ��� ���.");
			}
		}
	}
	request.send(req_STR);
	timeout_id = setTimeout(function(){ch_status("������� �������� �������. ���������� ��� ���.");}, 3000);
}
function addInOutBtn(){
	var div = document.createElement('div'), s = div.style;
	div.id = "inout";
	s.position = "fixed"; s.top = "10px"; s.right = "10px";
	s.cursor = "pointer"; s.fontWeight = "bold";
	div.addEventListener("click", inout, true);
	document.body.appendChild(div);
}
function addFGdiv(url){
	bgdiv = document.createElement("div");
	frm = document.createElement("div");
	document.body.appendChild(bgdiv);
	var s = bgdiv.style;
	s.position="absolute";
	s.top=0; s.left=0;
	s.width="100%"; s.height="100%";
	s.backgroundColor="Grey";
	s.opacity=0.5;
	s=frm.style;
	s.display="table";s.width="100%"; s.height="100%";s.position="absolute";
	s.top=0; s.left=0;
	frm.id="passfrm";
	var inner = document.createElement("div");
	s = inner.style;
	s.display="table-cell"; s.verticalAlign="middle"; s.textAlign="center";
	inner.innerHTML =
		"<div style='padding: 5px; border: 1px solid; display: inline-block; background-color: lightGrey;'>" +
		"���:<br><input type=text id='login' onchange='Pass.submit(\"passwd\");'><br>" +
		"������:<br><input type=password id='passwd' onchange='Pass.submit(\"login\");'><br><br>" +
		"<button onclick='Pass.submit();'>OK</button></div>"
	document.body.appendChild(frm);
	frm.appendChild(inner);
	$('login').focus();
}
function rmFGdiv(){
	if(!checkcookie()) window.location.reload();
	if(fn_on_auth_OK) fn_on_auth_OK();
	if(!frm || !bgdiv) return;
	document.body.removeChild(bgdiv);
	document.body.removeChild(frm);
}
function onAuthOK(request){
	var txt = request.responseText;
	if(txt.indexOf("KEY") != 0){
		ch_status(txt);
		return;
	}
	var n = txt.indexOf('\n');
	if(n) txt = txt.substring(0, n);
	var d = new Date();
	d.setFullYear(d.getFullYear() + 1); // ���� �������� ���� - 1 ���
	txt += "; expires="+d.toGMTString();
	document.cookie = txt;
	if(++reqnmbr == URL.length) rmFGdiv();
}
function onEX(req){
	var reply = req.responseText;
	if(reply != "Exit"){
		alert(reply);
		return;
	}
	var d = new Date(1), i;
	var str = "KEY=; expires="+d.toGMTString()+"; path=";
	document.cookie = str + document.location.pathname;
	for(i = 0; i < c_path.length; i++){
		document.cookie = str + c_path[i];
	}
	checkcookie();
	if(fn_on_auth_EX) fn_on_auth_EX();
}
function inout(){
	if(checkcookie()) sendrequest("exit", onEX);
	else getcookie();
}
function ch_status(txt, bgcolor){
	function rmmsg(){clearTimeout(msgTmout);document.body.removeChild($("_msg_div"));}
	clearTimeout(msgTmout);
	if(!bgcolor) bgcolor = "red";
	if(!$("_msg_div")){ // ��������� ���� ��� ������ ���������
		var div = document.createElement('div'), s = div.style;
		div.id = "_msg_div";
		s.position = "fixed"; s.top = "10px"; s.left = "0px";
		s.fontWeight = "bold";
		s.padding = "10px";
		s.width = "100%"; s.textAlign = "center";
		document.body.appendChild(div);
	};
	$("_msg_div").style.backgroundColor = bgcolor;
	$("_msg_div").innerHTML = txt.replace("\n", "<br>");
	$("_msg_div").addEventListener("click", rmmsg, true)
	msgTmout = setTimeout(rmmsg, 5000);
}
return{
	OnAuth: OnAuth,
	OnExit: OnExit,
	OnLoad: OnLoad,
	onLoadFile: onLoadFile,
	submit: subm,
	checkcookie: checkcookie,
	getcookie: getcookie,
	message: ch_status,
	}
}();
