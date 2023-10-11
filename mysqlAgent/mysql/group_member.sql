USE movie_hub;

DROP TABLE IF EXISTS `group_member`;
CREATE TABLE `group_member`(
	`gid` int(10) unsigned NOT NULL,
	`uid` int(10) unsigned NOT NULL,
	FOREIGN KEY(`uid`) REFERENCES `users`(`uid`),
	FOREIGN KEY(`gid`) REFERENCES `group`(`gid`),
	UNIQUE KEY(`gid`, `uid`)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

--注意1-2和2-1的意思不一样

--查询群号为1的所有成员信息
select `uid`, `name` from `users` where uid in(select `uid` from `group_member` where `gid` = 1); 

--查询uid为5的所有组
select `gid`, `name` from `group` where `gid` in (select `gid` from `group_member` where `uid` = 1);

--删除某个群的某个成员
delete from `group_member` where `gid` = 1 and `uid` = 1;

--删除某个群
delete from `group_member` where `gid` = 1;
delete from	`group` where `gid` = 1;

--创建某个群
INSERT INTO `group`
	VALUES(NULL, "项目交流群","大家踊跃交流近期的项目经验");  
INSERT INTO `group_member`
	VALUES(xxx, uid),

--添加群
INSERT INTO `group_member`
	VALUES(1, 1), (2, 1);  
INSERT INTO `group_member`
	VALUES(1, 2), (1, 3);