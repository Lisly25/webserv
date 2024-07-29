SRC = $(shell find srcs -name *.cpp)
OBJS = $(SRC:.cpp=.o)
CXX = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++11 -pedantic -I./headers
NAME = webserv

DOCKER_COMPOSE_FILE := ./docker-services/docker-compose.yml
TESTS_DIR := ./tests
TARBALL := $(TESTS_DIR)/tests_binaries.tar.gz

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CPPFLAGS) $(OBJS) -o $(NAME)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)
	$(RM) -r $(TESTS_DIR)/cgi_tester $(TESTS_DIR)/tester $(TESTS_DIR)/ubuntu_cgi_tester $(TESTS_DIR)/ubuntu_tester

re: fclean $(NAME)


# TESTS ----

proxy-pass-test: up unpack-test $(NAME)
	./webserv ./tests/nginx_test.conf

up:
	mkdir -p ./docker-services/homer
	chmod 777 ./docker-services/homer
	docker compose -f $(DOCKER_COMPOSE_FILE) up -d || @docker-compose -f $(DOCKER_COMPOSE_FILE) up -d
	docker compose -f $(DOCKER_COMPOSE_FILE) logs || @docker-compose -f $(DOCKER_COMPOSE_FILE) logs

down:
	@docker compose -f $(DOCKER_COMPOSE_FILE) down || @docker-compose -f $(DOCKER_COMPOSE_FILE) down

unpack-test: $(TARBALL)
	tar -xvzf $(TARBALL) -C $(TESTS_DIR)

# ---

.PHONY: all clean fclean re proxy-pass-test up down unpack-test
