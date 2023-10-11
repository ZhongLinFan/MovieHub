USE movie_hub;

DROP TABLE IF EXISTS `friends`;
CREATE TABLE `friends`(
	`uid` int(10) unsigned NOT NULL,
	`friend_id` int(10) unsigned NOT NULL,
	FOREIGN KEY(uid) REFERENCES `users`(uid),
	FOREIGN KEY(friend_id) REFERENCES `users`(uid),
	UNIQUE KEY(`uid`, `friend_id`)
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

--查询uid为5的所有朋友的 id
select * from `friends` where `uid` = 1 or `friend_id` = 1;   
--不过上面只能获得uid，如果需要获得信息可以进行如下
select `users`.uid, `users`.name from `users` where uid in(
	select `friends`.friend_id from `users` join `friends` on (`users`.uid = `friends`.uid) where `users`.uid = 1 union
	select `friends`.uid from `users` join `friends` on (`users`.uid = `friends`.friend_id) where `users`.uid = 1
);

--或者
select `users`.uid, `users`.name from `users` where uid in(
	select `friends`.friend_id from `friends` where `uid` = 1 union
	select `friends`.uid from `friends` where `friend_id` = 1 
);

--添加好友时，只需要添加一个1-2或者2-1而不需要同时添加
INSERT INTO `friends`
	VALUES(1, 2),(3, 1);

--删除好友
delete from `friends`  where (uid = 1 and friend_id = 3 ) or (uid = 3 and friend_id = 1);