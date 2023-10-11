--创建
USE movie_hub;

DROP TABLE IF EXISTS `router`;
CREATE TABLE `router`(
	`id` int(10) unsigned NOT NULL AUTO_INCREMENT,
	`modid` int(10) unsigned NOT NULL,
	`server_ip` varchar(16) NOT NULL,  
	`server_port` int(10) unsigned NOT NULL,
	`modify_time` timestamp 
	NOT NULL DEFAULT CURRENT_TIMESTAMP 
	ON UPDATE CURRENT_TIMESTAMP, 	
	UNIQUE KEY(`id`),
	PRIMARY KEY(`modid`, `server_ip`, `server_port`)
)ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;


--自增长必须要是主键或者唯一(如果不设置会报错Incorrect table definition; there can be only one auto column and it must be defined as a key)

--插入
INSERT INTO `router`
	VALUES(NULL, 1, "127.0.0.1", 10010, NULL),(NULL, 1, "127.0.0.1", 10011, NULL), (NULL, 2, "127.0.0.1", 11000, NULL), (NULL, 2, "127.0.0.1", 11001, NULL);

--查询
select * from router;
	
--解释	
	--上面不能有注释
	`server_ip` varchar(16),  --都是本地，需要转为网络字节序
	`server_port` int(10) unsigned NOT NULL,  --都是本地，需要转为网络字节序
	`modify_time` timestamp 
	NOT NULL DEFAULT CURRENT_TIMESTAMP 
	ON UPDATE CURRENT_TIMESTAMP, --NOT NULL DEFAULT CURRENT_TIMESTAMP这一句是插入的时候没有的话，使用现在时间戳，UPDATE CURRENT_TIMESTAMP这一句是更新的时候更新时间戳 