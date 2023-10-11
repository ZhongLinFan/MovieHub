#include "MysqlAgent.h"

int main() {
	MysqlAgent* agent = new MysqlAgent(10000, 10, "movie_hub");
	agent->run();
}