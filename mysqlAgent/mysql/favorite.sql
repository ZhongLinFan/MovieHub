--用户喜欢的电影列表
--users表和movies表的关系（多对多）
USE movie_hub;

DROP TABLE IF EXISTS `favorites`;
CREATE TABLE `favorites`(
	`uid` int(10) unsigned NOT NULL,
	`fid` int(10) unsigned NOT NULL,
	FOREIGN KEY(`uid`) REFERENCES `users`(`uid`),
	FOREIGN KEY(`fid`) REFERENCES `movies`(`fid`),
	UNIQUE KEY(`uid`, `fid`)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- 注意，先添加两个父表记录，再添加子表（本表）记录，删除时，先删除子表记录，再删除父表记录
--第一个数据代表uid喜欢赌神，第二条代表，uid为2的喜欢喜剧之王

--注意，这里的2-1和1-2不是一个含义，而friend确表达的是同一个意思，因为朋友总是双向的，所以朋友表可以设计的不同
--UNIQUE KEY(`uid`, `fid`);这个是保障一个人只能喜欢一次同一个电影
INSERT INTO `favorites`
	VALUES(1, 2), (1, 1),(2, 1);

--查询
--uid为5的喜欢的电影列表
select movies.name from `users` join `favorites` on `users`.uid = `favorites`.uid join `movies` on `favorites`.fid =`movies`.fid where `users`.uid = 1;
