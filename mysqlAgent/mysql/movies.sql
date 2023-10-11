USE movie_hub;

DROP TABLE IF EXISTS `movies`;
CREATE TABLE `movies`(
	`fid` int(10) unsigned NOT NULL AUTO_INCREMENT,
	`name` varchar(32) NOT NULL,
	`path` varchar(256) NOT NULL,
	`summary` varchar(255) NOT NULL,
	PRIMARY KEY(`fid`)
)ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

INSERT INTO `movies`
	VALUES(NULL,  "喜剧之王.mp4", "/home/tony", "一部很有喜剧特色的电影"), (NULL, "赌神.mp4", "/home/tony", "看的让人热血沸腾");

UPDATE `movies` SET `name` ="赌神.mp4" where fid = 2;

	-- 查询
select * from `movies`;
select * from `movies` where `fid` = 1;

INSERT INTO `movies`
	VALUES(NULL,  "我不是杀手.mp4", "/home/tony", "情节设计的相当巧妙");
