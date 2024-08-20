SRC = $(shell find srcs -name *.cpp)
OBJS = $(SRC:.cpp=.o)
DEPS = $(OBJS:.o=.d)
CXX = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++17 -pedantic $(addprefix -I, $(shell find srcs -type d)) -MMD -MP
NAME = webserv

DOCKER_COMPOSE_FILE := ./docker-services/docker-compose.yml
TESTS_DIR := ./tests
TARBALL := $(TESTS_DIR)/tests_binaries.tar.gz

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CPPFLAGS) $(OBJS) -o $(NAME)

-include $(DEPS)

clean: down
	$(RM) $(OBJS) $(DEPS)
	find srcs -type f \( -name "*.o" -o -name "*.d" \) -delete

fclean: clean
	$(RM) $(NAME)
	$(RM) -r $(TESTS_DIR)/cgi_tester $(TESTS_DIR)/tester $(TESTS_DIR)/ubuntu_cgi_tester $(TESTS_DIR)/ubuntu_tester

re: fclean $(NAME)

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

unpack-test: $(TARBALL)
	tar -xvzf $(TARBALL) -C $(TESTS_DIR)

conf-parse-test:
	c++ -Wall -Wextra -Werror -std=c++17 -ggdb3 srcs/WebErrors/WebErrors.cpp srcs/WebParser/WebParser.cpp srcs/config_parse_test_main.cpp -I srcs/WebErrors -lstdc++fs -o parseTest

run-nginx-server:
	docker build -t nginx-server ./nginx-debug
	docker run --rm -p 8085:8080 -v $(PWD)/fun_facts:/fun_facts nginx-server


# ---

.PHONY: all clean fclean re proxy-pass-test up down unpack-test conf-parse-test
