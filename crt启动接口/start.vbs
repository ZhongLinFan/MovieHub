# $language = "VBScript"
# $interface = "1.0"

'����һ��tabҳ��Ȼ���л�����ĿĿ¼�µ����·���£�ִ����Ӧ�ĳ���
Sub Main()
	'����mysqlAgent
	Call RunProgram("/home/tony/myprojects/MovieHub/mysqlAgent/bin", "mysqlAgent")
	'����lbAgent
	Call RunProgram( "/home/tony/myprojects/MovieHub/lbAgent/bin", "lbServer")
	'��������������1
	Call RunProgram("/home/tony/myprojects/MovieHub/baseServer/bin", "baseServer 127.0.0.1 10010")
	'��������������2
	Call RunProgram( "/home/tony/myprojects/MovieHub/baseServer/bin", "baseServer 127.0.0.1 10011")
	'������ý�������1
	Call RunProgram( "/home/tony/myprojects/MovieHub/mediaServer/bin", "mediaServer 127.0.0.1 11000")
	'������ý�������2
	Call RunProgram("/home/tony/myprojects/MovieHub/mediaServer/bin", "mediaServer 127.0.0.1 11001")
End Sub

Function ConnectHost(ByRef tab)
	'����һ����������
	On Error Resume Next
	'���Ӳ���
	'ע��һ��Ҫ����tabName������ֻ�е�һ�������ܹ�����������
	'������tabҳ��ʵ���ڵȴ���һ��tabҳ��crt.Screen.WaitForString("@heihei", 200, bMilliseconds=true)
	'tabΪRunProgram�ľֲ����󣬱���Ҫ���������ã�RunProgram�ķ���ָ��Ҳ����Ϊcrt��
	'��Ӧ��Ϊtab������Ҳ����ִ������tabΪ��ǰ�����ֲ�������Ȼʹ�õ�һ������
	set tab = crt.Session.ConnectInTab ("/SSH2 /PASSWORD 1234 tony@192.168.171.169", True)
	If Err.Number <> 0 Then
		MsgBox "Connection Failed"
		ConnectHost = False 
		Exit Function
	End If
	if TabIsReady(tab) <> True Then
		MsgBox "�ȴ��쳣"
		ConnectHost = False 
		Exit Function
	End if
	'���ӳɹ�
	ConnectHost = True 
End Function

Function RunProgram(path, pragramName)
	'��һ����ǩҳ����mysql������
	dim tab '��ǰ��tabҳ
	if ConnectHost(tab) <> True Then 
		RunProgram = False
		Exit Function
	End if	
	if TabIsReady(tab) <> True Then
		MsgBox "�ȴ��쳣"
		Exit Function
	End if
	tab.Screen.Send "cd " & path
	tab.screen.sendkeys("{ENTER}")
	tab.Screen.WaitForString("@heihei")
	tab.Screen.Send "./" & pragramName
	tab.screen.sendkeys("{ENTER}")
	RunProgram = True
	'tab.Screen.WaitForString("@heihei", 2)  '�ٴ���ʱ�ȴ��л�
End Function

Function TabIsReady(ByRef tab)
	'����ʱ����ʾ���л�ҳ�����
	waitTimes = 1
	'û�еȴ����������ѭ��
	tab.screen.sendkeys("{ENTER}")  'ע������һ��ҪΪtab�������crt����ô������tab����ȴ���һ��tabҳ
	do while tab.Screen.WaitForString("@heihei", 200, bMilliseconds=true) <> True  '����ȴ�
		tab.screen.sendkeys("{ENTER}")
		a=a+1
		if a = 10 then
			TabIsReady = false
			Exit Function
		end if
	loop
	TabIsReady = true
End Function