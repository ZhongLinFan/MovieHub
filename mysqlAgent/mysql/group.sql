USE movie_hub;

DROP TABLE IF EXISTS `group`;
CREATE TABLE `group`(
	`gid` int(10) unsigned NOT NULL AUTO_INCREMENT,
	`name` varchar(32) NOT NULL,
	`summary` varchar(64) NOT NULL DEFAULT "",
	`owner` int(10) unsigned NOT NULL,
	`member_nums` int(10) unsigned NOT NULL,
	PRIMARY KEY(`gid`)
)ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

--创建群,注意创建完成后记得将当前创建的人加入群聊
INSERT INTO `group`
	VALUES(NULL,  "学习交流群", "欢迎爱学习的同学相聚于此", 2, 1), (NULL, "相亲相爱一家亲", "一家人不说两家话", 1, 1);
INSERT INTO `group_member`
	VALUES(1, 2),(2, 1);


--查询
select * from `group`;
select * from `group` where `gid` = 1;
select * from `group` where `name` = "学习交流群";
