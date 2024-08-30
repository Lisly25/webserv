SRC = $(shell find srcs -name *.cpp)
OBJS = $(SRC:.cpp=.o)
DEPS = $(OBJS:.o=.d)
CXX = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++17 -pedantic $(addprefix -I, $(shell find srcs -type d)) -MMD -MP
NAME = webserv

DOCKER_COMPOSE_FILE := ./docker-services/docker-compose.yml

CGI_TESTS_DIR = ~/HIVE/webserv/tests/cgi-tests

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CPPFLAGS) $(OBJS) -o $(NAME)

-include $(DEPS)

clean: down
	$(RM) $(OBJS) $(DEPS)
	find srcs -type f \( -name "*.o" -o -name "*.d" \) -delete

fclean: clean
	$(RM) $(NAME)
	find $(CGI_TESTS_DIR) -type f ! -name 'POST-EXAMPLES.tar.gz.part*' -delete

re: fclean $(NAME)

run-cgi-tests:
	cat  $(CGI_TESTS_DIR)/POST-EXAMPLES.tar.gz.part-* > $(CGI_TESTS_DIR)/POST-EXAMPLES.tar.gz
	tar -xzvf $(CGI_TESTS_DIR)/POST-EXAMPLES.tar.gz -C $(CGI_TESTS_DIR)
	./tests/CGI-POST-DELETE-TEST.sh

# TESTS ----

proxy-cgi-test: up unpack-test $(NAME)
	./webserv ./tests/proxy-cgi-test.conf

up:
	mkdir -p ./docker-services/homer
	chmod 777 ./docker-services/homer
	docker compose -f $(DOCKER_COMPOSE_FILE) up -d || @docker-compose -f $(DOCKER_COMPOSE_FILE) up -d
	docker compose -f $(DOCKER_COMPOSE_FILE) logs || @docker-compose -f $(DOCKER_COMPOSE_FILE) logs

down:
	@docker compose -f $(DOCKER_COMPOSE_FILE) down


conf-parse-test:
	c++ -Wall -Wextra -Werror -std=c++17 -ggdb3 srcs/WebErrors/WebErrors.cpp srcs/WebParser/WebParser.cpp srcs/config_parse_test_main.cpp -I srcs/WebErrors -lstdc++fs -o parseTest


.PHONY: all clean fclean re proxy-pass-test up down unpack-test conf-parse-test
