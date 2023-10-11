# $language = "VBScript"
# $interface = "1.0"

Set scriptTab = crt.GetScriptTab  '注意运行脚本的那个窗口不能关闭，在第一个窗口能够正常执行，后续窗口会出错

Sub Main()
    ' 激活每个tab页并发送ctrl+c， 如果没有连接，那就不发送数据，并且直接关闭
	'从右往左停止
    Dim nIndex
    For nIndex = 1 to crt.GetTabCount
        Set objCurrentTab = crt.GetTab(crt.GetTabCount)  '这里必须要使用GetTabCount或者1，因为，页面的下标在你删除之后会动态变化，样例是错的
        objCurrentTab.Activate
        if objCurrentTab.Session.Connected = True then
            objCurrentTab.Screen.Send "^c" & vbcr
			objCurrentTab.Screen.WaitForString "@heihei", 500, bMilliseconds=true	'这里存粹是为了休眠0.5	
			objCurrentTab.Session.Disconnect	'如果不断连，会弹出窗口			
        end if
		'如果当前窗口为moviehub的服务器窗口则关闭，注意，脚本窗口无法关闭
		if objCurrentTab.Index <> scriptTab.Index then
			objCurrentTab.Close	'所有的页面都关闭
		End if	
    Next
	scriptTab.Activate
End Sub