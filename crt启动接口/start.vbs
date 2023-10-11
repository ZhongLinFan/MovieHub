# $language = "VBScript"
# $interface = "1.0"

'启动一个tab页，然后切换到项目目录下的相关路径下，执行相应的程序
Sub Main()
	'启动mysqlAgent
	Call RunProgram("/home/tony/myprojects/MovieHub/mysqlAgent/bin", "mysqlAgent")
	'启动lbAgent
	Call RunProgram( "/home/tony/myprojects/MovieHub/lbAgent/bin", "lbServer")
	'启动基础服务器1
	Call RunProgram("/home/tony/myprojects/MovieHub/baseServer/bin", "baseServer 127.0.0.1 10010")
	'启动基础服务器2
	Call RunProgram( "/home/tony/myprojects/MovieHub/baseServer/bin", "baseServer 127.0.0.1 10011")
	'启动流媒体服务器1
	Call RunProgram( "/home/tony/myprojects/MovieHub/mediaServer/bin", "mediaServer 127.0.0.1 11000")
	'启动流媒体服务器2
	Call RunProgram("/home/tony/myprojects/MovieHub/mediaServer/bin", "mediaServer 127.0.0.1 11001")
End Sub

Function ConnectHost(ByRef tab)
	'开启一个窗口连接
	On Error Resume Next
	'连接操作
	'注意一定要设置tabName，否则只有第一个程序能够正常启动，
	'后续的tab页其实都在等待第一个tab页的crt.Screen.WaitForString("@heihei", 200, bMilliseconds=true)
	'tab为RunProgram的局部对象，必须要在那里设置，RunProgram的发送指令也不能为crt，
	'而应该为tab，否则也会出现错误，如果tab为当前函数局部对象，依然使用第一个窗口
	set tab = crt.Session.ConnectInTab ("/SSH2 /PASSWORD 1234 tony@192.168.171.169", True)
	If Err.Number <> 0 Then
		MsgBox "Connection Failed"
		ConnectHost = False 
		Exit Function
	End If
	if TabIsReady(tab) <> True Then
		MsgBox "等待异常"
		ConnectHost = False 
		Exit Function
	End if
	'连接成功
	ConnectHost = True 
End Function

Function RunProgram(path, pragramName)
	'第一个标签页启动mysql服务器
	dim tab '当前的tab页
	if ConnectHost(tab) <> True Then 
		RunProgram = False
		Exit Function
	End if	
	if TabIsReady(tab) <> True Then
		MsgBox "等待异常"
		Exit Function
	End if
	tab.Screen.Send "cd " & path
	tab.screen.sendkeys("{ENTER}")
	tab.Screen.WaitForString("@heihei")
	tab.Screen.Send "./" & pragramName
	tab.screen.sendkeys("{ENTER}")
	RunProgram = True
	'tab.Screen.WaitForString("@heihei", 2)  '再次延时等待切换
End Function

Function TabIsReady(ByRef tab)
	'两秒时间显示和切换页面完成
	waitTimes = 1
	'没有等待到，会进入循环
	tab.screen.sendkeys("{ENTER}")  '注意这里一定要为tab，如果是crt，那么后续的tab都会等待第一个tab页
	do while tab.Screen.WaitForString("@heihei", 200, bMilliseconds=true) <> True  '毫秒等待
		tab.screen.sendkeys("{ENTER}")
		a=a+1
		if a = 10 then
			TabIsReady = false
			Exit Function
		end if
	loop
	TabIsReady = true
End Function