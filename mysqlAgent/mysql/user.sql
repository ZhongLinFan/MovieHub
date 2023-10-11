USE movie_hub;

DROP TABLE IF EXISTS `users`;
CREATE TABLE `users`(
	`uid` int(10) unsigned NOT NULL AUTO_INCREMENT,
	`name` varchar(32) NOT NULL,
	`passwd` varchar(32) NOT NULL,
	`state` tinyint(1) NOT NULL DEFAULT 0,
	PRIMARY KEY(`uid`)
)ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;


--状态位0代表离线，1代表在线
--插入数据
INSERT INTO `users`
	VALUES(NULL,  "小李", "1234", 0), (NULL, "Tony", "7689", 0);
INSERT INTO `users`
	VALUES(NULL,  "小企鹅", "1012", 1);
-- 查询
select * from `users`;
select * from `users` where `uid` = 1;
